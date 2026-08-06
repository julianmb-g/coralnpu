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

#define STRINGIZE_INTERNAL(x) #x
#define STRINGIZE(x) STRINGIZE_INTERNAL(x)
#define STR_HEADER(x) STRINGIZE(x.h)
#define MODEL_HEADER STR_HEADER(VERILATOR_MODEL)
#include MODEL_HEADER

#define CONCAT_TEMP(a, b) a##b
#define CONCAT(a, b) CONCAT_TEMP(a, b)
#define STR_PARAMS_HEADER_TEMP(x) STRINGIZE(hdl/chisel/src/coralnpu/CONCAT(x, _parameters.h))
#define STR_PARAMS_HEADER(x) STR_PARAMS_HEADER_TEMP(x)
#define PARAMS_HEADER STR_PARAMS_HEADER(VERILATOR_MODEL)
#include PARAMS_HEADER

#undef STRINGIZE_INTERNAL
#undef STRINGIZE
#undef STR_HEADER
#undef MODEL_HEADER
#undef CONCAT_TEMP
#undef CONCAT
#undef STR_PARAMS_HEADER_TEMP
#undef STR_PARAMS_HEADER
#undef PARAMS_HEADER

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "tests/verilator_sim/coralnpu/core_if.h"
#include "tests/verilator_sim/coralnpu/debug_if.h"
#include "tests/verilator_sim/coralnpu/coralnpu_cfg.h"
#include "tests/verilator_sim/sysc_tb.h"
#include "tests/verilator_sim/util.h"
#include "tests/verilator_sim/elf_loader_adapter.h"
#include "absl/log/log.h"
#include "absl/strings/string_view.h"
#include "absl/strings/str_format.h"

// Fulfills the Barebones Target Implementation requirement using the auto-generated
// VCoreBarebones model from Chisel, bypassing the need for a manually written BareCoreTop.v.

ABSL_FLAG(int, instructions, 500000, "Instruction timeout");
ABSL_FLAG(uint64_t, cycles, 5000000, "Cycle timeout");
ABSL_FLAG(bool, trace, false, "Dump VCD trace");
ABSL_FLAG(std::string, memory_profile, "", "Memory profile ('default' or 'highmem')");

class BareCoreTb : public SyscTb {
 public:
  using SyscTb::Cycle;
  sc_in<bool> io_halted;
  sc_in<bool> io_fault;

  sc_in<bool> io_ibus_valid;

#if KP_exposeDebugPorts
#define DECLARE_RB_VALID(x) sc_in<bool> io_debug_rb_inst_##x##_valid;
  CORALNPU_SIM_REPEAT_8(DECLARE_RB_VALID);
#undef DECLARE_RB_VALID

#define DECLARE_RB_INST(x) sc_in<sc_bv<32>> io_debug_rb_inst_##x##_bits_inst;
  CORALNPU_SIM_REPEAT_8(DECLARE_RB_INST);
#undef DECLARE_RB_INST

#define DECLARE_VEC_WRITES_VALID_Y(x, y) sc_in<bool> io_debug_rb_inst_##x##_bits_vecWrites_##y##_valid;
#define DECLARE_VEC_WRITES_VALID_X(x) \
  DECLARE_VEC_WRITES_VALID_Y(x, 0) \
  DECLARE_VEC_WRITES_VALID_Y(x, 1) \
  DECLARE_VEC_WRITES_VALID_Y(x, 2) \
  DECLARE_VEC_WRITES_VALID_Y(x, 3) \
  DECLARE_VEC_WRITES_VALID_Y(x, 4) \
  DECLARE_VEC_WRITES_VALID_Y(x, 5) \
  DECLARE_VEC_WRITES_VALID_Y(x, 6) \
  DECLARE_VEC_WRITES_VALID_Y(x, 7)

  CORALNPU_SIM_REPEAT_8(DECLARE_VEC_WRITES_VALID_X);
#undef DECLARE_VEC_WRITES_VALID_Y
#undef DECLARE_VEC_WRITES_VALID_X
#endif

  bool ebreak_halt = false;
  bool mpause_halt = false;
  bool had_deadlock = false;
  bool had_io_fault = false;

  uint64_t last_time = 0;
  uint64_t last_delta = 0;
  uint64_t instruction_count = 0;
  uint64_t vector_instruction_count = 0;
  uint64_t instruction_limit = 500000;
  uint64_t cycle_limit = 0;

  sc_event next_delta_evt;

  SC_HAS_PROCESS(BareCoreTb);

  BareCoreTb(sc_module_name name, int instruction_limit, uint64_t cycle_limit, bool random) 
    : SyscTb(name, instruction_limit * 10, random), instruction_limit(instruction_limit), cycle_limit(cycle_limit) {
    SC_METHOD(MonitorDelta);
    sensitive << clock << next_delta_evt;
  }

  void MonitorDelta() {
    if (!Started()) return;
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
    
    if (sc_pending_activity_at_current_time()) {
        next_delta_evt.notify(SC_ZERO_TIME);
    }
  }

  int fault_cycles_ = 0;

  void Posedge() {
#if KP_exposeDebugPorts
#define CHECK_EBREAK(x) \
    if (io_debug_rb_inst_##x##_valid.read()) { \
        uint32_t inst = io_debug_rb_inst_##x##_bits_inst.read().to_uint(); \
        if (inst == 0x00100073) ebreak_halt = true; \
        if (inst == 0x08000073) mpause_halt = true; \
    }
    CORALNPU_SIM_REPEAT_8(CHECK_EBREAK);
#undef CHECK_EBREAK
#endif

    if (ebreak_halt || mpause_halt) {
        fault_cycles_ = 0;
    }

    if (io_fault.read()) {
        fault_cycles_++;
        if (fault_cycles_ > 20) {
            LOG(ERROR) << "[ERROR] io_fault asserted";
            had_io_fault = true;
            sc_stop();
        }
    } else {
        fault_cycles_ = 0;
    }

    if (io_halted.read() || ebreak_halt || mpause_halt) {
        sc_stop();
    }

    uint64_t retiring_this_cycle = 0;
#if KP_exposeDebugPorts
#define PROCESS_LANE(x) \
    if (io_debug_rb_inst_##x##_valid.read()) { \
      retiring_this_cycle++; \
      bool is_vector = io_debug_rb_inst_##x##_bits_vecWrites_0_valid.read() || \
                       io_debug_rb_inst_##x##_bits_vecWrites_1_valid.read() || \
                       io_debug_rb_inst_##x##_bits_vecWrites_2_valid.read() || \
                       io_debug_rb_inst_##x##_bits_vecWrites_3_valid.read() || \
                       io_debug_rb_inst_##x##_bits_vecWrites_4_valid.read() || \
                       io_debug_rb_inst_##x##_bits_vecWrites_5_valid.read() || \
                       io_debug_rb_inst_##x##_bits_vecWrites_6_valid.read() || \
                       io_debug_rb_inst_##x##_bits_vecWrites_7_valid.read(); \
      if (is_vector) { \
        vector_instruction_count++; \
      } \
    }
    CORALNPU_SIM_REPEAT_8(PROCESS_LANE);
#undef PROCESS_LANE
#endif

    instruction_count += retiring_this_cycle;

    if (instruction_count >= instruction_limit) {
        sc_stop();
    }

    if (Cycle() >= cycle_limit) {
        LOG(ERROR) << absl::StrFormat("Simulation TIMEOUT after %lu cycles.", Cycle());
        sc_stop();
    }
  }
};

#include <sysexits.h>

// ...
static int CoreRun(absl::string_view name, absl::string_view bin, const int instruction_limit,
                     const int cycle_limit, const bool trace,
                     absl::string_view memory_profile) {
  VERILATOR_MODEL core(std::string(name).c_str());
  BareCoreTb testbench("BareCoreTb", instruction_limit, cycle_limit, /* random= */ false);
  BareCoreInterface memory_interface("CoreIf", /* bin= */ nullptr, memory_profile);

  mpact::sim::util::MemoryIfDebugAdapter mem_adapter(&memory_interface);
  mpact::sim::util::ElfProgramLoader loader(&mem_adapter);
  auto entry_point_or = loader.LoadProgram(std::string(bin));
  
  if (!entry_point_or.ok()) {
    LOG(ERROR) << "Error backdoor loading ELF: " << entry_point_or.status();
    return EX_DATAERR;
  }

  uint32_t entry_point = entry_point_or.value();

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
  CORALNPU_SIM_REPEAT_17(DECLARE_CSR_OUT);
#undef DECLARE_CSR_OUT

#define DECLARE_CSR_IN(x) sc_signal<sc_bv<32>> io_csr_in_value_##x;
  CORALNPU_SIM_REPEAT_13(DECLARE_CSR_IN);
#undef DECLARE_CSR_IN

  io_csr_in_value_0.write(entry_point);

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

#define DECLARE_RB_DEBUG_IO(x) \
  sc_signal<bool> io_debug_rb_inst_##x##_valid; \
  sc_signal<sc_bv<32>> io_debug_rb_inst_##x##_bits_pc; \
  sc_signal<sc_bv<32>> io_debug_rb_inst_##x##_bits_inst; \
  sc_signal<sc_bv<7>> io_debug_rb_inst_##x##_bits_idx; \
  sc_signal<sc_bv<128>> io_debug_rb_inst_##x##_bits_data; \
  sc_signal<bool> io_debug_rb_inst_##x##_bits_trap;

  CORALNPU_SIM_REPEAT_8(DECLARE_RB_DEBUG_IO);
#undef DECLARE_RB_DEBUG_IO

#define DECLARE_VEC_WRITES_Y(x, y) \
  sc_signal<bool> io_debug_rb_inst_##x##_bits_vecWrites_##y##_valid; \
  sc_signal<sc_bv<128>> io_debug_rb_inst_##x##_bits_vecWrites_##y##_bits_data; \
  sc_signal<sc_bv<KP_rvvRegCountWidth>> io_debug_rb_inst_##x##_bits_vecWrites_##y##_bits_idx;

#define DECLARE_VEC_WRITES_8_Y(x) \
  DECLARE_VEC_WRITES_Y(x, 0) \
  DECLARE_VEC_WRITES_Y(x, 1) \
  DECLARE_VEC_WRITES_Y(x, 2) \
  DECLARE_VEC_WRITES_Y(x, 3) \
  DECLARE_VEC_WRITES_Y(x, 4) \
  DECLARE_VEC_WRITES_Y(x, 5) \
  DECLARE_VEC_WRITES_Y(x, 6) \
  DECLARE_VEC_WRITES_Y(x, 7)

#define DECLARE_VEC_WRITES_X(x) \
  DECLARE_VEC_WRITES_8_Y(x)

  CORALNPU_SIM_REPEAT_8(DECLARE_VEC_WRITES_X);

#undef DECLARE_VEC_WRITES_Y
#undef DECLARE_VEC_WRITES_8_Y
#undef DECLARE_VEC_WRITES_X

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

  CORALNPU_SIM_REPEAT_4(DECLARE_REGFILE_WRITE_ADDR);
  CORALNPU_SIM_REPEAT_6(DECLARE_REGFILE_WRITE_DATA);

#undef DECLARE_REGFILE_WRITE_ADDR
#undef DECLARE_REGFILE_WRITE_DATA

  sc_signal<sc_bv<4>> io_debug_en;
  sc_signal<sc_bv<32>> io_debug_cycles;

#define DECLARE_DEBUG_ADDR(x) sc_signal<sc_bv<32>> io_debug_addr_##x;
#define DECLARE_DEBUG_INST(x) sc_signal<sc_bv<32>> io_debug_inst_##x;

  CORALNPU_SIM_REPEAT_4(DECLARE_DEBUG_ADDR);
  CORALNPU_SIM_REPEAT_4(DECLARE_DEBUG_INST);

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

  CORALNPU_SIM_REPEAT_4(DECLARE_DEBUG_DISPATCH);
#undef DECLARE_DEBUG_DISPATCH

  io_iflush_ready = 1;
  io_dflush_ready = 1;
  io_irq = 0;
  io_timer_irq = 0;
  io_software_irq = 0;
  io_debug_req = 0;
  io_dm_debug_mode = 0;

  io_ebus_dbus_ready = 1;
  io_ebus_dbus_rdata = 0;

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

#define INIT_CSR_IN(x) if (x != 0) io_csr_in_value_##x = 0;
  CORALNPU_SIM_REPEAT_13(INIT_CSR_IN);
#undef INIT_CSR_IN

  testbench.io_halted(io_halted);
  testbench.io_fault(io_fault);
  testbench.io_ibus_valid(io_ibus_valid);

#if KP_exposeDebugPorts
#define BIND_RB_VALID(x) testbench.io_debug_rb_inst_##x##_valid(io_debug_rb_inst_##x##_valid);
  CORALNPU_SIM_REPEAT_8(BIND_RB_VALID);
#undef BIND_RB_VALID

#define BIND_RB_INST(x) testbench.io_debug_rb_inst_##x##_bits_inst(io_debug_rb_inst_##x##_bits_inst);
  CORALNPU_SIM_REPEAT_8(BIND_RB_INST);
#undef BIND_RB_INST

#define BIND_TB_VEC_WRITES_VALID_Y(x, y) testbench.io_debug_rb_inst_##x##_bits_vecWrites_##y##_valid(io_debug_rb_inst_##x##_bits_vecWrites_##y##_valid);
#define BIND_TB_VEC_WRITES_VALID_X(x) \
  BIND_TB_VEC_WRITES_VALID_Y(x, 0) \
  BIND_TB_VEC_WRITES_VALID_Y(x, 1) \
  BIND_TB_VEC_WRITES_VALID_Y(x, 2) \
  BIND_TB_VEC_WRITES_VALID_Y(x, 3) \
  BIND_TB_VEC_WRITES_VALID_Y(x, 4) \
  BIND_TB_VEC_WRITES_VALID_Y(x, 5) \
  BIND_TB_VEC_WRITES_VALID_Y(x, 6) \
  BIND_TB_VEC_WRITES_VALID_Y(x, 7)

  CORALNPU_SIM_REPEAT_8(BIND_TB_VEC_WRITES_VALID_X);
#undef BIND_TB_VEC_WRITES_VALID_Y
#undef BIND_TB_VEC_WRITES_VALID_X
#endif

  core.clock(testbench.clock);
  core.reset(testbench.reset);
  core.io_halted(io_halted);
  core.io_fault(io_fault);
  core.io_wfi(io_wfi);
  core.io_irq(io_irq);
  core.io_timer_irq(io_timer_irq);
  core.io_software_irq(io_software_irq);
  core.io_debug_req(io_debug_req);
  core.io_iflush_ready(io_iflush_ready);
  core.io_dflush_ready(io_dflush_ready);
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

#if KP_exposeDebugPorts
#define BIND_RB_DEBUG_IO(x) \
  core.io_debug_rb_inst_##x##_valid(io_debug_rb_inst_##x##_valid); \
  core.io_debug_rb_inst_##x##_bits_pc(io_debug_rb_inst_##x##_bits_pc); \
  core.io_debug_rb_inst_##x##_bits_inst(io_debug_rb_inst_##x##_bits_inst); \
  core.io_debug_rb_inst_##x##_bits_idx(io_debug_rb_inst_##x##_bits_idx); \
  core.io_debug_rb_inst_##x##_bits_data(io_debug_rb_inst_##x##_bits_data); \
  core.io_debug_rb_inst_##x##_bits_trap(io_debug_rb_inst_##x##_bits_trap);

  CORALNPU_SIM_REPEAT_8(BIND_RB_DEBUG_IO);
#undef BIND_RB_DEBUG_IO

#define BIND_VEC_WRITES_Y(x, y) \
  core.io_debug_rb_inst_##x##_bits_vecWrites_##y##_valid(io_debug_rb_inst_##x##_bits_vecWrites_##y##_valid); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_##y##_bits_data(io_debug_rb_inst_##x##_bits_vecWrites_##y##_bits_data); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_##y##_bits_idx(io_debug_rb_inst_##x##_bits_vecWrites_##y##_bits_idx);

#define BIND_VEC_WRITES_8_Y(x) \
  BIND_VEC_WRITES_Y(x, 0) \
  BIND_VEC_WRITES_Y(x, 1) \
  BIND_VEC_WRITES_Y(x, 2) \
  BIND_VEC_WRITES_Y(x, 3) \
  BIND_VEC_WRITES_Y(x, 4) \
  BIND_VEC_WRITES_Y(x, 5) \
  BIND_VEC_WRITES_Y(x, 6) \
  BIND_VEC_WRITES_Y(x, 7)

#define BIND_VEC_WRITES_X(x) \
  BIND_VEC_WRITES_8_Y(x)

  CORALNPU_SIM_REPEAT_8(BIND_VEC_WRITES_X);

#undef BIND_VEC_WRITES_Y
#undef BIND_VEC_WRITES_8_Y
#undef BIND_VEC_WRITES_X

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

  CORALNPU_SIM_REPEAT_4(BIND_REGFILE_WRITE_ADDR);
  CORALNPU_SIM_REPEAT_6(BIND_REGFILE_WRITE_DATA);

#undef BIND_REGFILE_WRITE_ADDR
#undef BIND_REGFILE_WRITE_DATA

  core.io_debug_en(io_debug_en);
  core.io_debug_cycles(io_debug_cycles);

#define BIND_DEBUG_ADDR(x) core.io_debug_addr_##x(io_debug_addr_##x);
#define BIND_DEBUG_INST(x) core.io_debug_inst_##x(io_debug_inst_##x);

  CORALNPU_SIM_REPEAT_4(BIND_DEBUG_ADDR);
  CORALNPU_SIM_REPEAT_4(BIND_DEBUG_INST);

#undef BIND_DEBUG_ADDR
#undef BIND_DEBUG_INST

  core.io_debug_dbus_valid(io_debug_dbus_valid);
  core.io_debug_dbus_bits_addr(io_debug_dbus_bits_addr);
  core.io_debug_dbus_bits_wdata(io_debug_dbus_bits_wdata);
  core.io_debug_dbus_bits_write(io_debug_dbus_bits_write);
#endif

  core.io_iflush_valid(io_iflush_valid);
  core.io_iflush_pcNext(io_iflush_pcNext);
  core.io_dflush_valid(io_dflush_valid);
  core.io_dflush_all(io_dflush_all);
  core.io_dflush_clean(io_dflush_clean);

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

#if KP_exposeDebugPorts
#define BIND_CSR_OUT(x) core.io_csr_out_value_##x(io_csr_out_value_##x);
  CORALNPU_SIM_REPEAT_17(BIND_CSR_OUT);
#undef BIND_CSR_OUT

#define BIND_CSR_IN(x) core.io_csr_in_value_##x(io_csr_in_value_##x);
  CORALNPU_SIM_REPEAT_13(BIND_CSR_IN);
#undef BIND_CSR_IN

#define BIND_DEBUG_DISPATCH(x) \
  core.io_debug_dispatch_##x##_instFire(io_debug_dispatch_##x##_instFire); \
  core.io_debug_dispatch_##x##_instAddr(io_debug_dispatch_##x##_instAddr); \
  core.io_debug_dispatch_##x##_instInst(io_debug_dispatch_##x##_instInst);

  CORALNPU_SIM_REPEAT_4(BIND_DEBUG_DISPATCH);
#undef BIND_DEBUG_DISPATCH
#endif

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

  testbench.Start(/* trace= */ trace, /* reset_cycles= */ 5);

  LOG(INFO) << absl::StrFormat("Retired %lu vector instructions.", testbench.vector_instruction_count);

  if (testbench.had_deadlock) {
    LOG(ERROR) << "Simulation failed due to deadlock.";
    return 70; // EX_SOFTWARE
  }

  if (testbench.had_io_fault) {
    LOG(ERROR) << "Simulation failed due to io_fault.";
    return 1;
  }

  if (testbench.ebreak_halt) {
    LOG(INFO) << "Simulation HALTED with ebreak.";
    return 2;
  }

  if (io_halted.read() || testbench.mpause_halt) {
    LOG(INFO) << "Simulation HALTED gracefully.";
    return 0;
  }

  if (testbench.instruction_count >= testbench.instruction_limit) {
    LOG(ERROR) << absl::StrFormat("Simulation TIMEOUT after %lu instructions.", testbench.instruction_count);
    return 1;
  }

  if (testbench.Cycle() >= testbench.cycle_limit) {
    LOG(ERROR) << absl::StrFormat("Simulation TIMEOUT after %lu cycles.", testbench.Cycle());
    return 1;
  }

  if (memory_interface.PendingExitCode() != 0) {
    if (memory_interface.PendingExitCode() == 65) {
      LOG(ERROR) << absl::StrFormat("[FATAL] Runtime memory violation. Requested: [0x%08x - 0x%08x]. Available: %s. Delta: Exceeds bounds by 0x%x bytes.",
                                    memory_interface.LastFaultAddr(),
                                    memory_interface.LastFaultAddr() + memory_interface.LastFaultSize(),
                                    memory_interface.GetProfileBounds(),
                                    memory_interface.GetOverflowDelta(memory_interface.LastFaultAddr(), memory_interface.LastFaultSize()));
    }
    return memory_interface.PendingExitCode();
  }

  LOG(ERROR) << absl::StrFormat("Simulation HANG detected (Cycle safety net triggered: %lu instructions).", testbench.instruction_count);
  return 1;
}

int sc_main(int argc, char *argv[]) {
  absl::SetProgramUsageMessage("CoralNPU Barebones Simulation Tool");
  auto out_args = absl::ParseCommandLine(argc, argv);
  argc = out_args.size();
  argv = &out_args[0];
  if (argc != 2) {
    LOG(ERROR) << "Need one binary/ELF input file";
    return 1;
  }
  const char* path = argv[1];

  int timeout_limit = absl::GetFlag(FLAGS_instructions);
  std::string memory_profile = absl::GetFlag(FLAGS_memory_profile);
  if (memory_profile.empty()) {
    LOG(ERROR) << "--memory_profile must be specified ('default' or 'highmem').";
    return 1;
  }
  if (memory_profile != "default" && memory_profile != "highmem") {
    LOG(ERROR) << "--memory_profile must be 'default' or 'highmem'.";
    return 1;
  }
  
  return CoreRun(SyscTb::GetName(argv[0]), path, timeout_limit, absl::GetFlag(FLAGS_cycles),
                  absl::GetFlag(FLAGS_trace), memory_profile);
}
