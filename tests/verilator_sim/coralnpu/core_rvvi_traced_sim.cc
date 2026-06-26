// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define STRINGIZE(x) #x
#define STR(x) STRINGIZE(x)
#define MODEL_HEADER_SUFFIX .h
#define MODEL_HEADER STR(VERILATOR_MODEL MODEL_HEADER_SUFFIX)
#include MODEL_HEADER

#define PARAMS_HEADER_PREFIX hdl/chisel/src/coralnpu/
#define PARAMS_HEADER_SUFFIX _parameters.h
#define PARAMS_HEADER STR(PARAMS_HEADER_PREFIX VERILATOR_MODEL PARAMS_HEADER_SUFFIX)
#include PARAMS_HEADER

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits>
#include <thread>
#include <iostream>
#include <fstream>
#include <chrono>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "tests/verilator_sim/coralnpu/core_if.h"
#include "tests/verilator_sim/coralnpu/coralnpu_cfg.h"
#include "tests/verilator_sim/sysc_tb.h"
#include "tests/verilator_sim/util.h"
#include "tests/verilator_sim/elf.h"
#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "tests/verilator_sim/rvvi/spsc_ring_buffer.h"
#include "tests/verilator_sim/rvvi/trace_daemon.h"
#include "tests/verilator_sim/rvvi/custom_fallback_formatter.h"

using namespace mpact::sim::riscv::rvvi;

ABSL_FLAG(int, instructions, 500000, "Instruction timeout");
ABSL_FLAG(std::string, rvvi_out, "trace.rvvi", "RVVI trace output file");
ABSL_FLAG(std::string, memory_profile, "default", "Memory profile ('default' or 'highmem')");

struct RvviTraced_tb : Sysc_tb {
  sc_in<bool> io_halted;
  
  // RVVI ports
  // Note: These must match the Core module's RvviTrace interface
  sc_in<bool> io_debug_rb_inst_0_valid;
  sc_in<sc_bv<KP_programCounterBits>> io_debug_rb_inst_0_bits_pc;
  sc_in<sc_bv<32>> io_debug_rb_inst_0_bits_inst;
  sc_in<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_0_bits_idx;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_data;
  sc_in<bool> io_debug_rb_inst_0_bits_trap;

  SpscRingBuffer<TracePacket, 4096>* buffer;
  uint32_t internal_v_id = 0;
  uint64_t instruction_count = 0;
  uint64_t instruction_limit = 500000;

  SC_HAS_PROCESS(RvviTraced_tb);

  RvviTraced_tb(sc_module_name name, int instruction_limit, SpscRingBuffer<TracePacket, 4096>* buf) 
    : Sysc_tb(name, std::numeric_limits<int>::max(), false), buffer(buf), instruction_limit(instruction_limit) {
  }

  void posedge() {
    if (!reset.read()) {
        // Trace Extraction
        if (io_debug_rb_inst_0_valid.read()) {
            TracePacket ipacket = {};
            ipacket.type = 'I';
            ipacket.v_id = internal_v_id++;
            ipacket.inst.pc = io_debug_rb_inst_0_bits_pc.read().to_uint64();
            ipacket.inst.instruction = io_debug_rb_inst_0_bits_inst.read().to_uint();
            {
              auto start = std::chrono::steady_clock::now();
              while (!buffer->Push(ipacket)) {
                std::this_thread::yield();
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() > 5) {
                  fprintf(stderr, "[FATAL] Queue backpressure timeout! Watchdog triggered.\n");
                  exit(1);
                }
              }
            }
        }

        // Termination detection
        if (io_halted.read()) {
            sc_stop();
        }
        
        // Timeout check
        instruction_count++;
        if (instruction_count >= instruction_limit) {
            fprintf(stderr, "Simulation TIMEOUT after %lu instructions.\n", instruction_count);
            sc_stop();
        }
    }
  }
};

bool LoadElfToMemory(const std::string& file_name, Core_if& mif) {
  int fd = open(file_name.c_str(), O_RDONLY);
  if (fd < 0) { LOG(ERROR) << "Failed to open ELF file: " << file_name; return false; }
  struct stat sb;
  if (fstat(fd, &sb) != 0) { close(fd); return false; }
  auto file_size = sb.st_size;
  auto file_data = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (file_data == MAP_FAILED) { close(fd); return false; }
  close(fd);

  uint32_t elf_magic = 0x464c457f;
  uint8_t* data8 = reinterpret_cast<uint8_t*>(file_data);
  bool load_ok = true;
  if (memcmp(file_data, &elf_magic, sizeof(elf_magic)) == 0) {
    ::LoadElf(data8,
              [&mif, &load_ok](void* dest, const void* src, size_t count) {
                uint64_t addr = reinterpret_cast<uint64_t>(dest);
                if (!mif.Write(addr, count, reinterpret_cast<const uint8_t*>(src))) {
                    load_ok = false;
                }
                return dest;
              });
    munmap(file_data, file_size);
    return load_ok;
  }
  munmap(file_data, file_size);
  return false;
}

int sc_main(int argc, char *argv[]) {
  absl::SetProgramUsageMessage("CoralNPU RVVI Traced Simulation Target");
  auto out_args = absl::ParseCommandLine(argc, argv);
  if (out_args.size() != 2) { LOG(ERROR) << "Need one binary/ELF input file"; return 1; }
  const char* bin = out_args[1];

  SpscRingBuffer<TracePacket, 4096> buffer;
  std::ofstream trace_stream(absl::GetFlag(FLAGS_rvvi_out));
  TraceDaemon daemon(&buffer, &trace_stream);
  CustomFallbackFormatter formatter;
  daemon.SetTraceFormatter(&formatter);
  daemon.Start();

  VERILATOR_MODEL core("Core");
  RvviTraced_tb tb("RvviTraced_tb", absl::GetFlag(FLAGS_instructions), &buffer);
  Core_if mif("Core_if", nullptr, absl::GetFlag(FLAGS_memory_profile));

  if (!LoadElfToMemory(bin, mif)) { return 65; }

  sc_signal<bool> io_halted;
  core.clock(tb.clock);
  core.reset(tb.reset);
  core.io_halted(io_halted);
  tb.io_halted(io_halted);
  
  // RVVI port mapping
  sc_signal<bool> io_debug_rb_inst_0_valid;
  sc_signal<sc_bv<KP_programCounterBits>> io_debug_rb_inst_0_bits_pc;
  sc_signal<sc_bv<32>> io_debug_rb_inst_0_bits_inst;
  sc_signal<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_0_bits_idx;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_data;
  sc_signal<bool> io_debug_rb_inst_0_bits_trap;

  core.io_debug_rb_inst_0_valid(io_debug_rb_inst_0_valid);
  core.io_debug_rb_inst_0_bits_pc(io_debug_rb_inst_0_bits_pc);
  core.io_debug_rb_inst_0_bits_inst(io_debug_rb_inst_0_bits_inst);
  core.io_debug_rb_inst_0_bits_idx(io_debug_rb_inst_0_bits_idx);
  core.io_debug_rb_inst_0_bits_data(io_debug_rb_inst_0_bits_data);
  core.io_debug_rb_inst_0_bits_trap(io_debug_rb_inst_0_bits_trap);

  tb.io_debug_rb_inst_0_valid(io_debug_rb_inst_0_valid);
  tb.io_debug_rb_inst_0_bits_pc(io_debug_rb_inst_0_bits_pc);
  tb.io_debug_rb_inst_0_bits_inst(io_debug_rb_inst_0_bits_inst);
  tb.io_debug_rb_inst_0_bits_idx(io_debug_rb_inst_0_bits_idx);
  tb.io_debug_rb_inst_0_bits_data(io_debug_rb_inst_0_bits_data);
  tb.io_debug_rb_inst_0_bits_trap(io_debug_rb_inst_0_bits_trap);

  // TODO: Add other necessary port connections for IF/DF/Memory
  
  tb.start();
  
  // Flush
  while(!buffer.IsEmpty()) { std::this_thread::yield(); }
  daemon.Stop();
  
  return 0;
}
