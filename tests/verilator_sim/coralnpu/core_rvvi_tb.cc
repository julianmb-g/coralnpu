// Copyright 2026 Google LLC
// Dummy comment 2 to force Bazel recompile
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
#include <thread>
#include <iostream>
#include <fstream>
#include <limits>
#include <chrono>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "tests/verilator_sim/coralnpu/core_if.h"
#include "tests/verilator_sim/coralnpu/debug_if.h"
#include "tests/verilator_sim/coralnpu/coralnpu_cfg.h"
#include "tests/verilator_sim/sysc_tb.h"
#include "tests/verilator_sim/util.h"
#include "tests/verilator_sim/elf.h"
#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#if TRACE_ENABLED
#include <thread>
#include "tests/verilator_sim/rvvi/spsc_ring_buffer.h"
#include "tests/verilator_sim/rvvi/trace_daemon.h"
#include "tests/verilator_sim/rvvi/custom_fallback_formatter.h"
#endif

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

ABSL_FLAG(int, cycles, 500000, "Simulation cycles (Used as instruction timeout if --instructions not set)");
ABSL_FLAG(int, instructions, 500000, "Instruction timeout");
ABSL_FLAG(bool, trace, false, "Dump VCD trace");
ABSL_FLAG(std::string, rvvi_out, "trace.rvvi", "RVVI trace output file");
ABSL_FLAG(std::string, memory_profile, "default", "Memory profile ('default' or 'highmem')");

struct CoreRvvi_tb : Sysc_tb {
  sc_in<bool> io_halted;
  sc_in<bool> io_fault;
  sc_in<bool> io_ibus_valid;

  uint64_t last_time = 0;
  uint64_t last_delta = 0;

  SC_HAS_PROCESS(CoreRvvi_tb);

  
  // RVVI ports
  sc_in<bool> io_debug_rb_inst_0_valid;
  sc_in<sc_bv<KP_programCounterBits>> io_debug_rb_inst_0_bits_pc;
  sc_in<sc_bv<32>> io_debug_rb_inst_0_bits_inst;
  sc_in<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_0_bits_idx;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_data;
  sc_in<bool> io_debug_rb_inst_0_bits_trap;

  sc_in<bool> io_debug_rb_inst_0_bits_vecWrites_0_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_vecWrites_0_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_0_bits_vecWrites_0_bits_idx;
  sc_in<bool> io_debug_rb_inst_0_bits_vecWrites_1_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_vecWrites_1_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_0_bits_vecWrites_1_bits_idx;
  sc_in<bool> io_debug_rb_inst_0_bits_vecWrites_2_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_vecWrites_2_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_0_bits_vecWrites_2_bits_idx;
  sc_in<bool> io_debug_rb_inst_0_bits_vecWrites_3_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_vecWrites_3_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_0_bits_vecWrites_3_bits_idx;
  sc_in<bool> io_debug_rb_inst_0_bits_vecWrites_4_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_vecWrites_4_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_0_bits_vecWrites_4_bits_idx;
  sc_in<bool> io_debug_rb_inst_0_bits_vecWrites_5_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_vecWrites_5_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_0_bits_vecWrites_5_bits_idx;
  sc_in<bool> io_debug_rb_inst_0_bits_vecWrites_6_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_vecWrites_6_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_0_bits_vecWrites_6_bits_idx;
  sc_in<bool> io_debug_rb_inst_0_bits_vecWrites_7_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_vecWrites_7_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_0_bits_vecWrites_7_bits_idx;
sc_in<bool> io_debug_rb_inst_1_valid;
  sc_in<sc_bv<KP_programCounterBits>> io_debug_rb_inst_1_bits_pc;
  sc_in<sc_bv<32>> io_debug_rb_inst_1_bits_inst;
  sc_in<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_1_bits_idx;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_data;
  sc_in<bool> io_debug_rb_inst_1_bits_trap;

  sc_in<bool> io_debug_rb_inst_1_bits_vecWrites_0_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_vecWrites_0_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_1_bits_vecWrites_0_bits_idx;
  sc_in<bool> io_debug_rb_inst_1_bits_vecWrites_1_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_vecWrites_1_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_1_bits_vecWrites_1_bits_idx;
  sc_in<bool> io_debug_rb_inst_1_bits_vecWrites_2_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_vecWrites_2_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_1_bits_vecWrites_2_bits_idx;
  sc_in<bool> io_debug_rb_inst_1_bits_vecWrites_3_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_vecWrites_3_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_1_bits_vecWrites_3_bits_idx;
  sc_in<bool> io_debug_rb_inst_1_bits_vecWrites_4_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_vecWrites_4_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_1_bits_vecWrites_4_bits_idx;
  sc_in<bool> io_debug_rb_inst_1_bits_vecWrites_5_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_vecWrites_5_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_1_bits_vecWrites_5_bits_idx;
  sc_in<bool> io_debug_rb_inst_1_bits_vecWrites_6_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_vecWrites_6_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_1_bits_vecWrites_6_bits_idx;
  sc_in<bool> io_debug_rb_inst_1_bits_vecWrites_7_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_vecWrites_7_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_1_bits_vecWrites_7_bits_idx;
sc_in<bool> io_debug_rb_inst_2_valid;
  sc_in<sc_bv<KP_programCounterBits>> io_debug_rb_inst_2_bits_pc;
  sc_in<sc_bv<32>> io_debug_rb_inst_2_bits_inst;
  sc_in<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_2_bits_idx;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_data;
  sc_in<bool> io_debug_rb_inst_2_bits_trap;

  sc_in<bool> io_debug_rb_inst_2_bits_vecWrites_0_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_vecWrites_0_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_2_bits_vecWrites_0_bits_idx;
  sc_in<bool> io_debug_rb_inst_2_bits_vecWrites_1_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_vecWrites_1_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_2_bits_vecWrites_1_bits_idx;
  sc_in<bool> io_debug_rb_inst_2_bits_vecWrites_2_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_vecWrites_2_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_2_bits_vecWrites_2_bits_idx;
  sc_in<bool> io_debug_rb_inst_2_bits_vecWrites_3_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_vecWrites_3_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_2_bits_vecWrites_3_bits_idx;
  sc_in<bool> io_debug_rb_inst_2_bits_vecWrites_4_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_vecWrites_4_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_2_bits_vecWrites_4_bits_idx;
  sc_in<bool> io_debug_rb_inst_2_bits_vecWrites_5_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_vecWrites_5_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_2_bits_vecWrites_5_bits_idx;
  sc_in<bool> io_debug_rb_inst_2_bits_vecWrites_6_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_vecWrites_6_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_2_bits_vecWrites_6_bits_idx;
  sc_in<bool> io_debug_rb_inst_2_bits_vecWrites_7_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_vecWrites_7_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_2_bits_vecWrites_7_bits_idx;
sc_in<bool> io_debug_rb_inst_3_valid;
  sc_in<sc_bv<KP_programCounterBits>> io_debug_rb_inst_3_bits_pc;
  sc_in<sc_bv<32>> io_debug_rb_inst_3_bits_inst;
  sc_in<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_3_bits_idx;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_data;
  sc_in<bool> io_debug_rb_inst_3_bits_trap;

  sc_in<bool> io_debug_rb_inst_3_bits_vecWrites_0_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_vecWrites_0_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_3_bits_vecWrites_0_bits_idx;
  sc_in<bool> io_debug_rb_inst_3_bits_vecWrites_1_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_vecWrites_1_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_3_bits_vecWrites_1_bits_idx;
  sc_in<bool> io_debug_rb_inst_3_bits_vecWrites_2_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_vecWrites_2_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_3_bits_vecWrites_2_bits_idx;
  sc_in<bool> io_debug_rb_inst_3_bits_vecWrites_3_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_vecWrites_3_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_3_bits_vecWrites_3_bits_idx;
  sc_in<bool> io_debug_rb_inst_3_bits_vecWrites_4_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_vecWrites_4_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_3_bits_vecWrites_4_bits_idx;
  sc_in<bool> io_debug_rb_inst_3_bits_vecWrites_5_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_vecWrites_5_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_3_bits_vecWrites_5_bits_idx;
  sc_in<bool> io_debug_rb_inst_3_bits_vecWrites_6_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_vecWrites_6_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_3_bits_vecWrites_6_bits_idx;
  sc_in<bool> io_debug_rb_inst_3_bits_vecWrites_7_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_vecWrites_7_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_3_bits_vecWrites_7_bits_idx;
sc_in<bool> io_debug_rb_inst_4_valid;
  sc_in<sc_bv<KP_programCounterBits>> io_debug_rb_inst_4_bits_pc;
  sc_in<sc_bv<32>> io_debug_rb_inst_4_bits_inst;
  sc_in<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_4_bits_idx;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_data;
  sc_in<bool> io_debug_rb_inst_4_bits_trap;

  sc_in<bool> io_debug_rb_inst_4_bits_vecWrites_0_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_vecWrites_0_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_4_bits_vecWrites_0_bits_idx;
  sc_in<bool> io_debug_rb_inst_4_bits_vecWrites_1_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_vecWrites_1_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_4_bits_vecWrites_1_bits_idx;
  sc_in<bool> io_debug_rb_inst_4_bits_vecWrites_2_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_vecWrites_2_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_4_bits_vecWrites_2_bits_idx;
  sc_in<bool> io_debug_rb_inst_4_bits_vecWrites_3_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_vecWrites_3_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_4_bits_vecWrites_3_bits_idx;
  sc_in<bool> io_debug_rb_inst_4_bits_vecWrites_4_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_vecWrites_4_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_4_bits_vecWrites_4_bits_idx;
  sc_in<bool> io_debug_rb_inst_4_bits_vecWrites_5_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_vecWrites_5_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_4_bits_vecWrites_5_bits_idx;
  sc_in<bool> io_debug_rb_inst_4_bits_vecWrites_6_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_vecWrites_6_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_4_bits_vecWrites_6_bits_idx;
  sc_in<bool> io_debug_rb_inst_4_bits_vecWrites_7_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_vecWrites_7_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_4_bits_vecWrites_7_bits_idx;
sc_in<bool> io_debug_rb_inst_5_valid;
  sc_in<sc_bv<KP_programCounterBits>> io_debug_rb_inst_5_bits_pc;
  sc_in<sc_bv<32>> io_debug_rb_inst_5_bits_inst;
  sc_in<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_5_bits_idx;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_data;
  sc_in<bool> io_debug_rb_inst_5_bits_trap;

  sc_in<bool> io_debug_rb_inst_5_bits_vecWrites_0_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_vecWrites_0_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_5_bits_vecWrites_0_bits_idx;
  sc_in<bool> io_debug_rb_inst_5_bits_vecWrites_1_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_vecWrites_1_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_5_bits_vecWrites_1_bits_idx;
  sc_in<bool> io_debug_rb_inst_5_bits_vecWrites_2_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_vecWrites_2_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_5_bits_vecWrites_2_bits_idx;
  sc_in<bool> io_debug_rb_inst_5_bits_vecWrites_3_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_vecWrites_3_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_5_bits_vecWrites_3_bits_idx;
  sc_in<bool> io_debug_rb_inst_5_bits_vecWrites_4_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_vecWrites_4_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_5_bits_vecWrites_4_bits_idx;
  sc_in<bool> io_debug_rb_inst_5_bits_vecWrites_5_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_vecWrites_5_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_5_bits_vecWrites_5_bits_idx;
  sc_in<bool> io_debug_rb_inst_5_bits_vecWrites_6_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_vecWrites_6_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_5_bits_vecWrites_6_bits_idx;
  sc_in<bool> io_debug_rb_inst_5_bits_vecWrites_7_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_vecWrites_7_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_5_bits_vecWrites_7_bits_idx;
sc_in<bool> io_debug_rb_inst_6_valid;
  sc_in<sc_bv<KP_programCounterBits>> io_debug_rb_inst_6_bits_pc;
  sc_in<sc_bv<32>> io_debug_rb_inst_6_bits_inst;
  sc_in<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_6_bits_idx;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_data;
  sc_in<bool> io_debug_rb_inst_6_bits_trap;

  sc_in<bool> io_debug_rb_inst_6_bits_vecWrites_0_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_vecWrites_0_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_6_bits_vecWrites_0_bits_idx;
  sc_in<bool> io_debug_rb_inst_6_bits_vecWrites_1_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_vecWrites_1_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_6_bits_vecWrites_1_bits_idx;
  sc_in<bool> io_debug_rb_inst_6_bits_vecWrites_2_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_vecWrites_2_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_6_bits_vecWrites_2_bits_idx;
  sc_in<bool> io_debug_rb_inst_6_bits_vecWrites_3_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_vecWrites_3_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_6_bits_vecWrites_3_bits_idx;
  sc_in<bool> io_debug_rb_inst_6_bits_vecWrites_4_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_vecWrites_4_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_6_bits_vecWrites_4_bits_idx;
  sc_in<bool> io_debug_rb_inst_6_bits_vecWrites_5_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_vecWrites_5_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_6_bits_vecWrites_5_bits_idx;
  sc_in<bool> io_debug_rb_inst_6_bits_vecWrites_6_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_vecWrites_6_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_6_bits_vecWrites_6_bits_idx;
  sc_in<bool> io_debug_rb_inst_6_bits_vecWrites_7_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_vecWrites_7_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_6_bits_vecWrites_7_bits_idx;
sc_in<bool> io_debug_rb_inst_7_valid;
  sc_in<sc_bv<KP_programCounterBits>> io_debug_rb_inst_7_bits_pc;
  sc_in<sc_bv<32>> io_debug_rb_inst_7_bits_inst;
  sc_in<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_7_bits_idx;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_data;
  sc_in<bool> io_debug_rb_inst_7_bits_trap;

  sc_in<bool> io_debug_rb_inst_7_bits_vecWrites_0_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_vecWrites_0_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_7_bits_vecWrites_0_bits_idx;
  sc_in<bool> io_debug_rb_inst_7_bits_vecWrites_1_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_vecWrites_1_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_7_bits_vecWrites_1_bits_idx;
  sc_in<bool> io_debug_rb_inst_7_bits_vecWrites_2_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_vecWrites_2_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_7_bits_vecWrites_2_bits_idx;
  sc_in<bool> io_debug_rb_inst_7_bits_vecWrites_3_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_vecWrites_3_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_7_bits_vecWrites_3_bits_idx;
  sc_in<bool> io_debug_rb_inst_7_bits_vecWrites_4_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_vecWrites_4_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_7_bits_vecWrites_4_bits_idx;
  sc_in<bool> io_debug_rb_inst_7_bits_vecWrites_5_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_vecWrites_5_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_7_bits_vecWrites_5_bits_idx;
  sc_in<bool> io_debug_rb_inst_7_bits_vecWrites_6_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_vecWrites_6_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_7_bits_vecWrites_6_bits_idx;
  sc_in<bool> io_debug_rb_inst_7_bits_vecWrites_7_valid;
  sc_in<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_vecWrites_7_bits_data;
  sc_in<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_7_bits_vecWrites_7_bits_idx;


  SpscRingBuffer<TracePacket, 4096>* buffer;
  bool e_sent = false;
  bool ebreak_halt = false;
  uint32_t internal_v_id = 0;
  uint64_t instruction_count = 0;
  uint64_t instruction_limit = 500000;

  CoreRvvi_tb(sc_module_name name, int instruction_limit, bool random, SpscRingBuffer<TracePacket, 4096>* buf) 
    : Sysc_tb(name, std::numeric_limits<int>::max(), random), buffer(buf), instruction_limit(instruction_limit) {
    SC_METHOD(monitor_delta);
    sensitive << io_ibus_valid;
  }

  void monitor_delta() {
    uint64_t current_time = sc_time_stamp().value();
    uint64_t current_delta = sc_delta_count();
    if (current_time == last_time) {
        if (current_delta - last_delta > 10000) {
            fprintf(stderr, "[FATAL] Delta cycle deadlock detected! Time: %lu, Delta: %lu\n", current_time, current_delta);
            sc_stop();
            exit(1);
        }
    } else {
        last_time = current_time;
        last_delta = current_delta;
    }
  }

  void posedge() {
    
#define PROCESS_LANE(x) \
    if (io_debug_rb_inst_##x##_valid.read()) { \
      uint32_t inst = io_debug_rb_inst_##x##_bits_inst.read().to_uint(); \
      if (inst == 0x00100073) { \
        ebreak_halt = true; \
      } \
      if (io_debug_rb_inst_##x##_bits_trap.read()) { \
        TracePacket tpacket = {}; \
        tpacket.type = 'T'; \
        tpacket.v_id = internal_v_id++; \
        tpacket.inst.pc = io_debug_rb_inst_##x##_bits_pc.read().to_uint64(); \
        tpacket.inst.instruction = inst; \
        { \
          auto start = std::chrono::steady_clock::now(); \
          while (!buffer->Push(tpacket)) { \
            std::this_thread::yield(); \
            auto now = std::chrono::steady_clock::now(); \
            if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() > 5) { \
              fprintf(stderr, "[FATAL] Queue backpressure timeout (T-packet)! Watchdog triggered.\n"); \
              exit(1); \
            } \
          } \
        } \
      } else { \
        uint32_t v_id = internal_v_id++; \
      TracePacket ipacket = {}; \
      ipacket.type = 'I'; \
      ipacket.v_id = v_id; \
      ipacket.inst.pc = io_debug_rb_inst_##x##_bits_pc.read().to_uint64(); \
      ipacket.inst.instruction = inst; \
      \
      { \
        auto start = std::chrono::steady_clock::now(); \
        while (!buffer->Push(ipacket)) { \
          std::this_thread::yield(); \
          auto now = std::chrono::steady_clock::now(); \
          if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() > 5) { \
            fprintf(stderr, "[FATAL] Queue backpressure timeout (I-packet)! Watchdog triggered.\n"); \
            exit(1); \
          } \
        } \
      } \
      \
      uint32_t opcode = inst & 0x7f; \
      bool writes_rd = (opcode == 0x13) || (opcode == 0x33) || (opcode == 0x37) || \
                       (opcode == 0x17) || (opcode == 0x6f) || (opcode == 0x67) || \
                       (opcode == 0x03) || (opcode == 0x73) || (opcode == 0x57); \
      \
      if (writes_rd) { \
        uint32_t rd = (inst >> 7) & 0x1f; \
        if (rd != 0 || opcode == 0x57) { \
          int num_vec_writes = (opcode == 0x57) ? 8 : 1; \
          int sub_packets = (opcode == 0x57) ? ((KP_rvvVlen + 255) / 256) : 1; \
          for (int i = 0; i < num_vec_writes; ++i) { \
            for (int sp = 0; sp < sub_packets; ++sp) { \
              TracePacket rpacket = {}; \
              rpacket.type = 'R'; \
              rpacket.v_id = v_id; \
              rpacket.reg.reg_type = (opcode == 0x57) ? 'V' : 'X'; \
              rpacket.reg.index = rd; \
              rpacket.reg.offset = (opcode == 0x57) ? (sp * 32) : 0; \
              rpacket.reg.size = (opcode == 0x57) ? (((KP_rvvVlen / 8) - sp * 32 < 32) ? ((KP_rvvVlen / 8) - sp * 32) : 32) : (KP_xlen / 8); \
              rpacket.reg.total_size = (opcode == 0x57) ? (KP_rvvVlen / 8) : (KP_xlen / 8); \
              \
              if (opcode == 0x57) { \
                bool chunk_valid = false; \
                sc_bv<KP_rvvVlen> vval; \
                uint32_t chunk_idx = rd; \
                switch (i) { \
                  case 0: \
                    chunk_valid = io_debug_rb_inst_##x##_bits_vecWrites_0_valid.read(); \
                    if (chunk_valid) { \
                      vval = io_debug_rb_inst_##x##_bits_vecWrites_0_bits_data.read(); \
                      chunk_idx = io_debug_rb_inst_##x##_bits_vecWrites_0_bits_idx.read().to_uint(); \
                    } \
                    break; \
                  case 1: \
                    chunk_valid = io_debug_rb_inst_##x##_bits_vecWrites_1_valid.read(); \
                    if (chunk_valid) { \
                      vval = io_debug_rb_inst_##x##_bits_vecWrites_1_bits_data.read(); \
                      chunk_idx = io_debug_rb_inst_##x##_bits_vecWrites_1_bits_idx.read().to_uint(); \
                    } \
                    break; \
                  case 2: \
                    chunk_valid = io_debug_rb_inst_##x##_bits_vecWrites_2_valid.read(); \
                    if (chunk_valid) { \
                      vval = io_debug_rb_inst_##x##_bits_vecWrites_2_bits_data.read(); \
                      chunk_idx = io_debug_rb_inst_##x##_bits_vecWrites_2_bits_idx.read().to_uint(); \
                    } \
                    break; \
                  case 3: \
                    chunk_valid = io_debug_rb_inst_##x##_bits_vecWrites_3_valid.read(); \
                    if (chunk_valid) { \
                      vval = io_debug_rb_inst_##x##_bits_vecWrites_3_bits_data.read(); \
                      chunk_idx = io_debug_rb_inst_##x##_bits_vecWrites_3_bits_idx.read().to_uint(); \
                    } \
                    break; \
                  case 4: \
                    chunk_valid = io_debug_rb_inst_##x##_bits_vecWrites_4_valid.read(); \
                    if (chunk_valid) { \
                      vval = io_debug_rb_inst_##x##_bits_vecWrites_4_bits_data.read(); \
                      chunk_idx = io_debug_rb_inst_##x##_bits_vecWrites_4_bits_idx.read().to_uint(); \
                    } \
                    break; \
                  case 5: \
                    chunk_valid = io_debug_rb_inst_##x##_bits_vecWrites_5_valid.read(); \
                    if (chunk_valid) { \
                      vval = io_debug_rb_inst_##x##_bits_vecWrites_5_bits_data.read(); \
                      chunk_idx = io_debug_rb_inst_##x##_bits_vecWrites_5_bits_idx.read().to_uint(); \
                    } \
                    break; \
                  case 6: \
                    chunk_valid = io_debug_rb_inst_##x##_bits_vecWrites_6_valid.read(); \
                    if (chunk_valid) { \
                      vval = io_debug_rb_inst_##x##_bits_vecWrites_6_bits_data.read(); \
                      chunk_idx = io_debug_rb_inst_##x##_bits_vecWrites_6_bits_idx.read().to_uint(); \
                    } \
                    break; \
                  case 7: \
                    chunk_valid = io_debug_rb_inst_##x##_bits_vecWrites_7_valid.read(); \
                    if (chunk_valid) { \
                      vval = io_debug_rb_inst_##x##_bits_vecWrites_7_bits_data.read(); \
                      chunk_idx = io_debug_rb_inst_##x##_bits_vecWrites_7_bits_idx.read().to_uint(); \
                    } \
                    break; \
                } \
                if (chunk_valid) { \
                  rpacket.reg.index = chunk_idx; \
                  for (int w = 0; w < 4; ++w) { \
                    uint64_t val64 = 0; \
                    int word_idx_0 = (sp * 8) + (w * 2); \
                    int word_idx_1 = word_idx_0 + 1; \
                    if (word_idx_0 < (KP_rvvVlen / 32)) { \
                      val64 |= vval.get_word(word_idx_0); \
                    } \
                    if (word_idx_1 < (KP_rvvVlen / 32)) { \
                      val64 |= (static_cast<uint64_t>(vval.get_word(word_idx_1)) << 32); \
                    } \
                    rpacket.reg.value[w] = val64; \
                  } \
                } else { \
                  continue; \
                } \
              } else { \
                if (sp == 0) rpacket.reg.value[0] = io_debug_rb_inst_##x##_bits_data.read().to_uint64(); \
              } \
              \
              { \
                auto start = std::chrono::steady_clock::now(); \
                while (!buffer->Push(rpacket)) { \
                  std::this_thread::yield(); \
                  auto now = std::chrono::steady_clock::now(); \
                  if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() > 5) { \
                    fprintf(stderr, "[FATAL] Queue backpressure timeout (R-packet)! Watchdog triggered.\n"); \
                    exit(1); \
                  } \
                } \
              } \
            } \
          } \
        } \
      } \
      } \
    }

    PROCESS_LANE(0);
    PROCESS_LANE(1);
    PROCESS_LANE(2);
    PROCESS_LANE(3);
    PROCESS_LANE(4);
    PROCESS_LANE(5);
    PROCESS_LANE(6);
    PROCESS_LANE(7);

#undef PROCESS_LANE

    uint64_t retiring_this_cycle = 0;
    if (io_debug_rb_inst_0_valid.read()) retiring_this_cycle++;
    if (io_debug_rb_inst_1_valid.read()) retiring_this_cycle++;
    if (io_debug_rb_inst_2_valid.read()) retiring_this_cycle++;
    if (io_debug_rb_inst_3_valid.read()) retiring_this_cycle++;
    if (io_debug_rb_inst_4_valid.read()) retiring_this_cycle++;
    if (io_debug_rb_inst_5_valid.read()) retiring_this_cycle++;
    if (io_debug_rb_inst_6_valid.read()) retiring_this_cycle++;
    if (io_debug_rb_inst_7_valid.read()) retiring_this_cycle++;

    instruction_count += retiring_this_cycle;

    if (instruction_count >= instruction_limit) {
        sc_stop();
    }

    if ((io_halted.read() || io_fault.read() || ebreak_halt) && !e_sent) { \
      TracePacket epacket = {}; \
      epacket.type = 'E'; \
      while (!buffer->Push(epacket)) { \
        std::this_thread::yield(); \
      } \
      e_sent = true; \
    }

    if (io_halted.read() || ebreak_halt) {
      sc_stop();
    }
    if (!ebreak_halt) {
      check(!io_fault, "io_fault");
    }
  }
};

bool LoadElfToMemory(const std::string& file_name, Core_if& mif, uint32_t& entry_point) {
  int fd = open(file_name.c_str(), O_RDONLY);
  if (fd < 0) { perror("open"); return false; }
  struct stat sb;
  if (fstat(fd, &sb) != 0) { perror("fstat"); close(fd); return false; }
  auto file_size = sb.st_size;
  auto file_data = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (file_data == MAP_FAILED) { perror("mmap"); close(fd); return false; }
  close(fd);
  uint32_t elf_magic = 0x464c457f;
  uint8_t* data8 = reinterpret_cast<uint8_t*>(file_data);
  bool load_ok = true;
  if (memcmp(file_data, &elf_magic, sizeof(elf_magic)) == 0) {
    entry_point = ::LoadElf(data8,
              [&mif, &load_ok](void* dest, const void* src, size_t count) {
                uint64_t addr = reinterpret_cast<uint64_t>(dest);
                if (!mif.Write(addr, count, reinterpret_cast<const uint8_t*>(src))) {
                  uint64_t avail_end = (mif.profile_ == "highmem") ? 0x200000 : 0x400000;
                  uint64_t delta = (addr + count > avail_end) ? (addr + count - avail_end) : 0;
                  LOG(ERROR) << absl::StrFormat("[FATAL] ELF load violation. Requested: [0x%lx - 0x%lx]. Available: [0x0 - 0x%lx]. Delta: Exceeds bounds by 0x%lx bytes.", addr, addr + count, avail_end, delta);
                  load_ok = false;
                }
                return dest;
              });
    munmap(file_data, file_size);
    return load_ok;
  } else {
    fprintf(stderr, "Invalid ELF magic\n");
  }
  munmap(file_data, file_size);
  return false;
}

static int CoreRvvi_run(const char* name, const char* bin, const int instruction_limit,
                     const bool trace, const std::string& rvvi_out,
                     const std::string& memory_profile) {
  VERILATOR_MODEL core(name);
#if TRACE_ENABLED
  SpscRingBuffer<TracePacket, 4096> buffer;
  CoreRvvi_tb tb("CoreRvvi_tb", instruction_limit, false, &buffer);
#else
  CoreRvvi_tb tb("CoreRvvi_tb", instruction_limit, false, nullptr);
#endif
  Core_if mif("Core_if", nullptr, memory_profile);

  uint32_t entry_point = 0x80000000;
  if (!LoadElfToMemory(bin, mif, entry_point)) {
    fprintf(stderr, "Error backdoor loading ELF: %s\n", bin);
    exit(65);
  }

#if TRACE_ENABLED
  std::ofstream trace_stream(rvvi_out);
  TraceDaemon daemon(&buffer, &trace_stream);
#ifdef DELAY_FORMATTER
  DelayFormatter formatter;
#else
  CustomFallbackFormatter formatter;
#endif
  daemon.SetTraceFormatter(&formatter);
  daemon.Start();
#endif

  sc_signal<bool> io_halted;
  sc_signal<bool> io_fault;
  sc_signal<bool> io_wfi;
  sc_signal<bool> io_irq;
  sc_signal<bool> io_timer_irq;
  sc_signal<bool> io_software_irq;
  sc_signal<bool> io_debug_req;
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
  sc_signal<sc_bv<32> > io_dbus_pc;
  sc_signal<sc_bv<KP_dbusSize> > io_dbus_size;
  sc_signal<sc_bv<KP_lsuDataBits> > io_dbus_wdata;
  sc_signal<sc_bv<KP_lsuDataBits / 8> > io_dbus_wmask;
  sc_signal<sc_bv<KP_lsuDataBits> > io_dbus_rdata;

  sc_signal<bool> io_debug_rb_inst_0_valid;
  sc_signal<sc_bv<KP_programCounterBits>> io_debug_rb_inst_0_bits_pc;
  sc_signal<sc_bv<32>> io_debug_rb_inst_0_bits_inst;
  sc_signal<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_0_bits_idx;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_data;
  sc_signal<bool> io_debug_rb_inst_0_bits_trap;

  sc_signal<bool> io_debug_rb_inst_0_bits_vecWrites_0_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_vecWrites_0_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_0_bits_vecWrites_0_bits_idx;
  sc_signal<bool> io_debug_rb_inst_0_bits_vecWrites_1_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_vecWrites_1_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_0_bits_vecWrites_1_bits_idx;
  sc_signal<bool> io_debug_rb_inst_0_bits_vecWrites_2_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_vecWrites_2_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_0_bits_vecWrites_2_bits_idx;
  sc_signal<bool> io_debug_rb_inst_0_bits_vecWrites_3_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_vecWrites_3_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_0_bits_vecWrites_3_bits_idx;
  sc_signal<bool> io_debug_rb_inst_0_bits_vecWrites_4_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_vecWrites_4_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_0_bits_vecWrites_4_bits_idx;
  sc_signal<bool> io_debug_rb_inst_0_bits_vecWrites_5_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_vecWrites_5_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_0_bits_vecWrites_5_bits_idx;
  sc_signal<bool> io_debug_rb_inst_0_bits_vecWrites_6_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_vecWrites_6_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_0_bits_vecWrites_6_bits_idx;
  sc_signal<bool> io_debug_rb_inst_0_bits_vecWrites_7_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_0_bits_vecWrites_7_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_0_bits_vecWrites_7_bits_idx;
sc_signal<bool> io_debug_rb_inst_1_valid;
  sc_signal<sc_bv<KP_programCounterBits>> io_debug_rb_inst_1_bits_pc;
  sc_signal<sc_bv<32>> io_debug_rb_inst_1_bits_inst;
  sc_signal<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_1_bits_idx;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_data;
  sc_signal<bool> io_debug_rb_inst_1_bits_trap;

  sc_signal<bool> io_debug_rb_inst_1_bits_vecWrites_0_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_vecWrites_0_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_1_bits_vecWrites_0_bits_idx;
  sc_signal<bool> io_debug_rb_inst_1_bits_vecWrites_1_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_vecWrites_1_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_1_bits_vecWrites_1_bits_idx;
  sc_signal<bool> io_debug_rb_inst_1_bits_vecWrites_2_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_vecWrites_2_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_1_bits_vecWrites_2_bits_idx;
  sc_signal<bool> io_debug_rb_inst_1_bits_vecWrites_3_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_vecWrites_3_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_1_bits_vecWrites_3_bits_idx;
  sc_signal<bool> io_debug_rb_inst_1_bits_vecWrites_4_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_vecWrites_4_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_1_bits_vecWrites_4_bits_idx;
  sc_signal<bool> io_debug_rb_inst_1_bits_vecWrites_5_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_vecWrites_5_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_1_bits_vecWrites_5_bits_idx;
  sc_signal<bool> io_debug_rb_inst_1_bits_vecWrites_6_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_vecWrites_6_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_1_bits_vecWrites_6_bits_idx;
  sc_signal<bool> io_debug_rb_inst_1_bits_vecWrites_7_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_1_bits_vecWrites_7_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_1_bits_vecWrites_7_bits_idx;
sc_signal<bool> io_debug_rb_inst_2_valid;
  sc_signal<sc_bv<KP_programCounterBits>> io_debug_rb_inst_2_bits_pc;
  sc_signal<sc_bv<32>> io_debug_rb_inst_2_bits_inst;
  sc_signal<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_2_bits_idx;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_data;
  sc_signal<bool> io_debug_rb_inst_2_bits_trap;

  sc_signal<bool> io_debug_rb_inst_2_bits_vecWrites_0_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_vecWrites_0_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_2_bits_vecWrites_0_bits_idx;
  sc_signal<bool> io_debug_rb_inst_2_bits_vecWrites_1_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_vecWrites_1_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_2_bits_vecWrites_1_bits_idx;
  sc_signal<bool> io_debug_rb_inst_2_bits_vecWrites_2_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_vecWrites_2_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_2_bits_vecWrites_2_bits_idx;
  sc_signal<bool> io_debug_rb_inst_2_bits_vecWrites_3_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_vecWrites_3_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_2_bits_vecWrites_3_bits_idx;
  sc_signal<bool> io_debug_rb_inst_2_bits_vecWrites_4_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_vecWrites_4_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_2_bits_vecWrites_4_bits_idx;
  sc_signal<bool> io_debug_rb_inst_2_bits_vecWrites_5_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_vecWrites_5_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_2_bits_vecWrites_5_bits_idx;
  sc_signal<bool> io_debug_rb_inst_2_bits_vecWrites_6_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_vecWrites_6_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_2_bits_vecWrites_6_bits_idx;
  sc_signal<bool> io_debug_rb_inst_2_bits_vecWrites_7_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_2_bits_vecWrites_7_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_2_bits_vecWrites_7_bits_idx;
sc_signal<bool> io_debug_rb_inst_3_valid;
  sc_signal<sc_bv<KP_programCounterBits>> io_debug_rb_inst_3_bits_pc;
  sc_signal<sc_bv<32>> io_debug_rb_inst_3_bits_inst;
  sc_signal<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_3_bits_idx;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_data;
  sc_signal<bool> io_debug_rb_inst_3_bits_trap;

  sc_signal<bool> io_debug_rb_inst_3_bits_vecWrites_0_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_vecWrites_0_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_3_bits_vecWrites_0_bits_idx;
  sc_signal<bool> io_debug_rb_inst_3_bits_vecWrites_1_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_vecWrites_1_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_3_bits_vecWrites_1_bits_idx;
  sc_signal<bool> io_debug_rb_inst_3_bits_vecWrites_2_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_vecWrites_2_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_3_bits_vecWrites_2_bits_idx;
  sc_signal<bool> io_debug_rb_inst_3_bits_vecWrites_3_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_vecWrites_3_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_3_bits_vecWrites_3_bits_idx;
  sc_signal<bool> io_debug_rb_inst_3_bits_vecWrites_4_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_vecWrites_4_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_3_bits_vecWrites_4_bits_idx;
  sc_signal<bool> io_debug_rb_inst_3_bits_vecWrites_5_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_vecWrites_5_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_3_bits_vecWrites_5_bits_idx;
  sc_signal<bool> io_debug_rb_inst_3_bits_vecWrites_6_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_vecWrites_6_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_3_bits_vecWrites_6_bits_idx;
  sc_signal<bool> io_debug_rb_inst_3_bits_vecWrites_7_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_3_bits_vecWrites_7_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_3_bits_vecWrites_7_bits_idx;
sc_signal<bool> io_debug_rb_inst_4_valid;
  sc_signal<sc_bv<KP_programCounterBits>> io_debug_rb_inst_4_bits_pc;
  sc_signal<sc_bv<32>> io_debug_rb_inst_4_bits_inst;
  sc_signal<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_4_bits_idx;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_data;
  sc_signal<bool> io_debug_rb_inst_4_bits_trap;

  sc_signal<bool> io_debug_rb_inst_4_bits_vecWrites_0_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_vecWrites_0_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_4_bits_vecWrites_0_bits_idx;
  sc_signal<bool> io_debug_rb_inst_4_bits_vecWrites_1_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_vecWrites_1_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_4_bits_vecWrites_1_bits_idx;
  sc_signal<bool> io_debug_rb_inst_4_bits_vecWrites_2_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_vecWrites_2_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_4_bits_vecWrites_2_bits_idx;
  sc_signal<bool> io_debug_rb_inst_4_bits_vecWrites_3_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_vecWrites_3_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_4_bits_vecWrites_3_bits_idx;
  sc_signal<bool> io_debug_rb_inst_4_bits_vecWrites_4_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_vecWrites_4_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_4_bits_vecWrites_4_bits_idx;
  sc_signal<bool> io_debug_rb_inst_4_bits_vecWrites_5_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_vecWrites_5_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_4_bits_vecWrites_5_bits_idx;
  sc_signal<bool> io_debug_rb_inst_4_bits_vecWrites_6_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_vecWrites_6_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_4_bits_vecWrites_6_bits_idx;
  sc_signal<bool> io_debug_rb_inst_4_bits_vecWrites_7_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_4_bits_vecWrites_7_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_4_bits_vecWrites_7_bits_idx;
sc_signal<bool> io_debug_rb_inst_5_valid;
  sc_signal<sc_bv<KP_programCounterBits>> io_debug_rb_inst_5_bits_pc;
  sc_signal<sc_bv<32>> io_debug_rb_inst_5_bits_inst;
  sc_signal<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_5_bits_idx;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_data;
  sc_signal<bool> io_debug_rb_inst_5_bits_trap;

  sc_signal<bool> io_debug_rb_inst_5_bits_vecWrites_0_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_vecWrites_0_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_5_bits_vecWrites_0_bits_idx;
  sc_signal<bool> io_debug_rb_inst_5_bits_vecWrites_1_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_vecWrites_1_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_5_bits_vecWrites_1_bits_idx;
  sc_signal<bool> io_debug_rb_inst_5_bits_vecWrites_2_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_vecWrites_2_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_5_bits_vecWrites_2_bits_idx;
  sc_signal<bool> io_debug_rb_inst_5_bits_vecWrites_3_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_vecWrites_3_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_5_bits_vecWrites_3_bits_idx;
  sc_signal<bool> io_debug_rb_inst_5_bits_vecWrites_4_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_vecWrites_4_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_5_bits_vecWrites_4_bits_idx;
  sc_signal<bool> io_debug_rb_inst_5_bits_vecWrites_5_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_vecWrites_5_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_5_bits_vecWrites_5_bits_idx;
  sc_signal<bool> io_debug_rb_inst_5_bits_vecWrites_6_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_vecWrites_6_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_5_bits_vecWrites_6_bits_idx;
  sc_signal<bool> io_debug_rb_inst_5_bits_vecWrites_7_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_5_bits_vecWrites_7_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_5_bits_vecWrites_7_bits_idx;
sc_signal<bool> io_debug_rb_inst_6_valid;
  sc_signal<sc_bv<KP_programCounterBits>> io_debug_rb_inst_6_bits_pc;
  sc_signal<sc_bv<32>> io_debug_rb_inst_6_bits_inst;
  sc_signal<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_6_bits_idx;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_data;
  sc_signal<bool> io_debug_rb_inst_6_bits_trap;

  sc_signal<bool> io_debug_rb_inst_6_bits_vecWrites_0_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_vecWrites_0_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_6_bits_vecWrites_0_bits_idx;
  sc_signal<bool> io_debug_rb_inst_6_bits_vecWrites_1_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_vecWrites_1_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_6_bits_vecWrites_1_bits_idx;
  sc_signal<bool> io_debug_rb_inst_6_bits_vecWrites_2_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_vecWrites_2_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_6_bits_vecWrites_2_bits_idx;
  sc_signal<bool> io_debug_rb_inst_6_bits_vecWrites_3_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_vecWrites_3_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_6_bits_vecWrites_3_bits_idx;
  sc_signal<bool> io_debug_rb_inst_6_bits_vecWrites_4_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_vecWrites_4_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_6_bits_vecWrites_4_bits_idx;
  sc_signal<bool> io_debug_rb_inst_6_bits_vecWrites_5_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_vecWrites_5_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_6_bits_vecWrites_5_bits_idx;
  sc_signal<bool> io_debug_rb_inst_6_bits_vecWrites_6_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_vecWrites_6_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_6_bits_vecWrites_6_bits_idx;
  sc_signal<bool> io_debug_rb_inst_6_bits_vecWrites_7_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_6_bits_vecWrites_7_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_6_bits_vecWrites_7_bits_idx;
sc_signal<bool> io_debug_rb_inst_7_valid;
  sc_signal<sc_bv<KP_programCounterBits>> io_debug_rb_inst_7_bits_pc;
  sc_signal<sc_bv<32>> io_debug_rb_inst_7_bits_inst;
  sc_signal<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_7_bits_idx;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_data;
  sc_signal<bool> io_debug_rb_inst_7_bits_trap;

  sc_signal<bool> io_debug_rb_inst_7_bits_vecWrites_0_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_vecWrites_0_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_7_bits_vecWrites_0_bits_idx;
  sc_signal<bool> io_debug_rb_inst_7_bits_vecWrites_1_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_vecWrites_1_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_7_bits_vecWrites_1_bits_idx;
  sc_signal<bool> io_debug_rb_inst_7_bits_vecWrites_2_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_vecWrites_2_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_7_bits_vecWrites_2_bits_idx;
  sc_signal<bool> io_debug_rb_inst_7_bits_vecWrites_3_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_vecWrites_3_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_7_bits_vecWrites_3_bits_idx;
  sc_signal<bool> io_debug_rb_inst_7_bits_vecWrites_4_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_vecWrites_4_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_7_bits_vecWrites_4_bits_idx;
  sc_signal<bool> io_debug_rb_inst_7_bits_vecWrites_5_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_vecWrites_5_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_7_bits_vecWrites_5_bits_idx;
  sc_signal<bool> io_debug_rb_inst_7_bits_vecWrites_6_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_vecWrites_6_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_7_bits_vecWrites_6_bits_idx;
  sc_signal<bool> io_debug_rb_inst_7_bits_vecWrites_7_valid;
  sc_signal<sc_bv<KP_rvvVlen>> io_debug_rb_inst_7_bits_vecWrites_7_bits_data;
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_7_bits_vecWrites_7_bits_idx;

  sc_signal<bool> io_debug_float_writeAddr_valid;
  sc_signal<bool> io_debug_float_writeData_0_valid;
  sc_signal<bool> io_debug_float_writeData_1_valid;
  sc_signal<sc_bv<5>> io_debug_float_writeAddr_bits;
  sc_signal<sc_bv<5>> io_debug_float_writeData_0_bits_addr;
  sc_signal<sc_bv<32>> io_debug_float_writeData_0_bits_data;
  sc_signal<sc_bv<5>> io_debug_float_writeData_1_bits_addr;
  sc_signal<sc_bv<32>> io_debug_float_writeData_1_bits_data;

#define DECLARE_REGFILE_WRITE_ADDR(x) \
  sc_signal<bool> io_debug_regfile_writeAddr_##x##_valid; \
  sc_signal<sc_bv<5>> io_debug_regfile_writeAddr_##x##_bits;

#define DECLARE_REGFILE_WRITE_DATA(x) \
  sc_signal<bool> io_debug_regfile_writeData_##x##_valid; \
  sc_signal<sc_bv<5>> io_debug_regfile_writeData_##x##_bits_addr; \
  sc_signal<sc_bv<32>> io_debug_regfile_writeData_##x##_bits_data;

  REPEAT_4(DECLARE_REGFILE_WRITE_ADDR);
  REPEAT_6(DECLARE_REGFILE_WRITE_DATA);

#undef DECLARE_REGFILE_WRITE_ADDR
#undef DECLARE_REGFILE_WRITE_DATA

  sc_signal<sc_bv<4>> io_debug_en;
  sc_signal<sc_bv<32>> io_debug_cycles;

#define DECLARE_DEBUG_ADDR(x) sc_signal<sc_bv<32>> io_debug_addr_##x;
#define DECLARE_DEBUG_INST(x) sc_signal<sc_bv<32>> io_debug_inst_##x;

  REPEAT_4(DECLARE_DEBUG_ADDR);
  REPEAT_4(DECLARE_DEBUG_INST);

#undef DECLARE_DEBUG_ADDR
#undef DECLARE_DEBUG_INST

  sc_signal<bool> io_debug_dbus_valid;
  sc_signal<sc_bv<32>> io_debug_dbus_bits_addr;
  sc_signal<sc_bv<128>> io_debug_dbus_bits_wdata;
  sc_signal<bool> io_debug_dbus_bits_write;

#define DECLARE_DEBUG_DISPATCH(x) \
  sc_signal<bool> io_debug_dispatch_##x##_instFire; \
  sc_signal<sc_bv<32>> io_debug_dispatch_##x##_instAddr; \
  sc_signal<sc_bv<32>> io_debug_dispatch_##x##_instInst;

  REPEAT_4(DECLARE_DEBUG_DISPATCH);
#undef DECLARE_DEBUG_DISPATCH

  sc_signal<bool> io_iflush_valid;
  sc_signal<sc_bv<KP_programCounterBits> > io_iflush_pcNext;
  sc_signal<bool> io_iflush_ready;
  sc_signal<bool> io_dflush_valid;
  sc_signal<bool> io_dflush_ready;
  sc_signal<bool> io_dflush_all;
  sc_signal<bool> io_dflush_clean;

  sc_signal<bool> io_ebus_dbus_valid;
  sc_signal<bool> io_ebus_dbus_ready;
  sc_signal<bool> io_ebus_dbus_write;
  sc_signal<bool> io_ebus_internal;
  sc_signal<bool> io_ebus_fault_valid;
  sc_signal<bool> io_ebus_fault_bits_write;
  sc_signal<sc_bv<32>> io_ebus_dbus_pc;
  sc_signal<sc_bv<32>> io_ebus_dbus_addr;
  sc_signal<sc_bv<32>> io_ebus_dbus_adrx;
  sc_signal<sc_bv<5>> io_ebus_dbus_size;
  sc_signal<sc_bv<128>> io_ebus_dbus_wdata;
  sc_signal<sc_bv<16>> io_ebus_dbus_wmask;
  sc_signal<sc_bv<128>> io_ebus_dbus_rdata;
  sc_signal<sc_bv<32>> io_ebus_fault_bits_addr;
  sc_signal<sc_bv<32>> io_ebus_fault_bits_epc;

  sc_signal<bool> io_dm_debug_req;
  sc_signal<bool> io_dm_resume_req;
  sc_signal<bool> io_dm_csr_valid;
  sc_signal<bool> io_dm_csr_rd_valid;
  sc_signal<bool> io_dm_scalar_rd_ready;
  sc_signal<bool> io_dm_scalar_rd_valid;
  sc_signal<bool> io_dm_float_rd_valid;
  sc_signal<bool> io_dm_float_rd_data_sign;
  sc_signal<bool> io_dm_float_rs_valid;
  sc_signal<bool> io_dm_float_rs_data_sign;
  sc_signal<bool> io_dm_debug_mode;
  sc_signal<sc_bv<5>> io_dm_csr_bits_addr;
  sc_signal<sc_bv<12>> io_dm_csr_bits_index;
  sc_signal<sc_bv<5>> io_dm_csr_bits_rs1;
  sc_signal<sc_bv<2>> io_dm_csr_bits_op;
  sc_signal<sc_bv<32>> io_dm_csr_rs1;
  sc_signal<sc_bv<32>> io_dm_csr_rd_bits;
  sc_signal<sc_bv<5>> io_dm_scalar_rd_bits_addr;
  sc_signal<sc_bv<32>> io_dm_scalar_rd_bits_data;
  sc_signal<sc_bv<5>> io_dm_scalar_rs_idx;
  sc_signal<sc_bv<32>> io_dm_scalar_rs_data;
  sc_signal<sc_bv<5>> io_dm_float_rd_addr;
  sc_signal<sc_bv<23>> io_dm_float_rd_data_mantissa;
  sc_signal<sc_bv<8>> io_dm_float_rd_data_exponent;
  sc_signal<sc_bv<5>> io_dm_float_rs_addr;
  sc_signal<sc_bv<23>> io_dm_float_rs_data_mantissa;
  sc_signal<sc_bv<8>> io_dm_float_rs_data_exponent;

#define DECLARE_CSR_OUT(x) sc_signal<sc_bv<32>> io_csr_out_value_##x;
  REPEAT_8(DECLARE_CSR_OUT);
  DECLARE_CSR_OUT(8);
#undef DECLARE_CSR_OUT

#define DECLARE_CSR_IN(x) sc_signal<sc_bv<32>> io_csr_in_value_##x;
  REPEAT_13(DECLARE_CSR_IN);
#undef DECLARE_CSR_IN


  tb.io_halted(io_halted);
  tb.io_fault(io_fault);
  tb.io_ibus_valid(io_ibus_valid);
  tb.io_debug_rb_inst_0_valid(io_debug_rb_inst_0_valid);
  tb.io_debug_rb_inst_0_bits_pc(io_debug_rb_inst_0_bits_pc);
  tb.io_debug_rb_inst_0_bits_inst(io_debug_rb_inst_0_bits_inst);
  tb.io_debug_rb_inst_0_bits_idx(io_debug_rb_inst_0_bits_idx);
  tb.io_debug_rb_inst_0_bits_data(io_debug_rb_inst_0_bits_data);
  tb.io_debug_rb_inst_0_bits_trap(io_debug_rb_inst_0_bits_trap);

  tb.io_debug_rb_inst_0_bits_vecWrites_0_valid(io_debug_rb_inst_0_bits_vecWrites_0_valid);
  tb.io_debug_rb_inst_0_bits_vecWrites_0_bits_data(io_debug_rb_inst_0_bits_vecWrites_0_bits_data);
  tb.io_debug_rb_inst_0_bits_vecWrites_0_bits_idx(io_debug_rb_inst_0_bits_vecWrites_0_bits_idx);
  tb.io_debug_rb_inst_0_bits_vecWrites_1_valid(io_debug_rb_inst_0_bits_vecWrites_1_valid);
  tb.io_debug_rb_inst_0_bits_vecWrites_1_bits_data(io_debug_rb_inst_0_bits_vecWrites_1_bits_data);
  tb.io_debug_rb_inst_0_bits_vecWrites_1_bits_idx(io_debug_rb_inst_0_bits_vecWrites_1_bits_idx);
  tb.io_debug_rb_inst_0_bits_vecWrites_2_valid(io_debug_rb_inst_0_bits_vecWrites_2_valid);
  tb.io_debug_rb_inst_0_bits_vecWrites_2_bits_data(io_debug_rb_inst_0_bits_vecWrites_2_bits_data);
  tb.io_debug_rb_inst_0_bits_vecWrites_2_bits_idx(io_debug_rb_inst_0_bits_vecWrites_2_bits_idx);
  tb.io_debug_rb_inst_0_bits_vecWrites_3_valid(io_debug_rb_inst_0_bits_vecWrites_3_valid);
  tb.io_debug_rb_inst_0_bits_vecWrites_3_bits_data(io_debug_rb_inst_0_bits_vecWrites_3_bits_data);
  tb.io_debug_rb_inst_0_bits_vecWrites_3_bits_idx(io_debug_rb_inst_0_bits_vecWrites_3_bits_idx);
  tb.io_debug_rb_inst_0_bits_vecWrites_4_valid(io_debug_rb_inst_0_bits_vecWrites_4_valid);
  tb.io_debug_rb_inst_0_bits_vecWrites_4_bits_data(io_debug_rb_inst_0_bits_vecWrites_4_bits_data);
  tb.io_debug_rb_inst_0_bits_vecWrites_4_bits_idx(io_debug_rb_inst_0_bits_vecWrites_4_bits_idx);
  tb.io_debug_rb_inst_0_bits_vecWrites_5_valid(io_debug_rb_inst_0_bits_vecWrites_5_valid);
  tb.io_debug_rb_inst_0_bits_vecWrites_5_bits_data(io_debug_rb_inst_0_bits_vecWrites_5_bits_data);
  tb.io_debug_rb_inst_0_bits_vecWrites_5_bits_idx(io_debug_rb_inst_0_bits_vecWrites_5_bits_idx);
  tb.io_debug_rb_inst_0_bits_vecWrites_6_valid(io_debug_rb_inst_0_bits_vecWrites_6_valid);
  tb.io_debug_rb_inst_0_bits_vecWrites_6_bits_data(io_debug_rb_inst_0_bits_vecWrites_6_bits_data);
  tb.io_debug_rb_inst_0_bits_vecWrites_6_bits_idx(io_debug_rb_inst_0_bits_vecWrites_6_bits_idx);
  tb.io_debug_rb_inst_0_bits_vecWrites_7_valid(io_debug_rb_inst_0_bits_vecWrites_7_valid);
  tb.io_debug_rb_inst_0_bits_vecWrites_7_bits_data(io_debug_rb_inst_0_bits_vecWrites_7_bits_data);
  tb.io_debug_rb_inst_0_bits_vecWrites_7_bits_idx(io_debug_rb_inst_0_bits_vecWrites_7_bits_idx);
tb.io_debug_rb_inst_1_valid(io_debug_rb_inst_1_valid);
  tb.io_debug_rb_inst_1_bits_pc(io_debug_rb_inst_1_bits_pc);
  tb.io_debug_rb_inst_1_bits_inst(io_debug_rb_inst_1_bits_inst);
  tb.io_debug_rb_inst_1_bits_idx(io_debug_rb_inst_1_bits_idx);
  tb.io_debug_rb_inst_1_bits_data(io_debug_rb_inst_1_bits_data);
  tb.io_debug_rb_inst_1_bits_trap(io_debug_rb_inst_1_bits_trap);

  tb.io_debug_rb_inst_1_bits_vecWrites_0_valid(io_debug_rb_inst_1_bits_vecWrites_0_valid);
  tb.io_debug_rb_inst_1_bits_vecWrites_0_bits_data(io_debug_rb_inst_1_bits_vecWrites_0_bits_data);
  tb.io_debug_rb_inst_1_bits_vecWrites_0_bits_idx(io_debug_rb_inst_1_bits_vecWrites_0_bits_idx);
  tb.io_debug_rb_inst_1_bits_vecWrites_1_valid(io_debug_rb_inst_1_bits_vecWrites_1_valid);
  tb.io_debug_rb_inst_1_bits_vecWrites_1_bits_data(io_debug_rb_inst_1_bits_vecWrites_1_bits_data);
  tb.io_debug_rb_inst_1_bits_vecWrites_1_bits_idx(io_debug_rb_inst_1_bits_vecWrites_1_bits_idx);
  tb.io_debug_rb_inst_1_bits_vecWrites_2_valid(io_debug_rb_inst_1_bits_vecWrites_2_valid);
  tb.io_debug_rb_inst_1_bits_vecWrites_2_bits_data(io_debug_rb_inst_1_bits_vecWrites_2_bits_data);
  tb.io_debug_rb_inst_1_bits_vecWrites_2_bits_idx(io_debug_rb_inst_1_bits_vecWrites_2_bits_idx);
  tb.io_debug_rb_inst_1_bits_vecWrites_3_valid(io_debug_rb_inst_1_bits_vecWrites_3_valid);
  tb.io_debug_rb_inst_1_bits_vecWrites_3_bits_data(io_debug_rb_inst_1_bits_vecWrites_3_bits_data);
  tb.io_debug_rb_inst_1_bits_vecWrites_3_bits_idx(io_debug_rb_inst_1_bits_vecWrites_3_bits_idx);
  tb.io_debug_rb_inst_1_bits_vecWrites_4_valid(io_debug_rb_inst_1_bits_vecWrites_4_valid);
  tb.io_debug_rb_inst_1_bits_vecWrites_4_bits_data(io_debug_rb_inst_1_bits_vecWrites_4_bits_data);
  tb.io_debug_rb_inst_1_bits_vecWrites_4_bits_idx(io_debug_rb_inst_1_bits_vecWrites_4_bits_idx);
  tb.io_debug_rb_inst_1_bits_vecWrites_5_valid(io_debug_rb_inst_1_bits_vecWrites_5_valid);
  tb.io_debug_rb_inst_1_bits_vecWrites_5_bits_data(io_debug_rb_inst_1_bits_vecWrites_5_bits_data);
  tb.io_debug_rb_inst_1_bits_vecWrites_5_bits_idx(io_debug_rb_inst_1_bits_vecWrites_5_bits_idx);
  tb.io_debug_rb_inst_1_bits_vecWrites_6_valid(io_debug_rb_inst_1_bits_vecWrites_6_valid);
  tb.io_debug_rb_inst_1_bits_vecWrites_6_bits_data(io_debug_rb_inst_1_bits_vecWrites_6_bits_data);
  tb.io_debug_rb_inst_1_bits_vecWrites_6_bits_idx(io_debug_rb_inst_1_bits_vecWrites_6_bits_idx);
  tb.io_debug_rb_inst_1_bits_vecWrites_7_valid(io_debug_rb_inst_1_bits_vecWrites_7_valid);
  tb.io_debug_rb_inst_1_bits_vecWrites_7_bits_data(io_debug_rb_inst_1_bits_vecWrites_7_bits_data);
  tb.io_debug_rb_inst_1_bits_vecWrites_7_bits_idx(io_debug_rb_inst_1_bits_vecWrites_7_bits_idx);
tb.io_debug_rb_inst_2_valid(io_debug_rb_inst_2_valid);
  tb.io_debug_rb_inst_2_bits_pc(io_debug_rb_inst_2_bits_pc);
  tb.io_debug_rb_inst_2_bits_inst(io_debug_rb_inst_2_bits_inst);
  tb.io_debug_rb_inst_2_bits_idx(io_debug_rb_inst_2_bits_idx);
  tb.io_debug_rb_inst_2_bits_data(io_debug_rb_inst_2_bits_data);
  tb.io_debug_rb_inst_2_bits_trap(io_debug_rb_inst_2_bits_trap);

  tb.io_debug_rb_inst_2_bits_vecWrites_0_valid(io_debug_rb_inst_2_bits_vecWrites_0_valid);
  tb.io_debug_rb_inst_2_bits_vecWrites_0_bits_data(io_debug_rb_inst_2_bits_vecWrites_0_bits_data);
  tb.io_debug_rb_inst_2_bits_vecWrites_0_bits_idx(io_debug_rb_inst_2_bits_vecWrites_0_bits_idx);
  tb.io_debug_rb_inst_2_bits_vecWrites_1_valid(io_debug_rb_inst_2_bits_vecWrites_1_valid);
  tb.io_debug_rb_inst_2_bits_vecWrites_1_bits_data(io_debug_rb_inst_2_bits_vecWrites_1_bits_data);
  tb.io_debug_rb_inst_2_bits_vecWrites_1_bits_idx(io_debug_rb_inst_2_bits_vecWrites_1_bits_idx);
  tb.io_debug_rb_inst_2_bits_vecWrites_2_valid(io_debug_rb_inst_2_bits_vecWrites_2_valid);
  tb.io_debug_rb_inst_2_bits_vecWrites_2_bits_data(io_debug_rb_inst_2_bits_vecWrites_2_bits_data);
  tb.io_debug_rb_inst_2_bits_vecWrites_2_bits_idx(io_debug_rb_inst_2_bits_vecWrites_2_bits_idx);
  tb.io_debug_rb_inst_2_bits_vecWrites_3_valid(io_debug_rb_inst_2_bits_vecWrites_3_valid);
  tb.io_debug_rb_inst_2_bits_vecWrites_3_bits_data(io_debug_rb_inst_2_bits_vecWrites_3_bits_data);
  tb.io_debug_rb_inst_2_bits_vecWrites_3_bits_idx(io_debug_rb_inst_2_bits_vecWrites_3_bits_idx);
  tb.io_debug_rb_inst_2_bits_vecWrites_4_valid(io_debug_rb_inst_2_bits_vecWrites_4_valid);
  tb.io_debug_rb_inst_2_bits_vecWrites_4_bits_data(io_debug_rb_inst_2_bits_vecWrites_4_bits_data);
  tb.io_debug_rb_inst_2_bits_vecWrites_4_bits_idx(io_debug_rb_inst_2_bits_vecWrites_4_bits_idx);
  tb.io_debug_rb_inst_2_bits_vecWrites_5_valid(io_debug_rb_inst_2_bits_vecWrites_5_valid);
  tb.io_debug_rb_inst_2_bits_vecWrites_5_bits_data(io_debug_rb_inst_2_bits_vecWrites_5_bits_data);
  tb.io_debug_rb_inst_2_bits_vecWrites_5_bits_idx(io_debug_rb_inst_2_bits_vecWrites_5_bits_idx);
  tb.io_debug_rb_inst_2_bits_vecWrites_6_valid(io_debug_rb_inst_2_bits_vecWrites_6_valid);
  tb.io_debug_rb_inst_2_bits_vecWrites_6_bits_data(io_debug_rb_inst_2_bits_vecWrites_6_bits_data);
  tb.io_debug_rb_inst_2_bits_vecWrites_6_bits_idx(io_debug_rb_inst_2_bits_vecWrites_6_bits_idx);
  tb.io_debug_rb_inst_2_bits_vecWrites_7_valid(io_debug_rb_inst_2_bits_vecWrites_7_valid);
  tb.io_debug_rb_inst_2_bits_vecWrites_7_bits_data(io_debug_rb_inst_2_bits_vecWrites_7_bits_data);
  tb.io_debug_rb_inst_2_bits_vecWrites_7_bits_idx(io_debug_rb_inst_2_bits_vecWrites_7_bits_idx);
tb.io_debug_rb_inst_3_valid(io_debug_rb_inst_3_valid);
  tb.io_debug_rb_inst_3_bits_pc(io_debug_rb_inst_3_bits_pc);
  tb.io_debug_rb_inst_3_bits_inst(io_debug_rb_inst_3_bits_inst);
  tb.io_debug_rb_inst_3_bits_idx(io_debug_rb_inst_3_bits_idx);
  tb.io_debug_rb_inst_3_bits_data(io_debug_rb_inst_3_bits_data);
  tb.io_debug_rb_inst_3_bits_trap(io_debug_rb_inst_3_bits_trap);

  tb.io_debug_rb_inst_3_bits_vecWrites_0_valid(io_debug_rb_inst_3_bits_vecWrites_0_valid);
  tb.io_debug_rb_inst_3_bits_vecWrites_0_bits_data(io_debug_rb_inst_3_bits_vecWrites_0_bits_data);
  tb.io_debug_rb_inst_3_bits_vecWrites_0_bits_idx(io_debug_rb_inst_3_bits_vecWrites_0_bits_idx);
  tb.io_debug_rb_inst_3_bits_vecWrites_1_valid(io_debug_rb_inst_3_bits_vecWrites_1_valid);
  tb.io_debug_rb_inst_3_bits_vecWrites_1_bits_data(io_debug_rb_inst_3_bits_vecWrites_1_bits_data);
  tb.io_debug_rb_inst_3_bits_vecWrites_1_bits_idx(io_debug_rb_inst_3_bits_vecWrites_1_bits_idx);
  tb.io_debug_rb_inst_3_bits_vecWrites_2_valid(io_debug_rb_inst_3_bits_vecWrites_2_valid);
  tb.io_debug_rb_inst_3_bits_vecWrites_2_bits_data(io_debug_rb_inst_3_bits_vecWrites_2_bits_data);
  tb.io_debug_rb_inst_3_bits_vecWrites_2_bits_idx(io_debug_rb_inst_3_bits_vecWrites_2_bits_idx);
  tb.io_debug_rb_inst_3_bits_vecWrites_3_valid(io_debug_rb_inst_3_bits_vecWrites_3_valid);
  tb.io_debug_rb_inst_3_bits_vecWrites_3_bits_data(io_debug_rb_inst_3_bits_vecWrites_3_bits_data);
  tb.io_debug_rb_inst_3_bits_vecWrites_3_bits_idx(io_debug_rb_inst_3_bits_vecWrites_3_bits_idx);
  tb.io_debug_rb_inst_3_bits_vecWrites_4_valid(io_debug_rb_inst_3_bits_vecWrites_4_valid);
  tb.io_debug_rb_inst_3_bits_vecWrites_4_bits_data(io_debug_rb_inst_3_bits_vecWrites_4_bits_data);
  tb.io_debug_rb_inst_3_bits_vecWrites_4_bits_idx(io_debug_rb_inst_3_bits_vecWrites_4_bits_idx);
  tb.io_debug_rb_inst_3_bits_vecWrites_5_valid(io_debug_rb_inst_3_bits_vecWrites_5_valid);
  tb.io_debug_rb_inst_3_bits_vecWrites_5_bits_data(io_debug_rb_inst_3_bits_vecWrites_5_bits_data);
  tb.io_debug_rb_inst_3_bits_vecWrites_5_bits_idx(io_debug_rb_inst_3_bits_vecWrites_5_bits_idx);
  tb.io_debug_rb_inst_3_bits_vecWrites_6_valid(io_debug_rb_inst_3_bits_vecWrites_6_valid);
  tb.io_debug_rb_inst_3_bits_vecWrites_6_bits_data(io_debug_rb_inst_3_bits_vecWrites_6_bits_data);
  tb.io_debug_rb_inst_3_bits_vecWrites_6_bits_idx(io_debug_rb_inst_3_bits_vecWrites_6_bits_idx);
  tb.io_debug_rb_inst_3_bits_vecWrites_7_valid(io_debug_rb_inst_3_bits_vecWrites_7_valid);
  tb.io_debug_rb_inst_3_bits_vecWrites_7_bits_data(io_debug_rb_inst_3_bits_vecWrites_7_bits_data);
  tb.io_debug_rb_inst_3_bits_vecWrites_7_bits_idx(io_debug_rb_inst_3_bits_vecWrites_7_bits_idx);
tb.io_debug_rb_inst_4_valid(io_debug_rb_inst_4_valid);
  tb.io_debug_rb_inst_4_bits_pc(io_debug_rb_inst_4_bits_pc);
  tb.io_debug_rb_inst_4_bits_inst(io_debug_rb_inst_4_bits_inst);
  tb.io_debug_rb_inst_4_bits_idx(io_debug_rb_inst_4_bits_idx);
  tb.io_debug_rb_inst_4_bits_data(io_debug_rb_inst_4_bits_data);
  tb.io_debug_rb_inst_4_bits_trap(io_debug_rb_inst_4_bits_trap);

  tb.io_debug_rb_inst_4_bits_vecWrites_0_valid(io_debug_rb_inst_4_bits_vecWrites_0_valid);
  tb.io_debug_rb_inst_4_bits_vecWrites_0_bits_data(io_debug_rb_inst_4_bits_vecWrites_0_bits_data);
  tb.io_debug_rb_inst_4_bits_vecWrites_0_bits_idx(io_debug_rb_inst_4_bits_vecWrites_0_bits_idx);
  tb.io_debug_rb_inst_4_bits_vecWrites_1_valid(io_debug_rb_inst_4_bits_vecWrites_1_valid);
  tb.io_debug_rb_inst_4_bits_vecWrites_1_bits_data(io_debug_rb_inst_4_bits_vecWrites_1_bits_data);
  tb.io_debug_rb_inst_4_bits_vecWrites_1_bits_idx(io_debug_rb_inst_4_bits_vecWrites_1_bits_idx);
  tb.io_debug_rb_inst_4_bits_vecWrites_2_valid(io_debug_rb_inst_4_bits_vecWrites_2_valid);
  tb.io_debug_rb_inst_4_bits_vecWrites_2_bits_data(io_debug_rb_inst_4_bits_vecWrites_2_bits_data);
  tb.io_debug_rb_inst_4_bits_vecWrites_2_bits_idx(io_debug_rb_inst_4_bits_vecWrites_2_bits_idx);
  tb.io_debug_rb_inst_4_bits_vecWrites_3_valid(io_debug_rb_inst_4_bits_vecWrites_3_valid);
  tb.io_debug_rb_inst_4_bits_vecWrites_3_bits_data(io_debug_rb_inst_4_bits_vecWrites_3_bits_data);
  tb.io_debug_rb_inst_4_bits_vecWrites_3_bits_idx(io_debug_rb_inst_4_bits_vecWrites_3_bits_idx);
  tb.io_debug_rb_inst_4_bits_vecWrites_4_valid(io_debug_rb_inst_4_bits_vecWrites_4_valid);
  tb.io_debug_rb_inst_4_bits_vecWrites_4_bits_data(io_debug_rb_inst_4_bits_vecWrites_4_bits_data);
  tb.io_debug_rb_inst_4_bits_vecWrites_4_bits_idx(io_debug_rb_inst_4_bits_vecWrites_4_bits_idx);
  tb.io_debug_rb_inst_4_bits_vecWrites_5_valid(io_debug_rb_inst_4_bits_vecWrites_5_valid);
  tb.io_debug_rb_inst_4_bits_vecWrites_5_bits_data(io_debug_rb_inst_4_bits_vecWrites_5_bits_data);
  tb.io_debug_rb_inst_4_bits_vecWrites_5_bits_idx(io_debug_rb_inst_4_bits_vecWrites_5_bits_idx);
  tb.io_debug_rb_inst_4_bits_vecWrites_6_valid(io_debug_rb_inst_4_bits_vecWrites_6_valid);
  tb.io_debug_rb_inst_4_bits_vecWrites_6_bits_data(io_debug_rb_inst_4_bits_vecWrites_6_bits_data);
  tb.io_debug_rb_inst_4_bits_vecWrites_6_bits_idx(io_debug_rb_inst_4_bits_vecWrites_6_bits_idx);
  tb.io_debug_rb_inst_4_bits_vecWrites_7_valid(io_debug_rb_inst_4_bits_vecWrites_7_valid);
  tb.io_debug_rb_inst_4_bits_vecWrites_7_bits_data(io_debug_rb_inst_4_bits_vecWrites_7_bits_data);
  tb.io_debug_rb_inst_4_bits_vecWrites_7_bits_idx(io_debug_rb_inst_4_bits_vecWrites_7_bits_idx);
tb.io_debug_rb_inst_5_valid(io_debug_rb_inst_5_valid);
  tb.io_debug_rb_inst_5_bits_pc(io_debug_rb_inst_5_bits_pc);
  tb.io_debug_rb_inst_5_bits_inst(io_debug_rb_inst_5_bits_inst);
  tb.io_debug_rb_inst_5_bits_idx(io_debug_rb_inst_5_bits_idx);
  tb.io_debug_rb_inst_5_bits_data(io_debug_rb_inst_5_bits_data);
  tb.io_debug_rb_inst_5_bits_trap(io_debug_rb_inst_5_bits_trap);

  tb.io_debug_rb_inst_5_bits_vecWrites_0_valid(io_debug_rb_inst_5_bits_vecWrites_0_valid);
  tb.io_debug_rb_inst_5_bits_vecWrites_0_bits_data(io_debug_rb_inst_5_bits_vecWrites_0_bits_data);
  tb.io_debug_rb_inst_5_bits_vecWrites_0_bits_idx(io_debug_rb_inst_5_bits_vecWrites_0_bits_idx);
  tb.io_debug_rb_inst_5_bits_vecWrites_1_valid(io_debug_rb_inst_5_bits_vecWrites_1_valid);
  tb.io_debug_rb_inst_5_bits_vecWrites_1_bits_data(io_debug_rb_inst_5_bits_vecWrites_1_bits_data);
  tb.io_debug_rb_inst_5_bits_vecWrites_1_bits_idx(io_debug_rb_inst_5_bits_vecWrites_1_bits_idx);
  tb.io_debug_rb_inst_5_bits_vecWrites_2_valid(io_debug_rb_inst_5_bits_vecWrites_2_valid);
  tb.io_debug_rb_inst_5_bits_vecWrites_2_bits_data(io_debug_rb_inst_5_bits_vecWrites_2_bits_data);
  tb.io_debug_rb_inst_5_bits_vecWrites_2_bits_idx(io_debug_rb_inst_5_bits_vecWrites_2_bits_idx);
  tb.io_debug_rb_inst_5_bits_vecWrites_3_valid(io_debug_rb_inst_5_bits_vecWrites_3_valid);
  tb.io_debug_rb_inst_5_bits_vecWrites_3_bits_data(io_debug_rb_inst_5_bits_vecWrites_3_bits_data);
  tb.io_debug_rb_inst_5_bits_vecWrites_3_bits_idx(io_debug_rb_inst_5_bits_vecWrites_3_bits_idx);
  tb.io_debug_rb_inst_5_bits_vecWrites_4_valid(io_debug_rb_inst_5_bits_vecWrites_4_valid);
  tb.io_debug_rb_inst_5_bits_vecWrites_4_bits_data(io_debug_rb_inst_5_bits_vecWrites_4_bits_data);
  tb.io_debug_rb_inst_5_bits_vecWrites_4_bits_idx(io_debug_rb_inst_5_bits_vecWrites_4_bits_idx);
  tb.io_debug_rb_inst_5_bits_vecWrites_5_valid(io_debug_rb_inst_5_bits_vecWrites_5_valid);
  tb.io_debug_rb_inst_5_bits_vecWrites_5_bits_data(io_debug_rb_inst_5_bits_vecWrites_5_bits_data);
  tb.io_debug_rb_inst_5_bits_vecWrites_5_bits_idx(io_debug_rb_inst_5_bits_vecWrites_5_bits_idx);
  tb.io_debug_rb_inst_5_bits_vecWrites_6_valid(io_debug_rb_inst_5_bits_vecWrites_6_valid);
  tb.io_debug_rb_inst_5_bits_vecWrites_6_bits_data(io_debug_rb_inst_5_bits_vecWrites_6_bits_data);
  tb.io_debug_rb_inst_5_bits_vecWrites_6_bits_idx(io_debug_rb_inst_5_bits_vecWrites_6_bits_idx);
  tb.io_debug_rb_inst_5_bits_vecWrites_7_valid(io_debug_rb_inst_5_bits_vecWrites_7_valid);
  tb.io_debug_rb_inst_5_bits_vecWrites_7_bits_data(io_debug_rb_inst_5_bits_vecWrites_7_bits_data);
  tb.io_debug_rb_inst_5_bits_vecWrites_7_bits_idx(io_debug_rb_inst_5_bits_vecWrites_7_bits_idx);
tb.io_debug_rb_inst_6_valid(io_debug_rb_inst_6_valid);
  tb.io_debug_rb_inst_6_bits_pc(io_debug_rb_inst_6_bits_pc);
  tb.io_debug_rb_inst_6_bits_inst(io_debug_rb_inst_6_bits_inst);
  tb.io_debug_rb_inst_6_bits_idx(io_debug_rb_inst_6_bits_idx);
  tb.io_debug_rb_inst_6_bits_data(io_debug_rb_inst_6_bits_data);
  tb.io_debug_rb_inst_6_bits_trap(io_debug_rb_inst_6_bits_trap);

  tb.io_debug_rb_inst_6_bits_vecWrites_0_valid(io_debug_rb_inst_6_bits_vecWrites_0_valid);
  tb.io_debug_rb_inst_6_bits_vecWrites_0_bits_data(io_debug_rb_inst_6_bits_vecWrites_0_bits_data);
  tb.io_debug_rb_inst_6_bits_vecWrites_0_bits_idx(io_debug_rb_inst_6_bits_vecWrites_0_bits_idx);
  tb.io_debug_rb_inst_6_bits_vecWrites_1_valid(io_debug_rb_inst_6_bits_vecWrites_1_valid);
  tb.io_debug_rb_inst_6_bits_vecWrites_1_bits_data(io_debug_rb_inst_6_bits_vecWrites_1_bits_data);
  tb.io_debug_rb_inst_6_bits_vecWrites_1_bits_idx(io_debug_rb_inst_6_bits_vecWrites_1_bits_idx);
  tb.io_debug_rb_inst_6_bits_vecWrites_2_valid(io_debug_rb_inst_6_bits_vecWrites_2_valid);
  tb.io_debug_rb_inst_6_bits_vecWrites_2_bits_data(io_debug_rb_inst_6_bits_vecWrites_2_bits_data);
  tb.io_debug_rb_inst_6_bits_vecWrites_2_bits_idx(io_debug_rb_inst_6_bits_vecWrites_2_bits_idx);
  tb.io_debug_rb_inst_6_bits_vecWrites_3_valid(io_debug_rb_inst_6_bits_vecWrites_3_valid);
  tb.io_debug_rb_inst_6_bits_vecWrites_3_bits_data(io_debug_rb_inst_6_bits_vecWrites_3_bits_data);
  tb.io_debug_rb_inst_6_bits_vecWrites_3_bits_idx(io_debug_rb_inst_6_bits_vecWrites_3_bits_idx);
  tb.io_debug_rb_inst_6_bits_vecWrites_4_valid(io_debug_rb_inst_6_bits_vecWrites_4_valid);
  tb.io_debug_rb_inst_6_bits_vecWrites_4_bits_data(io_debug_rb_inst_6_bits_vecWrites_4_bits_data);
  tb.io_debug_rb_inst_6_bits_vecWrites_4_bits_idx(io_debug_rb_inst_6_bits_vecWrites_4_bits_idx);
  tb.io_debug_rb_inst_6_bits_vecWrites_5_valid(io_debug_rb_inst_6_bits_vecWrites_5_valid);
  tb.io_debug_rb_inst_6_bits_vecWrites_5_bits_data(io_debug_rb_inst_6_bits_vecWrites_5_bits_data);
  tb.io_debug_rb_inst_6_bits_vecWrites_5_bits_idx(io_debug_rb_inst_6_bits_vecWrites_5_bits_idx);
  tb.io_debug_rb_inst_6_bits_vecWrites_6_valid(io_debug_rb_inst_6_bits_vecWrites_6_valid);
  tb.io_debug_rb_inst_6_bits_vecWrites_6_bits_data(io_debug_rb_inst_6_bits_vecWrites_6_bits_data);
  tb.io_debug_rb_inst_6_bits_vecWrites_6_bits_idx(io_debug_rb_inst_6_bits_vecWrites_6_bits_idx);
  tb.io_debug_rb_inst_6_bits_vecWrites_7_valid(io_debug_rb_inst_6_bits_vecWrites_7_valid);
  tb.io_debug_rb_inst_6_bits_vecWrites_7_bits_data(io_debug_rb_inst_6_bits_vecWrites_7_bits_data);
  tb.io_debug_rb_inst_6_bits_vecWrites_7_bits_idx(io_debug_rb_inst_6_bits_vecWrites_7_bits_idx);
tb.io_debug_rb_inst_7_valid(io_debug_rb_inst_7_valid);
  tb.io_debug_rb_inst_7_bits_pc(io_debug_rb_inst_7_bits_pc);
  tb.io_debug_rb_inst_7_bits_inst(io_debug_rb_inst_7_bits_inst);
  tb.io_debug_rb_inst_7_bits_idx(io_debug_rb_inst_7_bits_idx);
  tb.io_debug_rb_inst_7_bits_data(io_debug_rb_inst_7_bits_data);
  tb.io_debug_rb_inst_7_bits_trap(io_debug_rb_inst_7_bits_trap);

  tb.io_debug_rb_inst_7_bits_vecWrites_0_valid(io_debug_rb_inst_7_bits_vecWrites_0_valid);
  tb.io_debug_rb_inst_7_bits_vecWrites_0_bits_data(io_debug_rb_inst_7_bits_vecWrites_0_bits_data);
  tb.io_debug_rb_inst_7_bits_vecWrites_0_bits_idx(io_debug_rb_inst_7_bits_vecWrites_0_bits_idx);
  tb.io_debug_rb_inst_7_bits_vecWrites_1_valid(io_debug_rb_inst_7_bits_vecWrites_1_valid);
  tb.io_debug_rb_inst_7_bits_vecWrites_1_bits_data(io_debug_rb_inst_7_bits_vecWrites_1_bits_data);
  tb.io_debug_rb_inst_7_bits_vecWrites_1_bits_idx(io_debug_rb_inst_7_bits_vecWrites_1_bits_idx);
  tb.io_debug_rb_inst_7_bits_vecWrites_2_valid(io_debug_rb_inst_7_bits_vecWrites_2_valid);
  tb.io_debug_rb_inst_7_bits_vecWrites_2_bits_data(io_debug_rb_inst_7_bits_vecWrites_2_bits_data);
  tb.io_debug_rb_inst_7_bits_vecWrites_2_bits_idx(io_debug_rb_inst_7_bits_vecWrites_2_bits_idx);
  tb.io_debug_rb_inst_7_bits_vecWrites_3_valid(io_debug_rb_inst_7_bits_vecWrites_3_valid);
  tb.io_debug_rb_inst_7_bits_vecWrites_3_bits_data(io_debug_rb_inst_7_bits_vecWrites_3_bits_data);
  tb.io_debug_rb_inst_7_bits_vecWrites_3_bits_idx(io_debug_rb_inst_7_bits_vecWrites_3_bits_idx);
  tb.io_debug_rb_inst_7_bits_vecWrites_4_valid(io_debug_rb_inst_7_bits_vecWrites_4_valid);
  tb.io_debug_rb_inst_7_bits_vecWrites_4_bits_data(io_debug_rb_inst_7_bits_vecWrites_4_bits_data);
  tb.io_debug_rb_inst_7_bits_vecWrites_4_bits_idx(io_debug_rb_inst_7_bits_vecWrites_4_bits_idx);
  tb.io_debug_rb_inst_7_bits_vecWrites_5_valid(io_debug_rb_inst_7_bits_vecWrites_5_valid);
  tb.io_debug_rb_inst_7_bits_vecWrites_5_bits_data(io_debug_rb_inst_7_bits_vecWrites_5_bits_data);
  tb.io_debug_rb_inst_7_bits_vecWrites_5_bits_idx(io_debug_rb_inst_7_bits_vecWrites_5_bits_idx);
  tb.io_debug_rb_inst_7_bits_vecWrites_6_valid(io_debug_rb_inst_7_bits_vecWrites_6_valid);
  tb.io_debug_rb_inst_7_bits_vecWrites_6_bits_data(io_debug_rb_inst_7_bits_vecWrites_6_bits_data);
  tb.io_debug_rb_inst_7_bits_vecWrites_6_bits_idx(io_debug_rb_inst_7_bits_vecWrites_6_bits_idx);
  tb.io_debug_rb_inst_7_bits_vecWrites_7_valid(io_debug_rb_inst_7_bits_vecWrites_7_valid);
  tb.io_debug_rb_inst_7_bits_vecWrites_7_bits_data(io_debug_rb_inst_7_bits_vecWrites_7_bits_data);
  tb.io_debug_rb_inst_7_bits_vecWrites_7_bits_idx(io_debug_rb_inst_7_bits_vecWrites_7_bits_idx);


  core.clock(tb.clock);
  core.reset(tb.reset);
  core.io_halted(io_halted);
  core.io_fault(io_fault);
  core.io_wfi(io_wfi);
  core.io_irq(io_irq);
  core.io_timer_irq(io_timer_irq);
  core.io_software_irq(io_software_irq);
  core.io_debug_req(io_debug_req);
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
  core.io_dbus_pc(io_dbus_pc);
  
  core.io_debug_rb_inst_0_valid(io_debug_rb_inst_0_valid);
  core.io_debug_rb_inst_0_bits_pc(io_debug_rb_inst_0_bits_pc);
  core.io_debug_rb_inst_0_bits_inst(io_debug_rb_inst_0_bits_inst);
  core.io_debug_rb_inst_0_bits_idx(io_debug_rb_inst_0_bits_idx);
  core.io_debug_rb_inst_0_bits_data(io_debug_rb_inst_0_bits_data);
  core.io_debug_rb_inst_0_bits_trap(io_debug_rb_inst_0_bits_trap);
  core.io_debug_rb_inst_0_bits_vecWrites_0_valid(io_debug_rb_inst_0_bits_vecWrites_0_valid);
  core.io_debug_rb_inst_0_bits_vecWrites_0_bits_data(io_debug_rb_inst_0_bits_vecWrites_0_bits_data);
  core.io_debug_rb_inst_0_bits_vecWrites_0_bits_idx(io_debug_rb_inst_0_bits_vecWrites_0_bits_idx);
  core.io_debug_rb_inst_0_bits_vecWrites_1_valid(io_debug_rb_inst_0_bits_vecWrites_1_valid);
  core.io_debug_rb_inst_0_bits_vecWrites_1_bits_data(io_debug_rb_inst_0_bits_vecWrites_1_bits_data);
  core.io_debug_rb_inst_0_bits_vecWrites_1_bits_idx(io_debug_rb_inst_0_bits_vecWrites_1_bits_idx);
  core.io_debug_rb_inst_0_bits_vecWrites_2_valid(io_debug_rb_inst_0_bits_vecWrites_2_valid);
  core.io_debug_rb_inst_0_bits_vecWrites_2_bits_data(io_debug_rb_inst_0_bits_vecWrites_2_bits_data);
  core.io_debug_rb_inst_0_bits_vecWrites_2_bits_idx(io_debug_rb_inst_0_bits_vecWrites_2_bits_idx);
  core.io_debug_rb_inst_0_bits_vecWrites_3_valid(io_debug_rb_inst_0_bits_vecWrites_3_valid);
  core.io_debug_rb_inst_0_bits_vecWrites_3_bits_data(io_debug_rb_inst_0_bits_vecWrites_3_bits_data);
  core.io_debug_rb_inst_0_bits_vecWrites_3_bits_idx(io_debug_rb_inst_0_bits_vecWrites_3_bits_idx);
  core.io_debug_rb_inst_0_bits_vecWrites_4_valid(io_debug_rb_inst_0_bits_vecWrites_4_valid);
  core.io_debug_rb_inst_0_bits_vecWrites_4_bits_data(io_debug_rb_inst_0_bits_vecWrites_4_bits_data);
  core.io_debug_rb_inst_0_bits_vecWrites_4_bits_idx(io_debug_rb_inst_0_bits_vecWrites_4_bits_idx);
  core.io_debug_rb_inst_0_bits_vecWrites_5_valid(io_debug_rb_inst_0_bits_vecWrites_5_valid);
  core.io_debug_rb_inst_0_bits_vecWrites_5_bits_data(io_debug_rb_inst_0_bits_vecWrites_5_bits_data);
  core.io_debug_rb_inst_0_bits_vecWrites_5_bits_idx(io_debug_rb_inst_0_bits_vecWrites_5_bits_idx);
  core.io_debug_rb_inst_0_bits_vecWrites_6_valid(io_debug_rb_inst_0_bits_vecWrites_6_valid);
  core.io_debug_rb_inst_0_bits_vecWrites_6_bits_data(io_debug_rb_inst_0_bits_vecWrites_6_bits_data);
  core.io_debug_rb_inst_0_bits_vecWrites_6_bits_idx(io_debug_rb_inst_0_bits_vecWrites_6_bits_idx);
  core.io_debug_rb_inst_0_bits_vecWrites_7_valid(io_debug_rb_inst_0_bits_vecWrites_7_valid);
  core.io_debug_rb_inst_0_bits_vecWrites_7_bits_data(io_debug_rb_inst_0_bits_vecWrites_7_bits_data);
  core.io_debug_rb_inst_0_bits_vecWrites_7_bits_idx(io_debug_rb_inst_0_bits_vecWrites_7_bits_idx);
core.io_debug_rb_inst_1_valid(io_debug_rb_inst_1_valid);
  core.io_debug_rb_inst_1_bits_pc(io_debug_rb_inst_1_bits_pc);
  core.io_debug_rb_inst_1_bits_inst(io_debug_rb_inst_1_bits_inst);
  core.io_debug_rb_inst_1_bits_idx(io_debug_rb_inst_1_bits_idx);
  core.io_debug_rb_inst_1_bits_data(io_debug_rb_inst_1_bits_data);
  core.io_debug_rb_inst_1_bits_trap(io_debug_rb_inst_1_bits_trap);
  core.io_debug_rb_inst_1_bits_vecWrites_0_valid(io_debug_rb_inst_1_bits_vecWrites_0_valid);
  core.io_debug_rb_inst_1_bits_vecWrites_0_bits_data(io_debug_rb_inst_1_bits_vecWrites_0_bits_data);
  core.io_debug_rb_inst_1_bits_vecWrites_0_bits_idx(io_debug_rb_inst_1_bits_vecWrites_0_bits_idx);
  core.io_debug_rb_inst_1_bits_vecWrites_1_valid(io_debug_rb_inst_1_bits_vecWrites_1_valid);
  core.io_debug_rb_inst_1_bits_vecWrites_1_bits_data(io_debug_rb_inst_1_bits_vecWrites_1_bits_data);
  core.io_debug_rb_inst_1_bits_vecWrites_1_bits_idx(io_debug_rb_inst_1_bits_vecWrites_1_bits_idx);
  core.io_debug_rb_inst_1_bits_vecWrites_2_valid(io_debug_rb_inst_1_bits_vecWrites_2_valid);
  core.io_debug_rb_inst_1_bits_vecWrites_2_bits_data(io_debug_rb_inst_1_bits_vecWrites_2_bits_data);
  core.io_debug_rb_inst_1_bits_vecWrites_2_bits_idx(io_debug_rb_inst_1_bits_vecWrites_2_bits_idx);
  core.io_debug_rb_inst_1_bits_vecWrites_3_valid(io_debug_rb_inst_1_bits_vecWrites_3_valid);
  core.io_debug_rb_inst_1_bits_vecWrites_3_bits_data(io_debug_rb_inst_1_bits_vecWrites_3_bits_data);
  core.io_debug_rb_inst_1_bits_vecWrites_3_bits_idx(io_debug_rb_inst_1_bits_vecWrites_3_bits_idx);
  core.io_debug_rb_inst_1_bits_vecWrites_4_valid(io_debug_rb_inst_1_bits_vecWrites_4_valid);
  core.io_debug_rb_inst_1_bits_vecWrites_4_bits_data(io_debug_rb_inst_1_bits_vecWrites_4_bits_data);
  core.io_debug_rb_inst_1_bits_vecWrites_4_bits_idx(io_debug_rb_inst_1_bits_vecWrites_4_bits_idx);
  core.io_debug_rb_inst_1_bits_vecWrites_5_valid(io_debug_rb_inst_1_bits_vecWrites_5_valid);
  core.io_debug_rb_inst_1_bits_vecWrites_5_bits_data(io_debug_rb_inst_1_bits_vecWrites_5_bits_data);
  core.io_debug_rb_inst_1_bits_vecWrites_5_bits_idx(io_debug_rb_inst_1_bits_vecWrites_5_bits_idx);
  core.io_debug_rb_inst_1_bits_vecWrites_6_valid(io_debug_rb_inst_1_bits_vecWrites_6_valid);
  core.io_debug_rb_inst_1_bits_vecWrites_6_bits_data(io_debug_rb_inst_1_bits_vecWrites_6_bits_data);
  core.io_debug_rb_inst_1_bits_vecWrites_6_bits_idx(io_debug_rb_inst_1_bits_vecWrites_6_bits_idx);
  core.io_debug_rb_inst_1_bits_vecWrites_7_valid(io_debug_rb_inst_1_bits_vecWrites_7_valid);
  core.io_debug_rb_inst_1_bits_vecWrites_7_bits_data(io_debug_rb_inst_1_bits_vecWrites_7_bits_data);
  core.io_debug_rb_inst_1_bits_vecWrites_7_bits_idx(io_debug_rb_inst_1_bits_vecWrites_7_bits_idx);
core.io_debug_rb_inst_2_valid(io_debug_rb_inst_2_valid);
  core.io_debug_rb_inst_2_bits_pc(io_debug_rb_inst_2_bits_pc);
  core.io_debug_rb_inst_2_bits_inst(io_debug_rb_inst_2_bits_inst);
  core.io_debug_rb_inst_2_bits_idx(io_debug_rb_inst_2_bits_idx);
  core.io_debug_rb_inst_2_bits_data(io_debug_rb_inst_2_bits_data);
  core.io_debug_rb_inst_2_bits_trap(io_debug_rb_inst_2_bits_trap);
  core.io_debug_rb_inst_2_bits_vecWrites_0_valid(io_debug_rb_inst_2_bits_vecWrites_0_valid);
  core.io_debug_rb_inst_2_bits_vecWrites_0_bits_data(io_debug_rb_inst_2_bits_vecWrites_0_bits_data);
  core.io_debug_rb_inst_2_bits_vecWrites_0_bits_idx(io_debug_rb_inst_2_bits_vecWrites_0_bits_idx);
  core.io_debug_rb_inst_2_bits_vecWrites_1_valid(io_debug_rb_inst_2_bits_vecWrites_1_valid);
  core.io_debug_rb_inst_2_bits_vecWrites_1_bits_data(io_debug_rb_inst_2_bits_vecWrites_1_bits_data);
  core.io_debug_rb_inst_2_bits_vecWrites_1_bits_idx(io_debug_rb_inst_2_bits_vecWrites_1_bits_idx);
  core.io_debug_rb_inst_2_bits_vecWrites_2_valid(io_debug_rb_inst_2_bits_vecWrites_2_valid);
  core.io_debug_rb_inst_2_bits_vecWrites_2_bits_data(io_debug_rb_inst_2_bits_vecWrites_2_bits_data);
  core.io_debug_rb_inst_2_bits_vecWrites_2_bits_idx(io_debug_rb_inst_2_bits_vecWrites_2_bits_idx);
  core.io_debug_rb_inst_2_bits_vecWrites_3_valid(io_debug_rb_inst_2_bits_vecWrites_3_valid);
  core.io_debug_rb_inst_2_bits_vecWrites_3_bits_data(io_debug_rb_inst_2_bits_vecWrites_3_bits_data);
  core.io_debug_rb_inst_2_bits_vecWrites_3_bits_idx(io_debug_rb_inst_2_bits_vecWrites_3_bits_idx);
  core.io_debug_rb_inst_2_bits_vecWrites_4_valid(io_debug_rb_inst_2_bits_vecWrites_4_valid);
  core.io_debug_rb_inst_2_bits_vecWrites_4_bits_data(io_debug_rb_inst_2_bits_vecWrites_4_bits_data);
  core.io_debug_rb_inst_2_bits_vecWrites_4_bits_idx(io_debug_rb_inst_2_bits_vecWrites_4_bits_idx);
  core.io_debug_rb_inst_2_bits_vecWrites_5_valid(io_debug_rb_inst_2_bits_vecWrites_5_valid);
  core.io_debug_rb_inst_2_bits_vecWrites_5_bits_data(io_debug_rb_inst_2_bits_vecWrites_5_bits_data);
  core.io_debug_rb_inst_2_bits_vecWrites_5_bits_idx(io_debug_rb_inst_2_bits_vecWrites_5_bits_idx);
  core.io_debug_rb_inst_2_bits_vecWrites_6_valid(io_debug_rb_inst_2_bits_vecWrites_6_valid);
  core.io_debug_rb_inst_2_bits_vecWrites_6_bits_data(io_debug_rb_inst_2_bits_vecWrites_6_bits_data);
  core.io_debug_rb_inst_2_bits_vecWrites_6_bits_idx(io_debug_rb_inst_2_bits_vecWrites_6_bits_idx);
  core.io_debug_rb_inst_2_bits_vecWrites_7_valid(io_debug_rb_inst_2_bits_vecWrites_7_valid);
  core.io_debug_rb_inst_2_bits_vecWrites_7_bits_data(io_debug_rb_inst_2_bits_vecWrites_7_bits_data);
  core.io_debug_rb_inst_2_bits_vecWrites_7_bits_idx(io_debug_rb_inst_2_bits_vecWrites_7_bits_idx);
core.io_debug_rb_inst_3_valid(io_debug_rb_inst_3_valid);
  core.io_debug_rb_inst_3_bits_pc(io_debug_rb_inst_3_bits_pc);
  core.io_debug_rb_inst_3_bits_inst(io_debug_rb_inst_3_bits_inst);
  core.io_debug_rb_inst_3_bits_idx(io_debug_rb_inst_3_bits_idx);
  core.io_debug_rb_inst_3_bits_data(io_debug_rb_inst_3_bits_data);
  core.io_debug_rb_inst_3_bits_trap(io_debug_rb_inst_3_bits_trap);
  core.io_debug_rb_inst_3_bits_vecWrites_0_valid(io_debug_rb_inst_3_bits_vecWrites_0_valid);
  core.io_debug_rb_inst_3_bits_vecWrites_0_bits_data(io_debug_rb_inst_3_bits_vecWrites_0_bits_data);
  core.io_debug_rb_inst_3_bits_vecWrites_0_bits_idx(io_debug_rb_inst_3_bits_vecWrites_0_bits_idx);
  core.io_debug_rb_inst_3_bits_vecWrites_1_valid(io_debug_rb_inst_3_bits_vecWrites_1_valid);
  core.io_debug_rb_inst_3_bits_vecWrites_1_bits_data(io_debug_rb_inst_3_bits_vecWrites_1_bits_data);
  core.io_debug_rb_inst_3_bits_vecWrites_1_bits_idx(io_debug_rb_inst_3_bits_vecWrites_1_bits_idx);
  core.io_debug_rb_inst_3_bits_vecWrites_2_valid(io_debug_rb_inst_3_bits_vecWrites_2_valid);
  core.io_debug_rb_inst_3_bits_vecWrites_2_bits_data(io_debug_rb_inst_3_bits_vecWrites_2_bits_data);
  core.io_debug_rb_inst_3_bits_vecWrites_2_bits_idx(io_debug_rb_inst_3_bits_vecWrites_2_bits_idx);
  core.io_debug_rb_inst_3_bits_vecWrites_3_valid(io_debug_rb_inst_3_bits_vecWrites_3_valid);
  core.io_debug_rb_inst_3_bits_vecWrites_3_bits_data(io_debug_rb_inst_3_bits_vecWrites_3_bits_data);
  core.io_debug_rb_inst_3_bits_vecWrites_3_bits_idx(io_debug_rb_inst_3_bits_vecWrites_3_bits_idx);
  core.io_debug_rb_inst_3_bits_vecWrites_4_valid(io_debug_rb_inst_3_bits_vecWrites_4_valid);
  core.io_debug_rb_inst_3_bits_vecWrites_4_bits_data(io_debug_rb_inst_3_bits_vecWrites_4_bits_data);
  core.io_debug_rb_inst_3_bits_vecWrites_4_bits_idx(io_debug_rb_inst_3_bits_vecWrites_4_bits_idx);
  core.io_debug_rb_inst_3_bits_vecWrites_5_valid(io_debug_rb_inst_3_bits_vecWrites_5_valid);
  core.io_debug_rb_inst_3_bits_vecWrites_5_bits_data(io_debug_rb_inst_3_bits_vecWrites_5_bits_data);
  core.io_debug_rb_inst_3_bits_vecWrites_5_bits_idx(io_debug_rb_inst_3_bits_vecWrites_5_bits_idx);
  core.io_debug_rb_inst_3_bits_vecWrites_6_valid(io_debug_rb_inst_3_bits_vecWrites_6_valid);
  core.io_debug_rb_inst_3_bits_vecWrites_6_bits_data(io_debug_rb_inst_3_bits_vecWrites_6_bits_data);
  core.io_debug_rb_inst_3_bits_vecWrites_6_bits_idx(io_debug_rb_inst_3_bits_vecWrites_6_bits_idx);
  core.io_debug_rb_inst_3_bits_vecWrites_7_valid(io_debug_rb_inst_3_bits_vecWrites_7_valid);
  core.io_debug_rb_inst_3_bits_vecWrites_7_bits_data(io_debug_rb_inst_3_bits_vecWrites_7_bits_data);
  core.io_debug_rb_inst_3_bits_vecWrites_7_bits_idx(io_debug_rb_inst_3_bits_vecWrites_7_bits_idx);
core.io_debug_rb_inst_4_valid(io_debug_rb_inst_4_valid);
  core.io_debug_rb_inst_4_bits_pc(io_debug_rb_inst_4_bits_pc);
  core.io_debug_rb_inst_4_bits_inst(io_debug_rb_inst_4_bits_inst);
  core.io_debug_rb_inst_4_bits_idx(io_debug_rb_inst_4_bits_idx);
  core.io_debug_rb_inst_4_bits_data(io_debug_rb_inst_4_bits_data);
  core.io_debug_rb_inst_4_bits_trap(io_debug_rb_inst_4_bits_trap);
  core.io_debug_rb_inst_4_bits_vecWrites_0_valid(io_debug_rb_inst_4_bits_vecWrites_0_valid);
  core.io_debug_rb_inst_4_bits_vecWrites_0_bits_data(io_debug_rb_inst_4_bits_vecWrites_0_bits_data);
  core.io_debug_rb_inst_4_bits_vecWrites_0_bits_idx(io_debug_rb_inst_4_bits_vecWrites_0_bits_idx);
  core.io_debug_rb_inst_4_bits_vecWrites_1_valid(io_debug_rb_inst_4_bits_vecWrites_1_valid);
  core.io_debug_rb_inst_4_bits_vecWrites_1_bits_data(io_debug_rb_inst_4_bits_vecWrites_1_bits_data);
  core.io_debug_rb_inst_4_bits_vecWrites_1_bits_idx(io_debug_rb_inst_4_bits_vecWrites_1_bits_idx);
  core.io_debug_rb_inst_4_bits_vecWrites_2_valid(io_debug_rb_inst_4_bits_vecWrites_2_valid);
  core.io_debug_rb_inst_4_bits_vecWrites_2_bits_data(io_debug_rb_inst_4_bits_vecWrites_2_bits_data);
  core.io_debug_rb_inst_4_bits_vecWrites_2_bits_idx(io_debug_rb_inst_4_bits_vecWrites_2_bits_idx);
  core.io_debug_rb_inst_4_bits_vecWrites_3_valid(io_debug_rb_inst_4_bits_vecWrites_3_valid);
  core.io_debug_rb_inst_4_bits_vecWrites_3_bits_data(io_debug_rb_inst_4_bits_vecWrites_3_bits_data);
  core.io_debug_rb_inst_4_bits_vecWrites_3_bits_idx(io_debug_rb_inst_4_bits_vecWrites_3_bits_idx);
  core.io_debug_rb_inst_4_bits_vecWrites_4_valid(io_debug_rb_inst_4_bits_vecWrites_4_valid);
  core.io_debug_rb_inst_4_bits_vecWrites_4_bits_data(io_debug_rb_inst_4_bits_vecWrites_4_bits_data);
  core.io_debug_rb_inst_4_bits_vecWrites_4_bits_idx(io_debug_rb_inst_4_bits_vecWrites_4_bits_idx);
  core.io_debug_rb_inst_4_bits_vecWrites_5_valid(io_debug_rb_inst_4_bits_vecWrites_5_valid);
  core.io_debug_rb_inst_4_bits_vecWrites_5_bits_data(io_debug_rb_inst_4_bits_vecWrites_5_bits_data);
  core.io_debug_rb_inst_4_bits_vecWrites_5_bits_idx(io_debug_rb_inst_4_bits_vecWrites_5_bits_idx);
  core.io_debug_rb_inst_4_bits_vecWrites_6_valid(io_debug_rb_inst_4_bits_vecWrites_6_valid);
  core.io_debug_rb_inst_4_bits_vecWrites_6_bits_data(io_debug_rb_inst_4_bits_vecWrites_6_bits_data);
  core.io_debug_rb_inst_4_bits_vecWrites_6_bits_idx(io_debug_rb_inst_4_bits_vecWrites_6_bits_idx);
  core.io_debug_rb_inst_4_bits_vecWrites_7_valid(io_debug_rb_inst_4_bits_vecWrites_7_valid);
  core.io_debug_rb_inst_4_bits_vecWrites_7_bits_data(io_debug_rb_inst_4_bits_vecWrites_7_bits_data);
  core.io_debug_rb_inst_4_bits_vecWrites_7_bits_idx(io_debug_rb_inst_4_bits_vecWrites_7_bits_idx);
core.io_debug_rb_inst_5_valid(io_debug_rb_inst_5_valid);
  core.io_debug_rb_inst_5_bits_pc(io_debug_rb_inst_5_bits_pc);
  core.io_debug_rb_inst_5_bits_inst(io_debug_rb_inst_5_bits_inst);
  core.io_debug_rb_inst_5_bits_idx(io_debug_rb_inst_5_bits_idx);
  core.io_debug_rb_inst_5_bits_data(io_debug_rb_inst_5_bits_data);
  core.io_debug_rb_inst_5_bits_trap(io_debug_rb_inst_5_bits_trap);
  core.io_debug_rb_inst_5_bits_vecWrites_0_valid(io_debug_rb_inst_5_bits_vecWrites_0_valid);
  core.io_debug_rb_inst_5_bits_vecWrites_0_bits_data(io_debug_rb_inst_5_bits_vecWrites_0_bits_data);
  core.io_debug_rb_inst_5_bits_vecWrites_0_bits_idx(io_debug_rb_inst_5_bits_vecWrites_0_bits_idx);
  core.io_debug_rb_inst_5_bits_vecWrites_1_valid(io_debug_rb_inst_5_bits_vecWrites_1_valid);
  core.io_debug_rb_inst_5_bits_vecWrites_1_bits_data(io_debug_rb_inst_5_bits_vecWrites_1_bits_data);
  core.io_debug_rb_inst_5_bits_vecWrites_1_bits_idx(io_debug_rb_inst_5_bits_vecWrites_1_bits_idx);
  core.io_debug_rb_inst_5_bits_vecWrites_2_valid(io_debug_rb_inst_5_bits_vecWrites_2_valid);
  core.io_debug_rb_inst_5_bits_vecWrites_2_bits_data(io_debug_rb_inst_5_bits_vecWrites_2_bits_data);
  core.io_debug_rb_inst_5_bits_vecWrites_2_bits_idx(io_debug_rb_inst_5_bits_vecWrites_2_bits_idx);
  core.io_debug_rb_inst_5_bits_vecWrites_3_valid(io_debug_rb_inst_5_bits_vecWrites_3_valid);
  core.io_debug_rb_inst_5_bits_vecWrites_3_bits_data(io_debug_rb_inst_5_bits_vecWrites_3_bits_data);
  core.io_debug_rb_inst_5_bits_vecWrites_3_bits_idx(io_debug_rb_inst_5_bits_vecWrites_3_bits_idx);
  core.io_debug_rb_inst_5_bits_vecWrites_4_valid(io_debug_rb_inst_5_bits_vecWrites_4_valid);
  core.io_debug_rb_inst_5_bits_vecWrites_4_bits_data(io_debug_rb_inst_5_bits_vecWrites_4_bits_data);
  core.io_debug_rb_inst_5_bits_vecWrites_4_bits_idx(io_debug_rb_inst_5_bits_vecWrites_4_bits_idx);
  core.io_debug_rb_inst_5_bits_vecWrites_5_valid(io_debug_rb_inst_5_bits_vecWrites_5_valid);
  core.io_debug_rb_inst_5_bits_vecWrites_5_bits_data(io_debug_rb_inst_5_bits_vecWrites_5_bits_data);
  core.io_debug_rb_inst_5_bits_vecWrites_5_bits_idx(io_debug_rb_inst_5_bits_vecWrites_5_bits_idx);
  core.io_debug_rb_inst_5_bits_vecWrites_6_valid(io_debug_rb_inst_5_bits_vecWrites_6_valid);
  core.io_debug_rb_inst_5_bits_vecWrites_6_bits_data(io_debug_rb_inst_5_bits_vecWrites_6_bits_data);
  core.io_debug_rb_inst_5_bits_vecWrites_6_bits_idx(io_debug_rb_inst_5_bits_vecWrites_6_bits_idx);
  core.io_debug_rb_inst_5_bits_vecWrites_7_valid(io_debug_rb_inst_5_bits_vecWrites_7_valid);
  core.io_debug_rb_inst_5_bits_vecWrites_7_bits_data(io_debug_rb_inst_5_bits_vecWrites_7_bits_data);
  core.io_debug_rb_inst_5_bits_vecWrites_7_bits_idx(io_debug_rb_inst_5_bits_vecWrites_7_bits_idx);
core.io_debug_rb_inst_6_valid(io_debug_rb_inst_6_valid);
  core.io_debug_rb_inst_6_bits_pc(io_debug_rb_inst_6_bits_pc);
  core.io_debug_rb_inst_6_bits_inst(io_debug_rb_inst_6_bits_inst);
  core.io_debug_rb_inst_6_bits_idx(io_debug_rb_inst_6_bits_idx);
  core.io_debug_rb_inst_6_bits_data(io_debug_rb_inst_6_bits_data);
  core.io_debug_rb_inst_6_bits_trap(io_debug_rb_inst_6_bits_trap);
  core.io_debug_rb_inst_6_bits_vecWrites_0_valid(io_debug_rb_inst_6_bits_vecWrites_0_valid);
  core.io_debug_rb_inst_6_bits_vecWrites_0_bits_data(io_debug_rb_inst_6_bits_vecWrites_0_bits_data);
  core.io_debug_rb_inst_6_bits_vecWrites_0_bits_idx(io_debug_rb_inst_6_bits_vecWrites_0_bits_idx);
  core.io_debug_rb_inst_6_bits_vecWrites_1_valid(io_debug_rb_inst_6_bits_vecWrites_1_valid);
  core.io_debug_rb_inst_6_bits_vecWrites_1_bits_data(io_debug_rb_inst_6_bits_vecWrites_1_bits_data);
  core.io_debug_rb_inst_6_bits_vecWrites_1_bits_idx(io_debug_rb_inst_6_bits_vecWrites_1_bits_idx);
  core.io_debug_rb_inst_6_bits_vecWrites_2_valid(io_debug_rb_inst_6_bits_vecWrites_2_valid);
  core.io_debug_rb_inst_6_bits_vecWrites_2_bits_data(io_debug_rb_inst_6_bits_vecWrites_2_bits_data);
  core.io_debug_rb_inst_6_bits_vecWrites_2_bits_idx(io_debug_rb_inst_6_bits_vecWrites_2_bits_idx);
  core.io_debug_rb_inst_6_bits_vecWrites_3_valid(io_debug_rb_inst_6_bits_vecWrites_3_valid);
  core.io_debug_rb_inst_6_bits_vecWrites_3_bits_data(io_debug_rb_inst_6_bits_vecWrites_3_bits_data);
  core.io_debug_rb_inst_6_bits_vecWrites_3_bits_idx(io_debug_rb_inst_6_bits_vecWrites_3_bits_idx);
  core.io_debug_rb_inst_6_bits_vecWrites_4_valid(io_debug_rb_inst_6_bits_vecWrites_4_valid);
  core.io_debug_rb_inst_6_bits_vecWrites_4_bits_data(io_debug_rb_inst_6_bits_vecWrites_4_bits_data);
  core.io_debug_rb_inst_6_bits_vecWrites_4_bits_idx(io_debug_rb_inst_6_bits_vecWrites_4_bits_idx);
  core.io_debug_rb_inst_6_bits_vecWrites_5_valid(io_debug_rb_inst_6_bits_vecWrites_5_valid);
  core.io_debug_rb_inst_6_bits_vecWrites_5_bits_data(io_debug_rb_inst_6_bits_vecWrites_5_bits_data);
  core.io_debug_rb_inst_6_bits_vecWrites_5_bits_idx(io_debug_rb_inst_6_bits_vecWrites_5_bits_idx);
  core.io_debug_rb_inst_6_bits_vecWrites_6_valid(io_debug_rb_inst_6_bits_vecWrites_6_valid);
  core.io_debug_rb_inst_6_bits_vecWrites_6_bits_data(io_debug_rb_inst_6_bits_vecWrites_6_bits_data);
  core.io_debug_rb_inst_6_bits_vecWrites_6_bits_idx(io_debug_rb_inst_6_bits_vecWrites_6_bits_idx);
  core.io_debug_rb_inst_6_bits_vecWrites_7_valid(io_debug_rb_inst_6_bits_vecWrites_7_valid);
  core.io_debug_rb_inst_6_bits_vecWrites_7_bits_data(io_debug_rb_inst_6_bits_vecWrites_7_bits_data);
  core.io_debug_rb_inst_6_bits_vecWrites_7_bits_idx(io_debug_rb_inst_6_bits_vecWrites_7_bits_idx);
core.io_debug_rb_inst_7_valid(io_debug_rb_inst_7_valid);
  core.io_debug_rb_inst_7_bits_pc(io_debug_rb_inst_7_bits_pc);
  core.io_debug_rb_inst_7_bits_inst(io_debug_rb_inst_7_bits_inst);
  core.io_debug_rb_inst_7_bits_idx(io_debug_rb_inst_7_bits_idx);
  core.io_debug_rb_inst_7_bits_data(io_debug_rb_inst_7_bits_data);
  core.io_debug_rb_inst_7_bits_trap(io_debug_rb_inst_7_bits_trap);
  core.io_debug_rb_inst_7_bits_vecWrites_0_valid(io_debug_rb_inst_7_bits_vecWrites_0_valid);
  core.io_debug_rb_inst_7_bits_vecWrites_0_bits_data(io_debug_rb_inst_7_bits_vecWrites_0_bits_data);
  core.io_debug_rb_inst_7_bits_vecWrites_0_bits_idx(io_debug_rb_inst_7_bits_vecWrites_0_bits_idx);
  core.io_debug_rb_inst_7_bits_vecWrites_1_valid(io_debug_rb_inst_7_bits_vecWrites_1_valid);
  core.io_debug_rb_inst_7_bits_vecWrites_1_bits_data(io_debug_rb_inst_7_bits_vecWrites_1_bits_data);
  core.io_debug_rb_inst_7_bits_vecWrites_1_bits_idx(io_debug_rb_inst_7_bits_vecWrites_1_bits_idx);
  core.io_debug_rb_inst_7_bits_vecWrites_2_valid(io_debug_rb_inst_7_bits_vecWrites_2_valid);
  core.io_debug_rb_inst_7_bits_vecWrites_2_bits_data(io_debug_rb_inst_7_bits_vecWrites_2_bits_data);
  core.io_debug_rb_inst_7_bits_vecWrites_2_bits_idx(io_debug_rb_inst_7_bits_vecWrites_2_bits_idx);
  core.io_debug_rb_inst_7_bits_vecWrites_3_valid(io_debug_rb_inst_7_bits_vecWrites_3_valid);
  core.io_debug_rb_inst_7_bits_vecWrites_3_bits_data(io_debug_rb_inst_7_bits_vecWrites_3_bits_data);
  core.io_debug_rb_inst_7_bits_vecWrites_3_bits_idx(io_debug_rb_inst_7_bits_vecWrites_3_bits_idx);
  core.io_debug_rb_inst_7_bits_vecWrites_4_valid(io_debug_rb_inst_7_bits_vecWrites_4_valid);
  core.io_debug_rb_inst_7_bits_vecWrites_4_bits_data(io_debug_rb_inst_7_bits_vecWrites_4_bits_data);
  core.io_debug_rb_inst_7_bits_vecWrites_4_bits_idx(io_debug_rb_inst_7_bits_vecWrites_4_bits_idx);
  core.io_debug_rb_inst_7_bits_vecWrites_5_valid(io_debug_rb_inst_7_bits_vecWrites_5_valid);
  core.io_debug_rb_inst_7_bits_vecWrites_5_bits_data(io_debug_rb_inst_7_bits_vecWrites_5_bits_data);
  core.io_debug_rb_inst_7_bits_vecWrites_5_bits_idx(io_debug_rb_inst_7_bits_vecWrites_5_bits_idx);
  core.io_debug_rb_inst_7_bits_vecWrites_6_valid(io_debug_rb_inst_7_bits_vecWrites_6_valid);
  core.io_debug_rb_inst_7_bits_vecWrites_6_bits_data(io_debug_rb_inst_7_bits_vecWrites_6_bits_data);
  core.io_debug_rb_inst_7_bits_vecWrites_6_bits_idx(io_debug_rb_inst_7_bits_vecWrites_6_bits_idx);
  core.io_debug_rb_inst_7_bits_vecWrites_7_valid(io_debug_rb_inst_7_bits_vecWrites_7_valid);
  core.io_debug_rb_inst_7_bits_vecWrites_7_bits_data(io_debug_rb_inst_7_bits_vecWrites_7_bits_data);
  core.io_debug_rb_inst_7_bits_vecWrites_7_bits_idx(io_debug_rb_inst_7_bits_vecWrites_7_bits_idx);

  core.io_debug_float_writeAddr_valid(io_debug_float_writeAddr_valid);
  core.io_debug_float_writeData_0_valid(io_debug_float_writeData_0_valid);
  core.io_debug_float_writeData_1_valid(io_debug_float_writeData_1_valid);
  core.io_debug_float_writeAddr_bits(io_debug_float_writeAddr_bits);
  core.io_debug_float_writeData_0_bits_addr(io_debug_float_writeData_0_bits_addr);
  core.io_debug_float_writeData_0_bits_data(io_debug_float_writeData_0_bits_data);
  core.io_debug_float_writeData_1_bits_addr(io_debug_float_writeData_1_bits_addr);
  core.io_debug_float_writeData_1_bits_data(io_debug_float_writeData_1_bits_data);

#define BIND_REGFILE_WRITE_ADDR(x) \
  core.io_debug_regfile_writeAddr_##x##_valid(io_debug_regfile_writeAddr_##x##_valid); \
  core.io_debug_regfile_writeAddr_##x##_bits(io_debug_regfile_writeAddr_##x##_bits);

#define BIND_REGFILE_WRITE_DATA(x) \
  core.io_debug_regfile_writeData_##x##_valid(io_debug_regfile_writeData_##x##_valid); \
  core.io_debug_regfile_writeData_##x##_bits_addr(io_debug_regfile_writeData_##x##_bits_addr); \
  core.io_debug_regfile_writeData_##x##_bits_data(io_debug_regfile_writeData_##x##_bits_data);

  REPEAT_4(BIND_REGFILE_WRITE_ADDR);
  REPEAT_6(BIND_REGFILE_WRITE_DATA);

#undef BIND_REGFILE_WRITE_ADDR
#undef BIND_REGFILE_WRITE_DATA

  core.io_debug_en(io_debug_en);
  core.io_debug_cycles(io_debug_cycles);

#define BIND_DEBUG_ADDR(x) core.io_debug_addr_##x(io_debug_addr_##x);
#define BIND_DEBUG_INST(x) core.io_debug_inst_##x(io_debug_inst_##x);

  REPEAT_4(BIND_DEBUG_ADDR);
  REPEAT_4(BIND_DEBUG_INST);

#undef BIND_DEBUG_ADDR
#undef BIND_DEBUG_INST

  core.io_debug_dbus_valid(io_debug_dbus_valid);
  core.io_debug_dbus_bits_addr(io_debug_dbus_bits_addr);
  core.io_debug_dbus_bits_wdata(io_debug_dbus_bits_wdata);
  core.io_debug_dbus_bits_write(io_debug_dbus_bits_write);

#define BIND_DEBUG_DISPATCH(x) \
  core.io_debug_dispatch_##x##_instFire(io_debug_dispatch_##x##_instFire); \
  core.io_debug_dispatch_##x##_instAddr(io_debug_dispatch_##x##_instAddr); \
  core.io_debug_dispatch_##x##_instInst(io_debug_dispatch_##x##_instInst);

  REPEAT_4(BIND_DEBUG_DISPATCH);
#undef BIND_DEBUG_DISPATCH

  core.io_iflush_valid(io_iflush_valid);
  core.io_iflush_pcNext(io_iflush_pcNext);
  core.io_iflush_ready(io_iflush_ready);
  core.io_dflush_valid(io_dflush_valid);
  core.io_dflush_all(io_dflush_all);
  core.io_dflush_clean(io_dflush_clean);
  core.io_dflush_ready(io_dflush_ready);

  core.io_ebus_dbus_valid(io_ebus_dbus_valid);
  core.io_ebus_dbus_ready(io_ebus_dbus_ready);
  core.io_ebus_dbus_write(io_ebus_dbus_write);
  core.io_ebus_internal(io_ebus_internal);
  core.io_ebus_fault_valid(io_ebus_fault_valid);
  core.io_ebus_fault_bits_write(io_ebus_fault_bits_write);
  core.io_ebus_dbus_pc(io_ebus_dbus_pc);
  core.io_ebus_dbus_addr(io_ebus_dbus_addr);
  core.io_ebus_dbus_adrx(io_ebus_dbus_adrx);
  core.io_ebus_dbus_size(io_ebus_dbus_size);
  core.io_ebus_dbus_wdata(io_ebus_dbus_wdata);
  core.io_ebus_dbus_wmask(io_ebus_dbus_wmask);
  core.io_ebus_dbus_rdata(io_ebus_dbus_rdata);
  core.io_ebus_fault_bits_addr(io_ebus_fault_bits_addr);
  core.io_ebus_fault_bits_epc(io_ebus_fault_bits_epc);

  core.io_dm_debug_req(io_dm_debug_req);
  core.io_dm_resume_req(io_dm_resume_req);
  core.io_dm_csr_valid(io_dm_csr_valid);
  core.io_dm_csr_rd_valid(io_dm_csr_rd_valid);
  core.io_dm_scalar_rd_ready(io_dm_scalar_rd_ready);
  core.io_dm_scalar_rd_valid(io_dm_scalar_rd_valid);
  core.io_dm_float_rd_valid(io_dm_float_rd_valid);
  core.io_dm_float_rd_data_sign(io_dm_float_rd_data_sign);
  core.io_dm_float_rs_valid(io_dm_float_rs_valid);
  core.io_dm_float_rs_data_sign(io_dm_float_rs_data_sign);
  core.io_dm_debug_mode(io_dm_debug_mode);
  core.io_dm_csr_bits_addr(io_dm_csr_bits_addr);
  core.io_dm_csr_bits_index(io_dm_csr_bits_index);
  core.io_dm_csr_bits_rs1(io_dm_csr_bits_rs1);
  core.io_dm_csr_bits_op(io_dm_csr_bits_op);
  core.io_dm_csr_rs1(io_dm_csr_rs1);
  core.io_dm_csr_rd_bits(io_dm_csr_rd_bits);
  core.io_dm_scalar_rd_bits_addr(io_dm_scalar_rd_bits_addr);
  core.io_dm_scalar_rd_bits_data(io_dm_scalar_rd_bits_data);
  core.io_dm_scalar_rs_idx(io_dm_scalar_rs_idx);
  core.io_dm_scalar_rs_data(io_dm_scalar_rs_data);
  core.io_dm_float_rd_addr(io_dm_float_rd_addr);
  core.io_dm_float_rd_data_mantissa(io_dm_float_rd_data_mantissa);
  core.io_dm_float_rd_data_exponent(io_dm_float_rd_data_exponent);
  core.io_dm_float_rs_addr(io_dm_float_rs_addr);
  core.io_dm_float_rs_data_mantissa(io_dm_float_rs_data_mantissa);
  core.io_dm_float_rs_data_exponent(io_dm_float_rs_data_exponent);

#define BIND_CSR_OUT(x) core.io_csr_out_value_##x(io_csr_out_value_##x);
  REPEAT_8(BIND_CSR_OUT);
  BIND_CSR_OUT(8);
#undef BIND_CSR_OUT

#define BIND_CSR_IN(x) core.io_csr_in_value_##x(io_csr_in_value_##x);
  REPEAT_13(BIND_CSR_IN);
#undef BIND_CSR_IN

  io_iflush_ready = 1;
  io_irq = 0;
  io_timer_irq = 0;
  io_software_irq = 0;
  io_debug_req = 0;
  io_dflush_ready = 1;
  io_ebus_dbus_ready = 1;
  io_ebus_fault_valid = 0;
  io_ebus_fault_bits_write = 0;
  io_ebus_dbus_rdata = 0;
  io_ebus_fault_bits_addr = 0;
  io_ebus_fault_bits_epc = 0;
  io_dm_debug_req = 0;
  io_dm_resume_req = 0;
  io_dm_csr_valid = 0;
  io_dm_scalar_rd_valid = 0;
  io_dm_float_rd_valid = 0;
  io_dm_float_rd_data_sign = 0;
  io_dm_float_rs_valid = 0;
  io_dm_csr_bits_addr = 0;
  io_dm_csr_bits_index = 0;
  io_dm_csr_bits_rs1 = 0;
  io_dm_csr_bits_op = 0;
  io_dm_csr_rs1 = 0;
  io_dm_scalar_rd_bits_addr = 0;
  io_dm_scalar_rd_bits_data = 0;
  io_dm_scalar_rs_idx = 0;
  io_dm_float_rd_addr = 0;
  io_dm_float_rd_data_mantissa = 0;
  io_dm_float_rd_data_exponent = 0;
  io_dm_float_rs_addr = 0;

#define INIT_CSR_IN(x) io_csr_in_value_##x = 0;
  REPEAT_13(INIT_CSR_IN);
#undef INIT_CSR_IN


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

  tb.start();

#if TRACE_ENABLED
  // Wait for buffer to drain
  while(!buffer.IsEmpty()) {
    std::this_thread::yield();
  }
  daemon.Stop();
#endif

  if (io_halted.read() || tb.ebreak_halt) {
    printf("Simulation HALTED gracefully.\n");
    return 0;
  } else if (tb.instruction_count >= tb.instruction_limit) {
    fprintf(stderr, "Simulation TIMEOUT after %lu instructions.\n", tb.instruction_count);
    return 1;
  } else {
    fprintf(stderr, "Simulation TIMEOUT after %d instructions.\n", instruction_limit);
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

  int timeout_limit = absl::GetFlag(FLAGS_instructions);

  return CoreRvvi_run(Sysc_tb::get_name(argv[0]), path, timeout_limit,
           absl::GetFlag(FLAGS_trace), absl::GetFlag(FLAGS_rvvi_out),
           absl::GetFlag(FLAGS_memory_profile));
}

int main(int argc, char* argv[]) {
  return sc_main(argc, argv);
}
