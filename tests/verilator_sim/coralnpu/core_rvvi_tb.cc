// Copyright 2026 Google LLC
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
#include <thread>
#include <iostream>
#include <fstream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "tests/verilator_sim/coralnpu/core_if.h"
#include "tests/verilator_sim/coralnpu/debug_if.h"
#include "tests/verilator_sim/coralnpu/coralnpu_cfg.h"
#include "tests/verilator_sim/sysc_tb.h"
#include "tests/verilator_sim/util.h"
#include "tests/verilator_sim/elf.h"
#include "tests/verilator_sim/rvvi/spsc_ring_buffer.h"
#include "tests/verilator_sim/rvvi/trace_daemon.h"
#include "tests/verilator_sim/rvvi/custom_fallback_formatter.h"

#ifdef DELAY_FORMATTER
#include <chrono>

namespace mpact::sim::riscv::rvvi {
class DelayFormatter : public CustomFallbackFormatter {
public:
  std::string Disassemble(uint32_t inst) override {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return CustomFallbackFormatter::Disassemble(inst);
  }
};
}
#endif

using namespace mpact::sim::riscv::rvvi;

ABSL_FLAG(int, cycles, 500000, "Simulation cycles");
ABSL_FLAG(bool, trace, false, "Dump VCD trace");
ABSL_FLAG(std::string, rvvi_out, "trace.rvvi", "RVVI trace output file");
ABSL_FLAG(std::string, memory_profile, "default", "Memory profile ('default' or 'highmem')");

struct CoreRvvi_tb : Sysc_tb {
  sc_in<bool> io_halted;
  sc_in<bool> io_fault;

  
  // RVVI ports
  sc_in<bool> io_debug_rb_inst_0_valid;
  sc_in<sc_bv<KP_programCounterBits>> io_debug_rb_inst_0_bits_pc;
  sc_in<sc_bv<32>> io_debug_rb_inst_0_bits_inst;
  sc_in<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_0_bits_idx;
  sc_in<sc_bv<KP_xlen>> io_debug_rb_inst_0_bits_data;
  sc_in<bool> io_debug_rb_inst_0_bits_trap;
  sc_in<bool> io_trace_halt;

  SpscRingBuffer<TracePacket, 4096>* buffer;

  CoreRvvi_tb(sc_module_name name, int cycles, bool random, SpscRingBuffer<TracePacket, 4096>* buf) 
    : Sysc_tb(name, cycles, random), buffer(buf) {}

  void posedge() {
    check(!io_fault, "io_fault");
    
    if (io_debug_rb_inst_0_valid.read()) {
      uint32_t v_id = io_debug_rb_inst_0_bits_idx.read().to_uint();
      TracePacket ipacket = {};
      ipacket.type = 'I';
      ipacket.v_id = v_id;
      ipacket.inst.pc = io_debug_rb_inst_0_bits_pc.read().to_uint64();
      ipacket.inst.instruction = io_debug_rb_inst_0_bits_inst.read().to_uint();
      
      while (!buffer->Push(ipacket)) {
        std::this_thread::yield();
      }

      // Finding 25: Register extraction logic
      uint32_t inst = ipacket.inst.instruction;
      uint32_t opcode = inst & 0x7f;
      bool writes_rd = (opcode == 0x13) || (opcode == 0x33) || (opcode == 0x37) || 
                       (opcode == 0x17) || (opcode == 0x6f) || (opcode == 0x67) || 
                       (opcode == 0x03) || (opcode == 0x73);
      
      if (writes_rd) {
        uint32_t rd = (inst >> 7) & 0x1f;
        if (rd != 0) {
          TracePacket rpacket = {};
          rpacket.type = 'R';
          rpacket.v_id = v_id;
          rpacket.reg.reg_type = 'X'; // GPR
          rpacket.reg.index = rd;
          rpacket.reg.size = 4;
          rpacket.reg.total_size = 8;
          rpacket.reg.value[0] = 0xDEADBEEF; // Garbage to detect read failures
          rpacket.reg.value[0] = io_debug_rb_inst_0_bits_data.read().to_uint64();
          
          while (!buffer->Push(rpacket)) {
            std::this_thread::yield();
          }
        }
      }

      if (io_trace_halt.read()) {
        TracePacket epacket = {};
        epacket.type = 'E';
        while (!buffer->Push(epacket)) {
          std::this_thread::yield();
        }
      }
    }

    if (io_halted.read()) {
      sc_stop();
    }
  }
};

bool LoadElfToMemory(const std::string& file_name, Core_if& mif, uint32_t& entry_point) {
  int fd = open(file_name.c_str(), O_RDONLY);
  if (fd < 0) return false;
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
    entry_point = ::LoadElf(data8,
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

static int CoreRvvi_run(const char* name, const char* bin, const int cycles,
                     const bool trace, const std::string& rvvi_out,
                     const std::string& memory_profile) {
  VERILATOR_MODEL core(name);
  SpscRingBuffer<TracePacket, 4096> buffer;
  CoreRvvi_tb tb("CoreRvvi_tb", cycles, false, &buffer);
  Core_if mif("Core_if", nullptr, memory_profile);

  uint32_t entry_point = 0x80000000;
  if (!LoadElfToMemory(bin, mif, entry_point)) {
    fprintf(stderr, "Error backdoor loading ELF: %s\n", bin);
    exit(-1);
  }

  std::ofstream trace_stream(rvvi_out);
  TraceDaemon daemon(&buffer, &trace_stream);
#ifdef DELAY_FORMATTER
  DelayFormatter formatter;
#else
  CustomFallbackFormatter formatter;
#endif
  daemon.SetTraceFormatter(&formatter);
  daemon.Start();

  sc_signal<bool> io_halted;
  sc_signal<bool> io_fault;
  sc_signal<bool> io_wfi;
  sc_signal<bool> io_ibus_valid;
  sc_signal<bool> io_ibus_ready;
  sc_signal<bool> io_ibus_fault_valid;
  sc_signal<bool> io_ibus_fault_bits_write;
  sc_signal<sc_bv<KP_programCounterBits>> io_ibus_fault_bits_epc;
  sc_signal<sc_bv<KP_programCounterBits>> io_ibus_fault_bits_addr;
  sc_signal<bool> io_dbus_valid;
  sc_signal<bool> io_dbus_ready;
  sc_signal<bool> io_dbus_write;
  sc_signal<sc_bv<KP_programCounterBits> > io_ibus_addr;
  sc_signal<sc_bv<KP_fetchDataBits> > io_ibus_rdata;
  sc_signal<sc_bv<KP_lsuAddrBits> > io_dbus_addr;
  sc_signal<sc_bv<KP_lsuAddrBits> > io_dbus_adrx;
  sc_signal<sc_bv<KP_dbusSize> > io_dbus_size;
  sc_signal<sc_bv<KP_lsuDataBits> > io_dbus_wdata;
  sc_signal<sc_bv<KP_lsuDataBits / 8> > io_dbus_wmask;
  sc_signal<sc_bv<KP_lsuDataBits> > io_dbus_rdata;

  sc_signal<bool> io_debug_rb_inst_0_valid;
  sc_signal<sc_bv<KP_programCounterBits>> io_debug_rb_inst_0_bits_pc;
  sc_signal<sc_bv<32>> io_debug_rb_inst_0_bits_inst;
  sc_signal<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_0_bits_idx;
  sc_signal<sc_bv<KP_xlen>> io_debug_rb_inst_0_bits_data;
  sc_signal<bool> io_debug_rb_inst_0_bits_trap;
  sc_signal<bool> io_trace_halt;

  tb.io_halted(io_halted);
  tb.io_fault(io_fault);
  tb.io_debug_rb_inst_0_valid(io_debug_rb_inst_0_valid);
  tb.io_debug_rb_inst_0_bits_pc(io_debug_rb_inst_0_bits_pc);
  tb.io_debug_rb_inst_0_bits_inst(io_debug_rb_inst_0_bits_inst);
  tb.io_debug_rb_inst_0_bits_idx(io_debug_rb_inst_0_bits_idx);
  tb.io_debug_rb_inst_0_bits_data(io_debug_rb_inst_0_bits_data);
  tb.io_debug_rb_inst_0_bits_trap(io_debug_rb_inst_0_bits_trap);
  tb.io_trace_halt(io_trace_halt);

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
  core.io_ibus_fault_valid(io_ibus_fault_valid);
  core.io_ibus_fault_bits_write(io_ibus_fault_bits_write);
  core.io_ibus_fault_bits_addr(io_ibus_fault_bits_addr);
  core.io_ibus_fault_bits_epc(io_ibus_fault_bits_epc);
  core.io_dbus_adrx(io_dbus_adrx);
  
  core.io_debug_rb_inst_0_valid(io_debug_rb_inst_0_valid);
  core.io_debug_rb_inst_0_bits_pc(io_debug_rb_inst_0_bits_pc);
  core.io_debug_rb_inst_0_bits_inst(io_debug_rb_inst_0_bits_inst);
  core.io_debug_rb_inst_0_bits_idx(io_debug_rb_inst_0_bits_idx);
  core.io_debug_rb_inst_0_bits_data(io_debug_rb_inst_0_bits_data);
  core.io_debug_rb_inst_0_bits_trap(io_debug_rb_inst_0_bits_trap);
  core.io_trace_halt(io_trace_halt);

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
  mif.reset.val = 1;
  core.eval();
  mif.eval();
  
  tb.reset = 0;
  core.reset.val = 0;
  mif.reset.val = 0;
  core.pc = entry_point;

  // Initialize ready signals to false
  core.io_ibus_ready.val = 0;
  core.io_dbus_ready.val = 0;
  bool halted = false;
  for (int i = 0; i < cycles; ++i) {
    // Negedge
    core.clock.val = 0;
    mif.clock.val = 0;
    core.eval();

    // Propagate Core -> MIF
    mif.io_ibus_valid.val = core.io_ibus_valid.val;
    mif.io_ibus_addr.val = core.io_ibus_addr.val;
    mif.io_dbus_valid.val = core.io_dbus_valid.val;
    mif.io_dbus_write.val = core.io_dbus_write.val;
    mif.io_dbus_addr.val = core.io_dbus_addr.val;
    mif.io_dbus_adrx.val = core.io_dbus_adrx.val;
    mif.io_dbus_size.val = core.io_dbus_size.val;
    mif.io_dbus_wdata.val = core.io_dbus_wdata.val;
    mif.io_dbus_wmask.val = core.io_dbus_wmask.val;

    mif.eval();

    // Propagate MIF -> Core
    core.io_ibus_ready.val = mif.io_ibus_ready.val;
    core.io_ibus_rdata.val = mif.io_ibus_rdata.val;
    core.io_ibus_fault_valid.val = mif.io_ibus_fault_valid.val;
    core.io_ibus_fault_bits_write.val = mif.io_ibus_fault_bits_write.val;
    core.io_ibus_fault_bits_addr.val = mif.io_ibus_fault_bits_addr.val;
    core.io_ibus_fault_bits_epc.val = mif.io_ibus_fault_bits_epc.val;
    core.io_dbus_ready.val = mif.io_dbus_ready.val;
    core.io_dbus_rdata.val = mif.io_dbus_rdata.val;

    // Posedge
    core.clock.val = 1;
    mif.clock.val = 1;
    core.eval();

    // Propagate Core -> MIF
    mif.io_ibus_valid.val = core.io_ibus_valid.val;
    mif.io_ibus_addr.val = core.io_ibus_addr.val;
    mif.io_dbus_valid.val = core.io_dbus_valid.val;
    mif.io_dbus_write.val = core.io_dbus_write.val;
    mif.io_dbus_addr.val = core.io_dbus_addr.val;
    mif.io_dbus_adrx.val = core.io_dbus_adrx.val;
    mif.io_dbus_size.val = core.io_dbus_size.val;
    mif.io_dbus_wdata.val = core.io_dbus_wdata.val;
    mif.io_dbus_wmask.val = core.io_dbus_wmask.val;

    mif.eval();

    // Propagate MIF -> Core
    core.io_ibus_ready.val = mif.io_ibus_ready.val;
    core.io_ibus_rdata.val = mif.io_ibus_rdata.val;
    core.io_ibus_fault_valid.val = mif.io_ibus_fault_valid.val;
    core.io_ibus_fault_bits_write.val = mif.io_ibus_fault_bits_write.val;
    core.io_ibus_fault_bits_addr.val = mif.io_ibus_fault_bits_addr.val;
    core.io_ibus_fault_bits_epc.val = mif.io_ibus_fault_bits_epc.val;
    core.io_dbus_ready.val = mif.io_dbus_ready.val;
    core.io_dbus_rdata.val = mif.io_dbus_rdata.val;

    // Manual propagation for mock systemc.h
    tb.io_debug_rb_inst_0_valid.val = core.io_debug_rb_inst_0_valid.val;
    tb.io_debug_rb_inst_0_bits_pc.val = core.io_debug_rb_inst_0_bits_pc.val;
    tb.io_debug_rb_inst_0_bits_inst.val = core.io_debug_rb_inst_0_bits_inst.val;
    tb.io_debug_rb_inst_0_bits_idx.val = core.io_debug_rb_inst_0_bits_idx.val;
    tb.io_debug_rb_inst_0_bits_data.val = core.io_debug_rb_inst_0_bits_data.val;
    tb.io_debug_rb_inst_0_bits_trap.val = core.io_debug_rb_inst_0_bits_trap.val;
    tb.io_trace_halt.val = core.io_trace_halt.val;
    tb.io_halted.val = core.io_halted.val;

    tb.posedge();
    if (tb.io_halted.val) {
      halted = true;
      break;
    }
  }

  // Wait for buffer to drain
  while(!buffer.IsEmpty()) {
    std::this_thread::yield();
  }
  daemon.Stop();

  if (halted) {
    printf("Simulation HALTED gracefully.\n");
    return 0;
  } else {
    fprintf(stderr, "Simulation TIMEOUT after %d cycles.\n", cycles);
    return 1;
  }
}

int sc_main(int argc, char *argv[]) {
  absl::SetProgramUsageMessage("CoralNPU RVVI Tracing Simulation Tool");
  auto out_args = absl::ParseCommandLine(argc, argv);
  argc = out_args.size();
  argv = &out_args[0];
  if (argc != 2) {
    fprintf(stderr, "Need one binary/ELF input file\n");
    return 1;
  }
  const char* path = argv[1];

  return CoreRvvi_run(Sysc_tb::get_name(argv[0]), path, absl::GetFlag(FLAGS_cycles),
           absl::GetFlag(FLAGS_trace), absl::GetFlag(FLAGS_rvvi_out),
           absl::GetFlag(FLAGS_memory_profile));
}

int main(int argc, char* argv[]) {
  return sc_main(argc, argv);
}
