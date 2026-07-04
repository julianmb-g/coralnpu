// Copyright 2026 Google LLC
#define STRINGIZE(x) #x
#define STR(x) STRINGIZE(x)
#define MODEL_HEADER_SUFFIX .h
#define MODEL_HEADER STR(VERILATOR_MODEL MODEL_HEADER_SUFFIX)
#include MODEL_HEADER

#define PARAMS_HEADER_PREFIX hdl/chisel/src/coralnpu/
#define PARAMS_HEADER_SUFFIX _parameters.h
#define PARAMS_HEADER STR(PARAMS_HEADER_PREFIX VERILATOR_MODEL PARAMS_HEADER_SUFFIX)
#include PARAMS_HEADER

#define TRACE_ENABLED 1


#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <thread>
#include <iostream>
#include <fstream>
#include <limits>
#include <string>
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
#include "tests/verilator_sim/rvvi/mpact_trace_formatter.h"
#endif

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 4096
#endif



using namespace mpact::sim::riscv::rvvi;

// Fulfills the RVVI Traced Target Implementation requirement. Extracts state from
// io_debug_rb_... ports and formats it using the asynchronous trace daemon.

ABSL_FLAG(int, instructions, 500000, "Instruction timeout");
ABSL_FLAG(bool, trace, false, "Dump VCD trace");
ABSL_FLAG(std::string, rvvi_out, "trace.rvvi", "RVVI trace output file");
ABSL_FLAG(std::string, memory_profile, "default", "Memory profile ('default' or 'highmem')");

struct CoreRvvi_tb : Sysc_tb {
  using Sysc_tb::cycle;
  sc_in<bool> io_halted;
  sc_in<bool> io_fault;
  sc_in<bool> io_ibus_valid;

  uint64_t last_time = 0;
  uint64_t last_delta = 0;
  bool had_deadlock = false;

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


  SpscRingBuffer<TracePacket, BUFFER_SIZE>* buffer;
  bool e_sent = false;
  bool ebreak_halt = false;
  bool had_io_fault = false;
  uint32_t internal_v_id = 0;
  uint64_t instruction_count = 0;
  uint64_t instruction_limit = 500000;

  CoreRvvi_tb(sc_module_name name, int instruction_limit, bool random, SpscRingBuffer<TracePacket, BUFFER_SIZE>* buf) 
    : Sysc_tb(name, instruction_limit * 10, random), buffer(buf), instruction_limit(instruction_limit) {
    SC_METHOD(monitor_delta);
    sensitive << io_ibus_valid;
  }

  void monitor_delta() {
    uint64_t current_time = sc_time_stamp().value();
    uint64_t current_delta = sc_delta_count();
    if (current_time == last_time) {
        if (current_delta - last_delta > 10000) {
            fprintf(stderr, "[FATAL] Delta cycle deadlock detected! Time: %lu, Delta: %lu\n", current_time, current_delta);
            had_deadlock = true;
            sc_stop();
        }
    } else {
        last_time = current_time;
        last_delta = current_delta;
    }
  }

  void posedge() {
    
    if (!e_sent) {
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
              exit(124); \
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
            exit(124); \
          } \
        } \
      } \
      \
      uint32_t opcode = inst & 0x7f; \
      bool writes_rd = (opcode == 0x13) || (opcode == 0x33) || (opcode == 0x37) || \
                       (opcode == 0x17) || (opcode == 0x6f) || (opcode == 0x67) || \
                       (opcode == 0x03) || (opcode == 0x73) || (opcode == 0x57) || \
                       (opcode == 0x07) || (opcode == 0x53); \
      \
      if (writes_rd) { \
        uint32_t rd = (inst >> 7) & 0x1f; \
        uint32_t wb_idx = io_debug_rb_inst_##x##_bits_idx.read().to_uint(); \
        char r_type = 'X'; \
        if (wb_idx >= 32 && wb_idx < 64) r_type = 'F'; \
        else if (wb_idx >= 64 && wb_idx < 96) r_type = 'V'; \
        bool is_vec = (r_type == 'V'); \
        if (rd != 0 || is_vec || r_type == 'F') { \
          int num_vec_writes = is_vec ? 8 : 1; \
          int sub_packets = is_vec ? ((KP_rvvVlen + 255) / 256) : 1; \
          for (int i = 0; i < num_vec_writes; ++i) { \
            for (int sp = 0; sp < sub_packets; ++sp) { \
              TracePacket rpacket = {}; \
              rpacket.type = 'R'; \
              rpacket.v_id = v_id; \
              rpacket.reg.reg_type = r_type; \
              rpacket.reg.index = rd; \
              rpacket.reg.offset = is_vec ? (sp * 32) : 0; \
              rpacket.reg.size = is_vec ? (((KP_rvvVlen / 8) - sp * 32 < 32) ? ((KP_rvvVlen / 8) - sp * 32) : 32) : (KP_xlen / 8); \
              rpacket.reg.total_size = is_vec ? (KP_rvvVlen / 8) : (KP_xlen / 8); \
              \
              if (is_vec) { \
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
                    exit(124); \
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
    }

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

    if ((io_halted.read() || io_fault.read() || ebreak_halt || instruction_count >= instruction_limit) && !e_sent) {
      TracePacket epacket = {};
      epacket.type = 'E';
      {
        auto start = std::chrono::steady_clock::now();
        while (!buffer->Push(epacket)) {
          std::this_thread::yield();
          auto now = std::chrono::steady_clock::now();
          if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() > 5) {
            fprintf(stderr, "[FATAL] Queue backpressure timeout (E-packet)! Watchdog triggered.\n");
            exit(124);
          }
        }
      }
      e_sent = true;
    }

    if (io_fault) {
      fprintf(stderr, "[ERROR] io_fault asserted\n");
      had_io_fault = true;
      sc_stop();
    }
    if (io_halted.read() || ebreak_halt) {
      sc_stop();
    }
    if (!ebreak_halt && !io_fault) {
      // already stopped if io_fault
    }
  }
};

bool LoadElfToMemory(const std::string& file_name, Core_if& memory_interface, uint32_t& entry_point) {
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
              [&memory_interface, &load_ok](void* dest, const void* src, size_t count) {
                uint64_t addr = reinterpret_cast<uint64_t>(dest);
                if (!memory_interface.Write(addr, count, reinterpret_cast<const uint8_t*>(src))) {
                  LOG(ERROR) << absl::StrFormat("[FATAL] ELF load violation. Requested: [0x%08lx - 0x%08lx]. Available: %s. Delta: Exceeds bounds by 0x%lx bytes.", addr, addr + count, memory_interface.GetProfileBounds(), memory_interface.GetOverflowDelta(addr, count));
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
  SpscRingBuffer<TracePacket, BUFFER_SIZE> buffer;
  CoreRvvi_tb testbench("CoreRvvi_tb", instruction_limit, /* random= */ false, &buffer);
#else
  CoreRvvi_tb testbench("CoreRvvi_tb", instruction_limit, /* random= */ false, nullptr);
#endif
  Core_if memory_interface("Core_if", /* bin= */ nullptr, memory_profile);

  uint32_t entry_point = 0x00000000;
  if (!LoadElfToMemory(bin, memory_interface, entry_point)) {
    fprintf(stderr, "Error backdoor loading ELF: %s\n", bin);
    exit(65);
  }

#if TRACE_ENABLED
  std::ofstream trace_stream(rvvi_out);
  MpactTraceFormatter formatter;
  TraceDaemon<KP_rvvVlen> daemon(&buffer, &trace_stream);
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


  testbench.io_halted(io_halted);
  testbench.io_fault(io_fault);
  testbench.io_ibus_valid(io_ibus_valid);
  testbench.io_debug_rb_inst_0_valid(io_debug_rb_inst_0_valid);
  testbench.io_debug_rb_inst_0_bits_pc(io_debug_rb_inst_0_bits_pc);
  testbench.io_debug_rb_inst_0_bits_inst(io_debug_rb_inst_0_bits_inst);
  testbench.io_debug_rb_inst_0_bits_idx(io_debug_rb_inst_0_bits_idx);
  testbench.io_debug_rb_inst_0_bits_data(io_debug_rb_inst_0_bits_data);
  testbench.io_debug_rb_inst_0_bits_trap(io_debug_rb_inst_0_bits_trap);

  testbench.io_debug_rb_inst_0_bits_vecWrites_0_valid(io_debug_rb_inst_0_bits_vecWrites_0_valid);
  testbench.io_debug_rb_inst_0_bits_vecWrites_0_bits_data(io_debug_rb_inst_0_bits_vecWrites_0_bits_data);
  testbench.io_debug_rb_inst_0_bits_vecWrites_0_bits_idx(io_debug_rb_inst_0_bits_vecWrites_0_bits_idx);
  testbench.io_debug_rb_inst_0_bits_vecWrites_1_valid(io_debug_rb_inst_0_bits_vecWrites_1_valid);
  testbench.io_debug_rb_inst_0_bits_vecWrites_1_bits_data(io_debug_rb_inst_0_bits_vecWrites_1_bits_data);
  testbench.io_debug_rb_inst_0_bits_vecWrites_1_bits_idx(io_debug_rb_inst_0_bits_vecWrites_1_bits_idx);
  testbench.io_debug_rb_inst_0_bits_vecWrites_2_valid(io_debug_rb_inst_0_bits_vecWrites_2_valid);
  testbench.io_debug_rb_inst_0_bits_vecWrites_2_bits_data(io_debug_rb_inst_0_bits_vecWrites_2_bits_data);
  testbench.io_debug_rb_inst_0_bits_vecWrites_2_bits_idx(io_debug_rb_inst_0_bits_vecWrites_2_bits_idx);
  testbench.io_debug_rb_inst_0_bits_vecWrites_3_valid(io_debug_rb_inst_0_bits_vecWrites_3_valid);
  testbench.io_debug_rb_inst_0_bits_vecWrites_3_bits_data(io_debug_rb_inst_0_bits_vecWrites_3_bits_data);
  testbench.io_debug_rb_inst_0_bits_vecWrites_3_bits_idx(io_debug_rb_inst_0_bits_vecWrites_3_bits_idx);
  testbench.io_debug_rb_inst_0_bits_vecWrites_4_valid(io_debug_rb_inst_0_bits_vecWrites_4_valid);
  testbench.io_debug_rb_inst_0_bits_vecWrites_4_bits_data(io_debug_rb_inst_0_bits_vecWrites_4_bits_data);
  testbench.io_debug_rb_inst_0_bits_vecWrites_4_bits_idx(io_debug_rb_inst_0_bits_vecWrites_4_bits_idx);
  testbench.io_debug_rb_inst_0_bits_vecWrites_5_valid(io_debug_rb_inst_0_bits_vecWrites_5_valid);
  testbench.io_debug_rb_inst_0_bits_vecWrites_5_bits_data(io_debug_rb_inst_0_bits_vecWrites_5_bits_data);
  testbench.io_debug_rb_inst_0_bits_vecWrites_5_bits_idx(io_debug_rb_inst_0_bits_vecWrites_5_bits_idx);
  testbench.io_debug_rb_inst_0_bits_vecWrites_6_valid(io_debug_rb_inst_0_bits_vecWrites_6_valid);
  testbench.io_debug_rb_inst_0_bits_vecWrites_6_bits_data(io_debug_rb_inst_0_bits_vecWrites_6_bits_data);
  testbench.io_debug_rb_inst_0_bits_vecWrites_6_bits_idx(io_debug_rb_inst_0_bits_vecWrites_6_bits_idx);
  testbench.io_debug_rb_inst_0_bits_vecWrites_7_valid(io_debug_rb_inst_0_bits_vecWrites_7_valid);
  testbench.io_debug_rb_inst_0_bits_vecWrites_7_bits_data(io_debug_rb_inst_0_bits_vecWrites_7_bits_data);
  testbench.io_debug_rb_inst_0_bits_vecWrites_7_bits_idx(io_debug_rb_inst_0_bits_vecWrites_7_bits_idx);
testbench.io_debug_rb_inst_1_valid(io_debug_rb_inst_1_valid);
  testbench.io_debug_rb_inst_1_bits_pc(io_debug_rb_inst_1_bits_pc);
  testbench.io_debug_rb_inst_1_bits_inst(io_debug_rb_inst_1_bits_inst);
  testbench.io_debug_rb_inst_1_bits_idx(io_debug_rb_inst_1_bits_idx);
  testbench.io_debug_rb_inst_1_bits_data(io_debug_rb_inst_1_bits_data);
  testbench.io_debug_rb_inst_1_bits_trap(io_debug_rb_inst_1_bits_trap);

  testbench.io_debug_rb_inst_1_bits_vecWrites_0_valid(io_debug_rb_inst_1_bits_vecWrites_0_valid);
  testbench.io_debug_rb_inst_1_bits_vecWrites_0_bits_data(io_debug_rb_inst_1_bits_vecWrites_0_bits_data);
  testbench.io_debug_rb_inst_1_bits_vecWrites_0_bits_idx(io_debug_rb_inst_1_bits_vecWrites_0_bits_idx);
  testbench.io_debug_rb_inst_1_bits_vecWrites_1_valid(io_debug_rb_inst_1_bits_vecWrites_1_valid);
  testbench.io_debug_rb_inst_1_bits_vecWrites_1_bits_data(io_debug_rb_inst_1_bits_vecWrites_1_bits_data);
  testbench.io_debug_rb_inst_1_bits_vecWrites_1_bits_idx(io_debug_rb_inst_1_bits_vecWrites_1_bits_idx);
  testbench.io_debug_rb_inst_1_bits_vecWrites_2_valid(io_debug_rb_inst_1_bits_vecWrites_2_valid);
  testbench.io_debug_rb_inst_1_bits_vecWrites_2_bits_data(io_debug_rb_inst_1_bits_vecWrites_2_bits_data);
  testbench.io_debug_rb_inst_1_bits_vecWrites_2_bits_idx(io_debug_rb_inst_1_bits_vecWrites_2_bits_idx);
  testbench.io_debug_rb_inst_1_bits_vecWrites_3_valid(io_debug_rb_inst_1_bits_vecWrites_3_valid);
  testbench.io_debug_rb_inst_1_bits_vecWrites_3_bits_data(io_debug_rb_inst_1_bits_vecWrites_3_bits_data);
  testbench.io_debug_rb_inst_1_bits_vecWrites_3_bits_idx(io_debug_rb_inst_1_bits_vecWrites_3_bits_idx);
  testbench.io_debug_rb_inst_1_bits_vecWrites_4_valid(io_debug_rb_inst_1_bits_vecWrites_4_valid);
  testbench.io_debug_rb_inst_1_bits_vecWrites_4_bits_data(io_debug_rb_inst_1_bits_vecWrites_4_bits_data);
  testbench.io_debug_rb_inst_1_bits_vecWrites_4_bits_idx(io_debug_rb_inst_1_bits_vecWrites_4_bits_idx);
  testbench.io_debug_rb_inst_1_bits_vecWrites_5_valid(io_debug_rb_inst_1_bits_vecWrites_5_valid);
  testbench.io_debug_rb_inst_1_bits_vecWrites_5_bits_data(io_debug_rb_inst_1_bits_vecWrites_5_bits_data);
  testbench.io_debug_rb_inst_1_bits_vecWrites_5_bits_idx(io_debug_rb_inst_1_bits_vecWrites_5_bits_idx);
  testbench.io_debug_rb_inst_1_bits_vecWrites_6_valid(io_debug_rb_inst_1_bits_vecWrites_6_valid);
  testbench.io_debug_rb_inst_1_bits_vecWrites_6_bits_data(io_debug_rb_inst_1_bits_vecWrites_6_bits_data);
  testbench.io_debug_rb_inst_1_bits_vecWrites_6_bits_idx(io_debug_rb_inst_1_bits_vecWrites_6_bits_idx);
  testbench.io_debug_rb_inst_1_bits_vecWrites_7_valid(io_debug_rb_inst_1_bits_vecWrites_7_valid);
  testbench.io_debug_rb_inst_1_bits_vecWrites_7_bits_data(io_debug_rb_inst_1_bits_vecWrites_7_bits_data);
  testbench.io_debug_rb_inst_1_bits_vecWrites_7_bits_idx(io_debug_rb_inst_1_bits_vecWrites_7_bits_idx);
testbench.io_debug_rb_inst_2_valid(io_debug_rb_inst_2_valid);
  testbench.io_debug_rb_inst_2_bits_pc(io_debug_rb_inst_2_bits_pc);
  testbench.io_debug_rb_inst_2_bits_inst(io_debug_rb_inst_2_bits_inst);
  testbench.io_debug_rb_inst_2_bits_idx(io_debug_rb_inst_2_bits_idx);
  testbench.io_debug_rb_inst_2_bits_data(io_debug_rb_inst_2_bits_data);
  testbench.io_debug_rb_inst_2_bits_trap(io_debug_rb_inst_2_bits_trap);

  testbench.io_debug_rb_inst_2_bits_vecWrites_0_valid(io_debug_rb_inst_2_bits_vecWrites_0_valid);
  testbench.io_debug_rb_inst_2_bits_vecWrites_0_bits_data(io_debug_rb_inst_2_bits_vecWrites_0_bits_data);
  testbench.io_debug_rb_inst_2_bits_vecWrites_0_bits_idx(io_debug_rb_inst_2_bits_vecWrites_0_bits_idx);
  testbench.io_debug_rb_inst_2_bits_vecWrites_1_valid(io_debug_rb_inst_2_bits_vecWrites_1_valid);
  testbench.io_debug_rb_inst_2_bits_vecWrites_1_bits_data(io_debug_rb_inst_2_bits_vecWrites_1_bits_data);
  testbench.io_debug_rb_inst_2_bits_vecWrites_1_bits_idx(io_debug_rb_inst_2_bits_vecWrites_1_bits_idx);
  testbench.io_debug_rb_inst_2_bits_vecWrites_2_valid(io_debug_rb_inst_2_bits_vecWrites_2_valid);
  testbench.io_debug_rb_inst_2_bits_vecWrites_2_bits_data(io_debug_rb_inst_2_bits_vecWrites_2_bits_data);
  testbench.io_debug_rb_inst_2_bits_vecWrites_2_bits_idx(io_debug_rb_inst_2_bits_vecWrites_2_bits_idx);
  testbench.io_debug_rb_inst_2_bits_vecWrites_3_valid(io_debug_rb_inst_2_bits_vecWrites_3_valid);
  testbench.io_debug_rb_inst_2_bits_vecWrites_3_bits_data(io_debug_rb_inst_2_bits_vecWrites_3_bits_data);
  testbench.io_debug_rb_inst_2_bits_vecWrites_3_bits_idx(io_debug_rb_inst_2_bits_vecWrites_3_bits_idx);
  testbench.io_debug_rb_inst_2_bits_vecWrites_4_valid(io_debug_rb_inst_2_bits_vecWrites_4_valid);
  testbench.io_debug_rb_inst_2_bits_vecWrites_4_bits_data(io_debug_rb_inst_2_bits_vecWrites_4_bits_data);
  testbench.io_debug_rb_inst_2_bits_vecWrites_4_bits_idx(io_debug_rb_inst_2_bits_vecWrites_4_bits_idx);
  testbench.io_debug_rb_inst_2_bits_vecWrites_5_valid(io_debug_rb_inst_2_bits_vecWrites_5_valid);
  testbench.io_debug_rb_inst_2_bits_vecWrites_5_bits_data(io_debug_rb_inst_2_bits_vecWrites_5_bits_data);
  testbench.io_debug_rb_inst_2_bits_vecWrites_5_bits_idx(io_debug_rb_inst_2_bits_vecWrites_5_bits_idx);
  testbench.io_debug_rb_inst_2_bits_vecWrites_6_valid(io_debug_rb_inst_2_bits_vecWrites_6_valid);
  testbench.io_debug_rb_inst_2_bits_vecWrites_6_bits_data(io_debug_rb_inst_2_bits_vecWrites_6_bits_data);
  testbench.io_debug_rb_inst_2_bits_vecWrites_6_bits_idx(io_debug_rb_inst_2_bits_vecWrites_6_bits_idx);
  testbench.io_debug_rb_inst_2_bits_vecWrites_7_valid(io_debug_rb_inst_2_bits_vecWrites_7_valid);
  testbench.io_debug_rb_inst_2_bits_vecWrites_7_bits_data(io_debug_rb_inst_2_bits_vecWrites_7_bits_data);
  testbench.io_debug_rb_inst_2_bits_vecWrites_7_bits_idx(io_debug_rb_inst_2_bits_vecWrites_7_bits_idx);
testbench.io_debug_rb_inst_3_valid(io_debug_rb_inst_3_valid);
  testbench.io_debug_rb_inst_3_bits_pc(io_debug_rb_inst_3_bits_pc);
  testbench.io_debug_rb_inst_3_bits_inst(io_debug_rb_inst_3_bits_inst);
  testbench.io_debug_rb_inst_3_bits_idx(io_debug_rb_inst_3_bits_idx);
  testbench.io_debug_rb_inst_3_bits_data(io_debug_rb_inst_3_bits_data);
  testbench.io_debug_rb_inst_3_bits_trap(io_debug_rb_inst_3_bits_trap);

  testbench.io_debug_rb_inst_3_bits_vecWrites_0_valid(io_debug_rb_inst_3_bits_vecWrites_0_valid);
  testbench.io_debug_rb_inst_3_bits_vecWrites_0_bits_data(io_debug_rb_inst_3_bits_vecWrites_0_bits_data);
  testbench.io_debug_rb_inst_3_bits_vecWrites_0_bits_idx(io_debug_rb_inst_3_bits_vecWrites_0_bits_idx);
  testbench.io_debug_rb_inst_3_bits_vecWrites_1_valid(io_debug_rb_inst_3_bits_vecWrites_1_valid);
  testbench.io_debug_rb_inst_3_bits_vecWrites_1_bits_data(io_debug_rb_inst_3_bits_vecWrites_1_bits_data);
  testbench.io_debug_rb_inst_3_bits_vecWrites_1_bits_idx(io_debug_rb_inst_3_bits_vecWrites_1_bits_idx);
  testbench.io_debug_rb_inst_3_bits_vecWrites_2_valid(io_debug_rb_inst_3_bits_vecWrites_2_valid);
  testbench.io_debug_rb_inst_3_bits_vecWrites_2_bits_data(io_debug_rb_inst_3_bits_vecWrites_2_bits_data);
  testbench.io_debug_rb_inst_3_bits_vecWrites_2_bits_idx(io_debug_rb_inst_3_bits_vecWrites_2_bits_idx);
  testbench.io_debug_rb_inst_3_bits_vecWrites_3_valid(io_debug_rb_inst_3_bits_vecWrites_3_valid);
  testbench.io_debug_rb_inst_3_bits_vecWrites_3_bits_data(io_debug_rb_inst_3_bits_vecWrites_3_bits_data);
  testbench.io_debug_rb_inst_3_bits_vecWrites_3_bits_idx(io_debug_rb_inst_3_bits_vecWrites_3_bits_idx);
  testbench.io_debug_rb_inst_3_bits_vecWrites_4_valid(io_debug_rb_inst_3_bits_vecWrites_4_valid);
  testbench.io_debug_rb_inst_3_bits_vecWrites_4_bits_data(io_debug_rb_inst_3_bits_vecWrites_4_bits_data);
  testbench.io_debug_rb_inst_3_bits_vecWrites_4_bits_idx(io_debug_rb_inst_3_bits_vecWrites_4_bits_idx);
  testbench.io_debug_rb_inst_3_bits_vecWrites_5_valid(io_debug_rb_inst_3_bits_vecWrites_5_valid);
  testbench.io_debug_rb_inst_3_bits_vecWrites_5_bits_data(io_debug_rb_inst_3_bits_vecWrites_5_bits_data);
  testbench.io_debug_rb_inst_3_bits_vecWrites_5_bits_idx(io_debug_rb_inst_3_bits_vecWrites_5_bits_idx);
  testbench.io_debug_rb_inst_3_bits_vecWrites_6_valid(io_debug_rb_inst_3_bits_vecWrites_6_valid);
  testbench.io_debug_rb_inst_3_bits_vecWrites_6_bits_data(io_debug_rb_inst_3_bits_vecWrites_6_bits_data);
  testbench.io_debug_rb_inst_3_bits_vecWrites_6_bits_idx(io_debug_rb_inst_3_bits_vecWrites_6_bits_idx);
  testbench.io_debug_rb_inst_3_bits_vecWrites_7_valid(io_debug_rb_inst_3_bits_vecWrites_7_valid);
  testbench.io_debug_rb_inst_3_bits_vecWrites_7_bits_data(io_debug_rb_inst_3_bits_vecWrites_7_bits_data);
  testbench.io_debug_rb_inst_3_bits_vecWrites_7_bits_idx(io_debug_rb_inst_3_bits_vecWrites_7_bits_idx);
testbench.io_debug_rb_inst_4_valid(io_debug_rb_inst_4_valid);
  testbench.io_debug_rb_inst_4_bits_pc(io_debug_rb_inst_4_bits_pc);
  testbench.io_debug_rb_inst_4_bits_inst(io_debug_rb_inst_4_bits_inst);
  testbench.io_debug_rb_inst_4_bits_idx(io_debug_rb_inst_4_bits_idx);
  testbench.io_debug_rb_inst_4_bits_data(io_debug_rb_inst_4_bits_data);
  testbench.io_debug_rb_inst_4_bits_trap(io_debug_rb_inst_4_bits_trap);

  testbench.io_debug_rb_inst_4_bits_vecWrites_0_valid(io_debug_rb_inst_4_bits_vecWrites_0_valid);
  testbench.io_debug_rb_inst_4_bits_vecWrites_0_bits_data(io_debug_rb_inst_4_bits_vecWrites_0_bits_data);
  testbench.io_debug_rb_inst_4_bits_vecWrites_0_bits_idx(io_debug_rb_inst_4_bits_vecWrites_0_bits_idx);
  testbench.io_debug_rb_inst_4_bits_vecWrites_1_valid(io_debug_rb_inst_4_bits_vecWrites_1_valid);
  testbench.io_debug_rb_inst_4_bits_vecWrites_1_bits_data(io_debug_rb_inst_4_bits_vecWrites_1_bits_data);
  testbench.io_debug_rb_inst_4_bits_vecWrites_1_bits_idx(io_debug_rb_inst_4_bits_vecWrites_1_bits_idx);
  testbench.io_debug_rb_inst_4_bits_vecWrites_2_valid(io_debug_rb_inst_4_bits_vecWrites_2_valid);
  testbench.io_debug_rb_inst_4_bits_vecWrites_2_bits_data(io_debug_rb_inst_4_bits_vecWrites_2_bits_data);
  testbench.io_debug_rb_inst_4_bits_vecWrites_2_bits_idx(io_debug_rb_inst_4_bits_vecWrites_2_bits_idx);
  testbench.io_debug_rb_inst_4_bits_vecWrites_3_valid(io_debug_rb_inst_4_bits_vecWrites_3_valid);
  testbench.io_debug_rb_inst_4_bits_vecWrites_3_bits_data(io_debug_rb_inst_4_bits_vecWrites_3_bits_data);
  testbench.io_debug_rb_inst_4_bits_vecWrites_3_bits_idx(io_debug_rb_inst_4_bits_vecWrites_3_bits_idx);
  testbench.io_debug_rb_inst_4_bits_vecWrites_4_valid(io_debug_rb_inst_4_bits_vecWrites_4_valid);
  testbench.io_debug_rb_inst_4_bits_vecWrites_4_bits_data(io_debug_rb_inst_4_bits_vecWrites_4_bits_data);
  testbench.io_debug_rb_inst_4_bits_vecWrites_4_bits_idx(io_debug_rb_inst_4_bits_vecWrites_4_bits_idx);
  testbench.io_debug_rb_inst_4_bits_vecWrites_5_valid(io_debug_rb_inst_4_bits_vecWrites_5_valid);
  testbench.io_debug_rb_inst_4_bits_vecWrites_5_bits_data(io_debug_rb_inst_4_bits_vecWrites_5_bits_data);
  testbench.io_debug_rb_inst_4_bits_vecWrites_5_bits_idx(io_debug_rb_inst_4_bits_vecWrites_5_bits_idx);
  testbench.io_debug_rb_inst_4_bits_vecWrites_6_valid(io_debug_rb_inst_4_bits_vecWrites_6_valid);
  testbench.io_debug_rb_inst_4_bits_vecWrites_6_bits_data(io_debug_rb_inst_4_bits_vecWrites_6_bits_data);
  testbench.io_debug_rb_inst_4_bits_vecWrites_6_bits_idx(io_debug_rb_inst_4_bits_vecWrites_6_bits_idx);
  testbench.io_debug_rb_inst_4_bits_vecWrites_7_valid(io_debug_rb_inst_4_bits_vecWrites_7_valid);
  testbench.io_debug_rb_inst_4_bits_vecWrites_7_bits_data(io_debug_rb_inst_4_bits_vecWrites_7_bits_data);
  testbench.io_debug_rb_inst_4_bits_vecWrites_7_bits_idx(io_debug_rb_inst_4_bits_vecWrites_7_bits_idx);
testbench.io_debug_rb_inst_5_valid(io_debug_rb_inst_5_valid);
  testbench.io_debug_rb_inst_5_bits_pc(io_debug_rb_inst_5_bits_pc);
  testbench.io_debug_rb_inst_5_bits_inst(io_debug_rb_inst_5_bits_inst);
  testbench.io_debug_rb_inst_5_bits_idx(io_debug_rb_inst_5_bits_idx);
  testbench.io_debug_rb_inst_5_bits_data(io_debug_rb_inst_5_bits_data);
  testbench.io_debug_rb_inst_5_bits_trap(io_debug_rb_inst_5_bits_trap);

  testbench.io_debug_rb_inst_5_bits_vecWrites_0_valid(io_debug_rb_inst_5_bits_vecWrites_0_valid);
  testbench.io_debug_rb_inst_5_bits_vecWrites_0_bits_data(io_debug_rb_inst_5_bits_vecWrites_0_bits_data);
  testbench.io_debug_rb_inst_5_bits_vecWrites_0_bits_idx(io_debug_rb_inst_5_bits_vecWrites_0_bits_idx);
  testbench.io_debug_rb_inst_5_bits_vecWrites_1_valid(io_debug_rb_inst_5_bits_vecWrites_1_valid);
  testbench.io_debug_rb_inst_5_bits_vecWrites_1_bits_data(io_debug_rb_inst_5_bits_vecWrites_1_bits_data);
  testbench.io_debug_rb_inst_5_bits_vecWrites_1_bits_idx(io_debug_rb_inst_5_bits_vecWrites_1_bits_idx);
  testbench.io_debug_rb_inst_5_bits_vecWrites_2_valid(io_debug_rb_inst_5_bits_vecWrites_2_valid);
  testbench.io_debug_rb_inst_5_bits_vecWrites_2_bits_data(io_debug_rb_inst_5_bits_vecWrites_2_bits_data);
  testbench.io_debug_rb_inst_5_bits_vecWrites_2_bits_idx(io_debug_rb_inst_5_bits_vecWrites_2_bits_idx);
  testbench.io_debug_rb_inst_5_bits_vecWrites_3_valid(io_debug_rb_inst_5_bits_vecWrites_3_valid);
  testbench.io_debug_rb_inst_5_bits_vecWrites_3_bits_data(io_debug_rb_inst_5_bits_vecWrites_3_bits_data);
  testbench.io_debug_rb_inst_5_bits_vecWrites_3_bits_idx(io_debug_rb_inst_5_bits_vecWrites_3_bits_idx);
  testbench.io_debug_rb_inst_5_bits_vecWrites_4_valid(io_debug_rb_inst_5_bits_vecWrites_4_valid);
  testbench.io_debug_rb_inst_5_bits_vecWrites_4_bits_data(io_debug_rb_inst_5_bits_vecWrites_4_bits_data);
  testbench.io_debug_rb_inst_5_bits_vecWrites_4_bits_idx(io_debug_rb_inst_5_bits_vecWrites_4_bits_idx);
  testbench.io_debug_rb_inst_5_bits_vecWrites_5_valid(io_debug_rb_inst_5_bits_vecWrites_5_valid);
  testbench.io_debug_rb_inst_5_bits_vecWrites_5_bits_data(io_debug_rb_inst_5_bits_vecWrites_5_bits_data);
  testbench.io_debug_rb_inst_5_bits_vecWrites_5_bits_idx(io_debug_rb_inst_5_bits_vecWrites_5_bits_idx);
  testbench.io_debug_rb_inst_5_bits_vecWrites_6_valid(io_debug_rb_inst_5_bits_vecWrites_6_valid);
  testbench.io_debug_rb_inst_5_bits_vecWrites_6_bits_data(io_debug_rb_inst_5_bits_vecWrites_6_bits_data);
  testbench.io_debug_rb_inst_5_bits_vecWrites_6_bits_idx(io_debug_rb_inst_5_bits_vecWrites_6_bits_idx);
  testbench.io_debug_rb_inst_5_bits_vecWrites_7_valid(io_debug_rb_inst_5_bits_vecWrites_7_valid);
  testbench.io_debug_rb_inst_5_bits_vecWrites_7_bits_data(io_debug_rb_inst_5_bits_vecWrites_7_bits_data);
  testbench.io_debug_rb_inst_5_bits_vecWrites_7_bits_idx(io_debug_rb_inst_5_bits_vecWrites_7_bits_idx);
testbench.io_debug_rb_inst_6_valid(io_debug_rb_inst_6_valid);
  testbench.io_debug_rb_inst_6_bits_pc(io_debug_rb_inst_6_bits_pc);
  testbench.io_debug_rb_inst_6_bits_inst(io_debug_rb_inst_6_bits_inst);
  testbench.io_debug_rb_inst_6_bits_idx(io_debug_rb_inst_6_bits_idx);
  testbench.io_debug_rb_inst_6_bits_data(io_debug_rb_inst_6_bits_data);
  testbench.io_debug_rb_inst_6_bits_trap(io_debug_rb_inst_6_bits_trap);

  testbench.io_debug_rb_inst_6_bits_vecWrites_0_valid(io_debug_rb_inst_6_bits_vecWrites_0_valid);
  testbench.io_debug_rb_inst_6_bits_vecWrites_0_bits_data(io_debug_rb_inst_6_bits_vecWrites_0_bits_data);
  testbench.io_debug_rb_inst_6_bits_vecWrites_0_bits_idx(io_debug_rb_inst_6_bits_vecWrites_0_bits_idx);
  testbench.io_debug_rb_inst_6_bits_vecWrites_1_valid(io_debug_rb_inst_6_bits_vecWrites_1_valid);
  testbench.io_debug_rb_inst_6_bits_vecWrites_1_bits_data(io_debug_rb_inst_6_bits_vecWrites_1_bits_data);
  testbench.io_debug_rb_inst_6_bits_vecWrites_1_bits_idx(io_debug_rb_inst_6_bits_vecWrites_1_bits_idx);
  testbench.io_debug_rb_inst_6_bits_vecWrites_2_valid(io_debug_rb_inst_6_bits_vecWrites_2_valid);
  testbench.io_debug_rb_inst_6_bits_vecWrites_2_bits_data(io_debug_rb_inst_6_bits_vecWrites_2_bits_data);
  testbench.io_debug_rb_inst_6_bits_vecWrites_2_bits_idx(io_debug_rb_inst_6_bits_vecWrites_2_bits_idx);
  testbench.io_debug_rb_inst_6_bits_vecWrites_3_valid(io_debug_rb_inst_6_bits_vecWrites_3_valid);
  testbench.io_debug_rb_inst_6_bits_vecWrites_3_bits_data(io_debug_rb_inst_6_bits_vecWrites_3_bits_data);
  testbench.io_debug_rb_inst_6_bits_vecWrites_3_bits_idx(io_debug_rb_inst_6_bits_vecWrites_3_bits_idx);
  testbench.io_debug_rb_inst_6_bits_vecWrites_4_valid(io_debug_rb_inst_6_bits_vecWrites_4_valid);
  testbench.io_debug_rb_inst_6_bits_vecWrites_4_bits_data(io_debug_rb_inst_6_bits_vecWrites_4_bits_data);
  testbench.io_debug_rb_inst_6_bits_vecWrites_4_bits_idx(io_debug_rb_inst_6_bits_vecWrites_4_bits_idx);
  testbench.io_debug_rb_inst_6_bits_vecWrites_5_valid(io_debug_rb_inst_6_bits_vecWrites_5_valid);
  testbench.io_debug_rb_inst_6_bits_vecWrites_5_bits_data(io_debug_rb_inst_6_bits_vecWrites_5_bits_data);
  testbench.io_debug_rb_inst_6_bits_vecWrites_5_bits_idx(io_debug_rb_inst_6_bits_vecWrites_5_bits_idx);
  testbench.io_debug_rb_inst_6_bits_vecWrites_6_valid(io_debug_rb_inst_6_bits_vecWrites_6_valid);
  testbench.io_debug_rb_inst_6_bits_vecWrites_6_bits_data(io_debug_rb_inst_6_bits_vecWrites_6_bits_data);
  testbench.io_debug_rb_inst_6_bits_vecWrites_6_bits_idx(io_debug_rb_inst_6_bits_vecWrites_6_bits_idx);
  testbench.io_debug_rb_inst_6_bits_vecWrites_7_valid(io_debug_rb_inst_6_bits_vecWrites_7_valid);
  testbench.io_debug_rb_inst_6_bits_vecWrites_7_bits_data(io_debug_rb_inst_6_bits_vecWrites_7_bits_data);
  testbench.io_debug_rb_inst_6_bits_vecWrites_7_bits_idx(io_debug_rb_inst_6_bits_vecWrites_7_bits_idx);
testbench.io_debug_rb_inst_7_valid(io_debug_rb_inst_7_valid);
  testbench.io_debug_rb_inst_7_bits_pc(io_debug_rb_inst_7_bits_pc);
  testbench.io_debug_rb_inst_7_bits_inst(io_debug_rb_inst_7_bits_inst);
  testbench.io_debug_rb_inst_7_bits_idx(io_debug_rb_inst_7_bits_idx);
  testbench.io_debug_rb_inst_7_bits_data(io_debug_rb_inst_7_bits_data);
  testbench.io_debug_rb_inst_7_bits_trap(io_debug_rb_inst_7_bits_trap);

  testbench.io_debug_rb_inst_7_bits_vecWrites_0_valid(io_debug_rb_inst_7_bits_vecWrites_0_valid);
  testbench.io_debug_rb_inst_7_bits_vecWrites_0_bits_data(io_debug_rb_inst_7_bits_vecWrites_0_bits_data);
  testbench.io_debug_rb_inst_7_bits_vecWrites_0_bits_idx(io_debug_rb_inst_7_bits_vecWrites_0_bits_idx);
  testbench.io_debug_rb_inst_7_bits_vecWrites_1_valid(io_debug_rb_inst_7_bits_vecWrites_1_valid);
  testbench.io_debug_rb_inst_7_bits_vecWrites_1_bits_data(io_debug_rb_inst_7_bits_vecWrites_1_bits_data);
  testbench.io_debug_rb_inst_7_bits_vecWrites_1_bits_idx(io_debug_rb_inst_7_bits_vecWrites_1_bits_idx);
  testbench.io_debug_rb_inst_7_bits_vecWrites_2_valid(io_debug_rb_inst_7_bits_vecWrites_2_valid);
  testbench.io_debug_rb_inst_7_bits_vecWrites_2_bits_data(io_debug_rb_inst_7_bits_vecWrites_2_bits_data);
  testbench.io_debug_rb_inst_7_bits_vecWrites_2_bits_idx(io_debug_rb_inst_7_bits_vecWrites_2_bits_idx);
  testbench.io_debug_rb_inst_7_bits_vecWrites_3_valid(io_debug_rb_inst_7_bits_vecWrites_3_valid);
  testbench.io_debug_rb_inst_7_bits_vecWrites_3_bits_data(io_debug_rb_inst_7_bits_vecWrites_3_bits_data);
  testbench.io_debug_rb_inst_7_bits_vecWrites_3_bits_idx(io_debug_rb_inst_7_bits_vecWrites_3_bits_idx);
  testbench.io_debug_rb_inst_7_bits_vecWrites_4_valid(io_debug_rb_inst_7_bits_vecWrites_4_valid);
  testbench.io_debug_rb_inst_7_bits_vecWrites_4_bits_data(io_debug_rb_inst_7_bits_vecWrites_4_bits_data);
  testbench.io_debug_rb_inst_7_bits_vecWrites_4_bits_idx(io_debug_rb_inst_7_bits_vecWrites_4_bits_idx);
  testbench.io_debug_rb_inst_7_bits_vecWrites_5_valid(io_debug_rb_inst_7_bits_vecWrites_5_valid);
  testbench.io_debug_rb_inst_7_bits_vecWrites_5_bits_data(io_debug_rb_inst_7_bits_vecWrites_5_bits_data);
  testbench.io_debug_rb_inst_7_bits_vecWrites_5_bits_idx(io_debug_rb_inst_7_bits_vecWrites_5_bits_idx);
  testbench.io_debug_rb_inst_7_bits_vecWrites_6_valid(io_debug_rb_inst_7_bits_vecWrites_6_valid);
  testbench.io_debug_rb_inst_7_bits_vecWrites_6_bits_data(io_debug_rb_inst_7_bits_vecWrites_6_bits_data);
  testbench.io_debug_rb_inst_7_bits_vecWrites_6_bits_idx(io_debug_rb_inst_7_bits_vecWrites_6_bits_idx);
  testbench.io_debug_rb_inst_7_bits_vecWrites_7_valid(io_debug_rb_inst_7_bits_vecWrites_7_valid);
  testbench.io_debug_rb_inst_7_bits_vecWrites_7_bits_data(io_debug_rb_inst_7_bits_vecWrites_7_bits_data);
  testbench.io_debug_rb_inst_7_bits_vecWrites_7_bits_idx(io_debug_rb_inst_7_bits_vecWrites_7_bits_idx);


  core.clock(testbench.clock);
  core.reset(testbench.reset);
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


  memory_interface.clock(testbench.clock);
  memory_interface.reset(testbench.reset);
  memory_interface.io_ibus_valid(io_ibus_valid);
  memory_interface.io_ibus_ready(io_ibus_ready);
  memory_interface.io_ibus_addr(io_ibus_addr);
  memory_interface.io_ibus_rdata(io_ibus_rdata);
  memory_interface.io_dbus_valid(io_dbus_valid);
  memory_interface.io_dbus_ready(io_dbus_ready);
  memory_interface.io_dbus_write(io_dbus_write);
  memory_interface.io_dbus_addr(io_dbus_addr);
  memory_interface.io_dbus_adrx(io_dbus_adrx);
  memory_interface.io_dbus_size(io_dbus_size);
  memory_interface.io_dbus_wdata(io_dbus_wdata);
  memory_interface.io_dbus_wmask(io_dbus_wmask);
  memory_interface.io_dbus_rdata(io_dbus_rdata);
  memory_interface.io_ibus_fault_valid(io_ibus_fault_valid);
  memory_interface.io_ibus_fault_bits_write(io_ibus_fault_bits_write);
  memory_interface.io_ibus_fault_bits_addr(io_ibus_fault_bits_addr);
  memory_interface.io_ibus_fault_bits_epc(io_ibus_fault_bits_epc);
  memory_interface.io_ebus_fault_valid(io_ebus_fault_valid);
  memory_interface.io_ebus_fault_bits_write(io_ebus_fault_bits_write);
  memory_interface.io_ebus_fault_bits_addr(io_ebus_fault_bits_addr);
  memory_interface.io_ebus_fault_bits_epc(io_ebus_fault_bits_epc);

  if (trace) {
    testbench.trace(&core);
  }

  testbench.start();

#if TRACE_ENABLED
  // Wait for buffer to drain
  auto flush_start = std::chrono::steady_clock::now();
  while(!buffer.IsEmpty()) {
    std::this_thread::yield();
    auto flush_now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(flush_now - flush_start).count() > 5) {
      fprintf(stderr, "[FATAL] Queue flush timeout (5s) exceeded! Trace daemon may be hung. Exiting with code 124.\n");
      _exit(124);
    }
  }
  daemon.Stop();
#endif

  if (testbench.had_deadlock) {
    fprintf(stderr, "[FATAL] Simulation failed due to delta cycle deadlock.\n");
    return 1;
  }

  if (io_halted.read() || testbench.ebreak_halt) {
    printf("Simulation HALTED gracefully.\n");
    return 0;
  }

  if (testbench.instruction_count >= testbench.instruction_limit) {
    fprintf(stderr, "Simulation TIMEOUT after %lu instructions.\n", testbench.instruction_count);
    return 124;
  }

  if (memory_interface.pending_exit_code() != 0) {
    return memory_interface.pending_exit_code();
  }

  if (testbench.had_io_fault) {
    fprintf(stderr, "Simulation failed due to io_fault.\n");
    return 1;
  }

  fprintf(stderr, "Simulation reached cycle limit safety net (hang) after %lu instructions.\n", testbench.instruction_count);
  return 124;
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
