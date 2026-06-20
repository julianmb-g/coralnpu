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

#define PARAMS_HEADER_PREFIX tests/verilator_sim/coralnpu/
#define PARAMS_HEADER_SUFFIX _parameters.h
#define PARAMS_HEADER STR(PARAMS_HEADER_PREFIX VERILATOR_MODEL PARAMS_HEADER_SUFFIX)
#include PARAMS_HEADER

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "tests/verilator_sim/coralnpu/core_if.h"
#include "tests/verilator_sim/coralnpu/debug_if.h"
#include "tests/verilator_sim/coralnpu/coralnpu_cfg.h"
#include "tests/verilator_sim/sysc_tb.h"
#include "tests/verilator_sim/util.h"
#include "tests/verilator_sim/elf.h"

ABSL_FLAG(int, cycles, 500000, "Simulation cycles");
ABSL_FLAG(bool, trace, false, "Dump VCD trace");
ABSL_FLAG(std::string, memory_profile, "default", "Memory profile ('default' or 'highmem')");

struct Core_tb : Sysc_tb {
  sc_in<bool> io_halted;
  sc_in<bool> io_fault;

  using Sysc_tb::Sysc_tb;  // constructor

  void posedge() {
    check(!io_fault, "io_fault");
    if (io_halted) sc_stop();
  }
};

bool LoadElfToMemory(const std::string& file_name, Core_if& mif) {
  int fd = open(file_name.c_str(), O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Failed to open ELF file: %s\n", file_name.c_str());
    return false;
  }
  struct stat sb;
  if (fstat(fd, &sb) != 0) {
    close(fd);
    return false;
  }
  auto file_size = sb.st_size;
  auto file_data = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (file_data == MAP_FAILED) {
    close(fd);
    return false;
  }
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

static void Core_run(const char* name, const char* bin, const int cycles,
                     const bool trace, const std::string& memory_profile) {
  VERILATOR_MODEL core(name);
  Core_tb tb("Core_tb", cycles, /* random= */ false);
  Core_if mif("Core_if", nullptr, memory_profile); // nullptr since we will load ELF

  if (!LoadElfToMemory(bin, mif)) {
    fprintf(stderr, "Error backdoor loading ELF: %s\n", bin);
    exit(-1);
  }

  sc_signal<bool> io_halted;
  sc_signal<bool> io_fault;
  sc_signal<bool> io_wfi;
  sc_signal<bool> io_irq;
  sc_signal<bool> io_debug_req;
  sc_signal<bool> io_ibus_valid;
  sc_signal<bool> io_ibus_ready;
  sc_signal<bool> io_ibus_fault_valid;
  sc_signal<bool> io_ibus_fault_bits_write;
  sc_signal<bool> io_dbus_valid;
  sc_signal<bool> io_dbus_ready;
  sc_signal<bool> io_dbus_write;
  sc_signal<bool> io_iflush_valid;
  sc_signal<sc_bv<KP_programCounterBits> > io_iflush_pcNext;
  sc_signal<bool> io_iflush_ready;
  sc_signal<bool> io_dflush_valid;
  sc_signal<bool> io_dflush_ready;
  sc_signal<bool> io_dflush_all;
  sc_signal<bool> io_dflush_clean;
  sc_signal<sc_bv<KP_programCounterBits> > io_ibus_addr;
  sc_signal<sc_bv<KP_fetchDataBits> > io_ibus_rdata;
  sc_signal<sc_bv<KP_programCounterBits>> io_ibus_fault_bits_epc;
  sc_signal<sc_bv<KP_programCounterBits>> io_ibus_fault_bits_addr;
  sc_signal<sc_bv<KP_lsuAddrBits> > io_dbus_addr;
  sc_signal<sc_bv<KP_lsuAddrBits> > io_dbus_adrx;
  sc_signal<sc_bv<32> > io_dbus_pc;
  sc_signal<sc_bv<KP_dbusSize> > io_dbus_size;
  sc_signal<sc_bv<KP_lsuDataBits> > io_dbus_wdata;
  sc_signal<sc_bv<KP_lsuDataBits / 8> > io_dbus_wmask;
  sc_signal<sc_bv<KP_lsuDataBits> > io_dbus_rdata;

  io_iflush_ready = 1;
  io_dflush_ready = 1;

  tb.io_halted(io_halted);
  tb.io_fault(io_fault);

  core.clock(tb.clock);
  core.reset(tb.reset);
  core.io_halted(io_halted);
  core.io_fault(io_fault);
  core.io_wfi(io_wfi);
  core.io_ibus_valid(io_ibus_valid);
  core.io_ibus_ready(io_ibus_ready);
  core.io_ibus_addr(io_ibus_addr);
  core.io_ibus_rdata(io_ibus_rdata);
  core.io_dbus_valid(io_dbus_valid);
  core.io_dbus_ready(io_dbus_ready);
  core.io_dbus_write(io_dbus_write);
  core.io_dbus_addr(io_dbus_addr);
  core.io_dbus_size(io_dbus_size);
  core.io_dbus_wdata(io_dbus_wdata);
  core.io_dbus_wmask(io_dbus_wmask);
  core.io_dbus_rdata(io_dbus_rdata);

  mif.clock(tb.clock);
  mif.reset(tb.reset);
  mif.io_ibus_valid(io_ibus_valid);
  mif.io_ibus_ready(io_ibus_ready);
  mif.io_ibus_addr(io_ibus_addr);
  mif.io_ibus_rdata(io_ibus_rdata);
  mif.io_dbus_valid(io_dbus_valid);
  mif.io_dbus_ready(io_dbus_ready);
  mif.io_dbus_write(io_dbus_write);
  mif.io_dbus_addr(io_dbus_addr);
  mif.io_dbus_adrx(io_dbus_adrx);
  mif.io_dbus_size(io_dbus_size);
  mif.io_dbus_wdata(io_dbus_wdata);
  mif.io_dbus_wmask(io_dbus_wmask);
  mif.io_dbus_rdata(io_dbus_rdata);
  mif.io_ibus_fault_valid(io_ibus_fault_valid);
  mif.io_ibus_fault_bits_write(io_ibus_fault_bits_write);
  mif.io_ibus_fault_bits_addr(io_ibus_fault_bits_addr);
  mif.io_ibus_fault_bits_epc(io_ibus_fault_bits_epc);

  if (trace) {
    tb.trace(&core);
  }

  tb.reset = 1;
  core.reset.val = 1;
  core.eval();
  mif.eval();
  
  tb.reset = 0;
  core.reset.val = 0;
  for (int i = 0; i < cycles; ++i) {
    // Toggle clock for Verilator model
    core.clock.val = !core.clock.val;
    
    core.eval();
    mif.eval();
    
    // Manual propagation for mock systemc.h
    tb.io_halted.val = core.io_halted.val;
    tb.io_fault.val = core.io_fault.val;

    tb.posedge();
    if (tb.io_halted.val) break;
  }
}

int sc_main(int argc, char *argv[]) {
  absl::SetProgramUsageMessage("CoralNPU Barebones Simulation Tool");
  auto out_args = absl::ParseCommandLine(argc, argv);
  argc = out_args.size();
  argv = &out_args[0];
  if (argc != 2) {
    fprintf(stderr, "Need one binary/ELF input file\n");
    return 1;
  }
  const char* path = argv[1];

  Core_run(Sysc_tb::get_name(argv[0]), path, absl::GetFlag(FLAGS_cycles),
           absl::GetFlag(FLAGS_trace), absl::GetFlag(FLAGS_memory_profile));
  return 0;
}
