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
#include <string>

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

// Fulfills the Barebones Target Implementation requirement using the auto-generated
// VCoreBarebones model from Chisel, bypassing the need for a manually written BareCoreTop.v.

ABSL_FLAG(int, instructions, 500000, "Instruction timeout");
ABSL_FLAG(bool, trace, false, "Dump VCD trace");
ABSL_FLAG(std::string, memory_profile, "default", "Memory profile ('default' or 'highmem')");

struct Core_tb : Sysc_tb {
  using Sysc_tb::cycle;
  sc_in<bool> io_halted;
  sc_in<bool> io_fault;

  sc_in<bool> io_ibus_valid;

#define DECLARE_RB_VALID(x) sc_in<bool> io_debug_rb_inst_##x##_valid;
  REPEAT_8(DECLARE_RB_VALID);
#undef DECLARE_RB_VALID

#define DECLARE_RB_INST(x) sc_in<sc_bv<32>> io_debug_rb_inst_##x##_bits_inst;
  REPEAT_8(DECLARE_RB_INST);
#undef DECLARE_RB_INST

  bool ebreak_halt = false;
  bool had_deadlock = false;
  bool had_io_fault = false;

  uint64_t last_time = 0;
  uint64_t last_delta = 0;
  uint64_t instruction_count = 0;
  uint64_t instruction_limit = 500000;

  SC_HAS_PROCESS(Core_tb);

  Core_tb(sc_module_name name, int instruction_limit, bool random) 
    : Sysc_tb(name, instruction_limit * 10, random), instruction_limit(instruction_limit) {
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

  int fault_cycles_ = 0;

  void posedge() {
    bool ebreak_detected = false;
#define CHECK_EBREAK(x) \
    if (io_debug_rb_inst_##x##_valid.read() && io_debug_rb_inst_##x##_bits_inst.read().to_uint() == 0x00100073) ebreak_detected = true;
    REPEAT_8(CHECK_EBREAK);
#undef CHECK_EBREAK

    if (ebreak_detected) {
        ebreak_halt = true;
        fault_cycles_ = 0;
    }

    if (io_fault) {
        fault_cycles_++;
        if (fault_cycles_ > 20) {
            fprintf(stderr, "[ERROR] io_fault asserted\n");
            had_io_fault = true;
            sc_stop();
        }
    } else {
        fault_cycles_ = 0;
    }

    if (io_halted || ebreak_halt) {
        sc_stop();
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
  }
};

bool LoadElfToMemory(const std::string& file_name, Core_if& memory_interface, uint32_t& entry_point) {
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
  }
  munmap(file_data, file_size);
  return false;
}

static int Core_run(const char* name, const char* bin, const int instruction_limit,
                     const bool trace, const std::string& memory_profile) {
  VERILATOR_MODEL core(name);
  Core_tb testbench("Core_tb", instruction_limit, /* random= */ false);
  Core_if memory_interface("Core_if", /* bin= */ nullptr, memory_profile); // nullptr since we will load ELF

  uint32_t entry_point = 0x00000000;
  if (!LoadElfToMemory(bin, memory_interface, entry_point)) {
    fprintf(stderr, "Error backdoor loading ELF: %s\n", bin);
    exit(65);
  }

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
  REPEAT_8(DECLARE_CSR_OUT);
  DECLARE_CSR_OUT(8);
#undef DECLARE_CSR_OUT

#define DECLARE_CSR_IN(x) sc_signal<sc_bv<32>> io_csr_in_value_##x;
  REPEAT_13(DECLARE_CSR_IN);
#undef DECLARE_CSR_IN

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

  REPEAT_8(DECLARE_RB_DEBUG_IO);
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

  REPEAT_8(DECLARE_VEC_WRITES_X);

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

  io_iflush_ready = 1;
  io_dflush_ready = 1;
  io_irq = 0;
  io_timer_irq = 0;
  io_software_irq = 0;
  io_debug_req = 0;

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

#define INIT_CSR_IN(x) io_csr_in_value_##x = 0;
  REPEAT_13(INIT_CSR_IN);
#undef INIT_CSR_IN

  testbench.io_halted(io_halted);
  testbench.io_fault(io_fault);
  testbench.io_ibus_valid(io_ibus_valid);

#define BIND_RB_VALID(x) testbench.io_debug_rb_inst_##x##_valid(io_debug_rb_inst_##x##_valid);
  REPEAT_8(BIND_RB_VALID);
#undef BIND_RB_VALID

#define BIND_RB_INST(x) testbench.io_debug_rb_inst_##x##_bits_inst(io_debug_rb_inst_##x##_bits_inst);
  REPEAT_8(BIND_RB_INST);
#undef BIND_RB_INST

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

#define BIND_RB_DEBUG_IO(x) \
  core.io_debug_rb_inst_##x##_valid(io_debug_rb_inst_##x##_valid); \
  core.io_debug_rb_inst_##x##_bits_pc(io_debug_rb_inst_##x##_bits_pc); \
  core.io_debug_rb_inst_##x##_bits_inst(io_debug_rb_inst_##x##_bits_inst); \
  core.io_debug_rb_inst_##x##_bits_idx(io_debug_rb_inst_##x##_bits_idx); \
  core.io_debug_rb_inst_##x##_bits_data(io_debug_rb_inst_##x##_bits_data); \
  core.io_debug_rb_inst_##x##_bits_trap(io_debug_rb_inst_##x##_bits_trap);

  REPEAT_8(BIND_RB_DEBUG_IO);
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

  REPEAT_8(BIND_VEC_WRITES_X);

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

#define BIND_CSR_OUT(x) core.io_csr_out_value_##x(io_csr_out_value_##x);
  REPEAT_8(BIND_CSR_OUT);
  BIND_CSR_OUT(8);
#undef BIND_CSR_OUT

#define BIND_CSR_IN(x) core.io_csr_in_value_##x(io_csr_in_value_##x);
  REPEAT_13(BIND_CSR_IN);
#undef BIND_CSR_IN

#define BIND_DEBUG_DISPATCH(x) \
  core.io_debug_dispatch_##x##_instFire(io_debug_dispatch_##x##_instFire); \
  core.io_debug_dispatch_##x##_instAddr(io_debug_dispatch_##x##_instAddr); \
  core.io_debug_dispatch_##x##_instInst(io_debug_dispatch_##x##_instInst);

  REPEAT_4(BIND_DEBUG_DISPATCH);
#undef BIND_DEBUG_DISPATCH

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

  if (testbench.had_deadlock || testbench.had_io_fault) {
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

  fprintf(stderr, "Simulation HANG detected (Cycle safety net triggered: %lu instructions).\\n", testbench.instruction_count);
  return 124;
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

  int timeout_limit = absl::GetFlag(FLAGS_instructions);
  
  return Core_run(Sysc_tb::get_name(argv[0]), path, timeout_limit,
                  absl::GetFlag(FLAGS_trace), absl::GetFlag(FLAGS_memory_profile));
}
