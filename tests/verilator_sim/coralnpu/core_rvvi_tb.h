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

#ifndef TESTS_VERILATOR_SIM_CORALNPU_CORE_RVVI_TB_H_
#define TESTS_VERILATOR_SIM_CORALNPU_CORE_RVVI_TB_H_

#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "tests/verilator_sim/coralnpu/coralnpu_cfg.h"
#include "tests/verilator_sim/coralnpu/core_if.h"
#include "tests/verilator_sim/coralnpu/debug_if.h"
#include "tests/verilator_sim/coralnpu/elf_loader.h"
#include "tests/verilator_sim/rvvi/mpact_trace_formatter.h"
#include "tests/verilator_sim/rvvi/spsc_ring_buffer.h"
#include "tests/verilator_sim/rvvi/trace_daemon.h"
#include "tests/verilator_sim/sysc_tb.h"
#include "tests/verilator_sim/util.h"

#define STRINGIZE(x) #x
#define STR(x) STRINGIZE(x)
#define MODEL_HEADER_SUFFIX .h
#define MODEL_HEADER STR(VERILATOR_MODEL MODEL_HEADER_SUFFIX)
#include MODEL_HEADER

#define PARAMS_HEADER_PREFIX hdl/chisel/src/coralnpu/
#define PARAMS_HEADER_SUFFIX _parameters.h
#define PARAMS_HEADER STR(PARAMS_HEADER_PREFIX VERILATOR_MODEL PARAMS_HEADER_SUFFIX)
#include PARAMS_HEADER

#undef STRINGIZE
#undef STR
#undef MODEL_HEADER_SUFFIX
#undef MODEL_HEADER
#undef PARAMS_HEADER_PREFIX
#undef PARAMS_HEADER_SUFFIX
#undef PARAMS_HEADER

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 4096
#endif

using namespace coralnpu::sim::rvvi;

struct CoreRvvi_tb : Sysc_tb {
  using Sysc_tb::cycle;
  sc_in<bool> io_halted;
  sc_in<bool> io_fault;
  sc_in<bool> io_ibus_valid;

  uint64_t last_time = 0;
  uint64_t last_delta = 0;
  bool had_deadlock = false;

  sc_event next_delta_evt;

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

  sc_in<bool> io_debug_float_writeAddr_valid;
  sc_in<bool> io_debug_float_writeData_0_valid;
  sc_in<bool> io_debug_float_writeData_1_valid;
  sc_in<sc_bv<5>> io_debug_float_writeAddr_bits;
  sc_in<sc_bv<5>> io_debug_float_writeData_0_bits_addr;
  sc_in<sc_bv<32>> io_debug_float_writeData_0_bits_data;
  sc_in<sc_bv<5>> io_debug_float_writeData_1_bits_addr;
  sc_in<sc_bv<32>> io_debug_float_writeData_1_bits_data;

  // Other debug signals...

  SpscRingBuffer<TracePacket, BUFFER_SIZE>* buffer;
  bool e_sent = false;
  bool ebreak_halt = false;
  bool had_io_fault = false;
  uint32_t internal_v_id = 0;
  uint64_t instruction_count = 0;
  uint64_t instruction_limit = 500000;

  void push_packet(const TracePacket& packet, const char* label) {
    auto start = std::chrono::steady_clock::now();
    while (!buffer->Push(packet)) {
      std::this_thread::yield();
      auto now = std::chrono::steady_clock::now();
      if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() > 5) {
        LOG(ERROR) << "[FATAL] Queue backpressure timeout (" << label << ")! Watchdog triggered.";
        exit(124);
      }
    }
  }

  CoreRvvi_tb(sc_module_name name, int instruction_limit, bool random, SpscRingBuffer<TracePacket, BUFFER_SIZE>* buf)
      : Sysc_tb(name, instruction_limit * 10, random), buffer(buf), instruction_limit(instruction_limit) {
    SC_METHOD(monitor_delta);
    sensitive << clock << next_delta_evt;
  }

  void monitor_delta() {
    uint64_t current_time = sc_time_stamp().value();
    uint64_t current_delta = sc_delta_count();
    if (current_time == last_time) {
      if (current_delta - last_delta > 10000) {
        LOG(ERROR) << absl::StrFormat("[FATAL] Delta cycle deadlock detected! Time: %lu, Delta: %lu", current_time, current_delta);
        had_deadlock = true;
        sc_stop();
        return;
      }
    } else {
      last_time = current_time;
      last_delta = current_delta;
    }

    // Simplified deadlock simulation for testing
    if (absl::GetFlag(FLAGS_simulate_deadlock) && instruction_count > 10) {
      next_delta_evt.notify(SC_ZERO_TIME);
    } else if (sc_pending_activity_at_current_time()) {
      next_delta_evt.notify(SC_ZERO_TIME);
    }
  }

  void posedge() {
    // RVVI trace packet generation logic...
    // This will be moved or adapted in the test
    if (!e_sent) {
      // ... (Lane processing logic from core_rvvi_traced_sim.cc) ...
      // Simplified for header: Assume packet generation based on valid signals
      // and push_packet is called.
    }

    // Simplified instruction count update
    uint64_t retiring_this_cycle = 0;
    if (io_debug_rb_inst_0_valid.read()) retiring_this_cycle++;
    // ... (other lanes) ...
    instruction_count += retiring_this_cycle;

    if (instruction_count >= instruction_limit) {
      sc_stop();
    }

    bool sim_io_fault = absl::GetFlag(FLAGS_simulate_io_fault) && instruction_count > 5;

    if ((io_halted.read() || io_fault.read() || sim_io_fault || ebreak_halt || instruction_count >= instruction_limit) && !e_sent) {
      // ... (E-packet generation) ...
      e_sent = true;
    }

    if (io_fault || sim_io_fault) {
      LOG(ERROR) << "[ERROR] io_fault asserted";
      had_io_fault = true;
      sc_stop();
    }
    if (io_halted.read() || ebreak_halt) {
      sc_stop();
    }
  }
};

#endif  // TESTS_VERILATOR_SIM_CORALNPU_CORE_RVVI_TB_H_
