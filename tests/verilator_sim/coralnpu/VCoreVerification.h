#ifndef TESTS_VERILATOR_SIM_CORALNPU_VCOREVERIFICATION_H_
#define TESTS_VERILATOR_SIM_CORALNPU_VCOREVERIFICATION_H_

#include <cstdint>
#include <systemc.h>
#include <cstdio>

// Define constants based on core_if.h and coralnpu_cfg.h
#define KP_programCounterBits 32
#define KP_fetchDataBits 256
#define KP_lsuAddrBits 32
#define KP_lsuDataBits 256
#define KP_dbusSize 4
#define KP_retirementBufferIdxWidth 7
#define KP_xlen 32

SC_MODULE(VCoreVerification) {
  sc_in<bool> clock;
  sc_in<bool> reset;

  sc_out<bool> io_halted;
  sc_out<bool> io_fault;
  sc_out<bool> io_wfi;

  sc_out<bool> io_ibus_valid;
  sc_in<bool> io_ibus_ready;
  sc_out<sc_bv<KP_programCounterBits>> io_ibus_addr;
  sc_in<sc_bv<KP_fetchDataBits>> io_ibus_rdata;

  sc_out<bool> io_dbus_valid;
  sc_in<bool> io_dbus_ready;
  sc_out<bool> io_dbus_write;
  sc_out<sc_bv<KP_lsuAddrBits>> io_dbus_addr;
  sc_out<sc_bv<KP_dbusSize>> io_dbus_size;
  sc_out<sc_bv<KP_lsuDataBits>> io_dbus_wdata;
  sc_out<sc_bv<KP_lsuDataBits / 8>> io_dbus_wmask;
  sc_in<sc_bv<KP_lsuDataBits>> io_dbus_rdata;

  sc_in<bool> io_ibus_fault_valid;
  sc_in<bool> io_ibus_fault_bits_write;
  sc_in<sc_bv<KP_programCounterBits>> io_ibus_fault_bits_addr;
  sc_in<sc_bv<KP_programCounterBits>> io_ibus_fault_bits_epc;
  sc_out<sc_bv<KP_lsuAddrBits>> io_dbus_adrx;

  // RVVI Trace Ports (Aligned with real RTL io_debug_rb_...)
  sc_out<bool> io_debug_rb_inst_0_valid;
  sc_out<sc_bv<KP_programCounterBits>> io_debug_rb_inst_0_bits_pc;
  sc_out<sc_bv<32>> io_debug_rb_inst_0_bits_inst;
  sc_out<sc_bv<KP_retirementBufferIdxWidth>> io_debug_rb_inst_0_bits_idx;
  sc_out<sc_bv<KP_xlen>> io_debug_rb_inst_0_bits_data;
  sc_out<bool> io_debug_rb_inst_0_bits_trap;
  sc_out<bool> io_trace_halt; // To simulate mpause (Temporary, to be removed)

  sc_out<bool> io_debug_rb_inst_0_bits_vecWrites_0_valid;
  sc_out<sc_bv<32>> io_debug_rb_inst_0_bits_vecWrites_0_data;
  sc_out<sc_bv<5>> io_debug_rb_inst_0_bits_vecWrites_0_idx;
  sc_out<bool> io_debug_rb_inst_0_bits_vecWrites_1_valid;
  sc_out<sc_bv<32>> io_debug_rb_inst_0_bits_vecWrites_1_data;
  sc_out<sc_bv<5>> io_debug_rb_inst_0_bits_vecWrites_1_idx;
  sc_out<bool> io_debug_rb_inst_0_bits_vecWrites_2_valid;
  sc_out<sc_bv<32>> io_debug_rb_inst_0_bits_vecWrites_2_data;
  sc_out<sc_bv<5>> io_debug_rb_inst_0_bits_vecWrites_2_idx;
  sc_out<bool> io_debug_rb_inst_0_bits_vecWrites_3_valid;
  sc_out<sc_bv<32>> io_debug_rb_inst_0_bits_vecWrites_3_data;
  sc_out<sc_bv<5>> io_debug_rb_inst_0_bits_vecWrites_3_idx;
  sc_out<bool> io_debug_rb_inst_0_bits_vecWrites_4_valid;
  sc_out<sc_bv<32>> io_debug_rb_inst_0_bits_vecWrites_4_data;
  sc_out<sc_bv<5>> io_debug_rb_inst_0_bits_vecWrites_4_idx;
  sc_out<bool> io_debug_rb_inst_0_bits_vecWrites_5_valid;
  sc_out<sc_bv<32>> io_debug_rb_inst_0_bits_vecWrites_5_data;
  sc_out<sc_bv<5>> io_debug_rb_inst_0_bits_vecWrites_5_idx;
  sc_out<bool> io_debug_rb_inst_0_bits_vecWrites_6_valid;
  sc_out<sc_bv<32>> io_debug_rb_inst_0_bits_vecWrites_6_data;
  sc_out<sc_bv<5>> io_debug_rb_inst_0_bits_vecWrites_6_idx;
  sc_out<bool> io_debug_rb_inst_0_bits_vecWrites_7_valid;
  sc_out<sc_bv<32>> io_debug_rb_inst_0_bits_vecWrites_7_data;
  sc_out<sc_bv<5>> io_debug_rb_inst_0_bits_vecWrites_7_idx;


  uint32_t pc = 0x80000000;
  bool halted = false;
  uint32_t instruction_count = 0;
  int cycle_count;
  uint32_t gpr[32] = {0};

  void eval() {
    if (!clock.read() && !reset.read()) return;
    if (reset.read()) {
      pc = 0x80000000;
      io_halted.write(false);
      io_fault.write(false);
      io_wfi.write(false);
      io_ibus_valid.write(false);
      io_dbus_valid.write(false);
      io_dbus_adrx.write(0);
      io_debug_rb_inst_0_valid.write(false);
      io_debug_rb_inst_0_bits_pc.write(0);
      io_debug_rb_inst_0_bits_inst.write(0);
      io_debug_rb_inst_0_bits_idx.write(0);
      io_debug_rb_inst_0_bits_data.write(0);
      io_debug_rb_inst_0_bits_trap.write(false);
      io_trace_halt.write(false);
      io_debug_rb_inst_0_bits_vecWrites_0_valid.write(false);
      io_debug_rb_inst_0_bits_vecWrites_0_data.write(0);
      io_debug_rb_inst_0_bits_vecWrites_0_idx.write(0);
      io_debug_rb_inst_0_bits_vecWrites_1_valid.write(false);
      io_debug_rb_inst_0_bits_vecWrites_1_data.write(0);
      io_debug_rb_inst_0_bits_vecWrites_1_idx.write(0);
      io_debug_rb_inst_0_bits_vecWrites_2_valid.write(false);
      io_debug_rb_inst_0_bits_vecWrites_2_data.write(0);
      io_debug_rb_inst_0_bits_vecWrites_2_idx.write(0);
      io_debug_rb_inst_0_bits_vecWrites_3_valid.write(false);
      io_debug_rb_inst_0_bits_vecWrites_3_data.write(0);
      io_debug_rb_inst_0_bits_vecWrites_3_idx.write(0);
      io_debug_rb_inst_0_bits_vecWrites_4_valid.write(false);
      io_debug_rb_inst_0_bits_vecWrites_4_data.write(0);
      io_debug_rb_inst_0_bits_vecWrites_4_idx.write(0);
      io_debug_rb_inst_0_bits_vecWrites_5_valid.write(false);
      io_debug_rb_inst_0_bits_vecWrites_5_data.write(0);
      io_debug_rb_inst_0_bits_vecWrites_5_idx.write(0);
      io_debug_rb_inst_0_bits_vecWrites_6_valid.write(false);
      io_debug_rb_inst_0_bits_vecWrites_6_data.write(0);
      io_debug_rb_inst_0_bits_vecWrites_6_idx.write(0);
      io_debug_rb_inst_0_bits_vecWrites_7_valid.write(false);
      io_debug_rb_inst_0_bits_vecWrites_7_data.write(0);
      io_debug_rb_inst_0_bits_vecWrites_7_idx.write(0);
      cycle_count = 0;
      halted = false;
      instruction_count = 0;
      for (int i = 0; i < 32; i++) {
        gpr[i] = 0;
      }
      return;
    }

    if (halted) {
      io_halted.write(true);
      return;
    }

    if (io_ibus_fault_valid.read()) {
      io_fault.write(true);
      halted = true;
      io_halted.write(true);
      return;
    }

    cycle_count++;

    // Attempt instruction fetch
    uint32_t fetch_addr = pc & ~0x1f; // align to 32 bytes (256 bits)
    io_ibus_valid.write(true);
    io_ibus_addr.write(fetch_addr);
    io_dbus_adrx.write(io_dbus_addr.read());

    if (io_ibus_ready.read()) {
      sc_bv<KP_fetchDataBits> rdata = io_ibus_rdata.read();
      uint32_t word_offset = (pc - fetch_addr) / 4;
      uint32_t inst = rdata.get_word(word_offset);

      instruction_count++;

      // Decode instruction to track register updates
      uint32_t write_val = 0;
      uint32_t opcode = inst & 0x7f;
      uint32_t funct3 = (inst >> 12) & 0x7;
      uint32_t rd = (inst >> 7) & 0x1f;
      if (opcode == 0x13 && funct3 == 0x0) { // ADDI
        uint32_t rs1 = (inst >> 15) & 0x1f;
        int32_t imm = (int32_t)inst >> 20;
        uint32_t val1 = (rs1 == 0) ? 0 : gpr[rs1];
        write_val = val1 + imm;
        if (rd != 0) gpr[rd] = write_val;
      } else if (opcode == 0x37) { // LUI
        write_val = inst & 0xfffff000;
        if (rd != 0) gpr[rd] = write_val;
      } else if (opcode == 0x17) { // AUIPC
        write_val = pc + (inst & 0xfffff000);
        if (rd != 0) gpr[rd] = write_val;
      } else {
        if (rd != 0) write_val = gpr[rd];
      }

      // Emit trace
      io_debug_rb_inst_0_valid.write(true);
      io_debug_rb_inst_0_bits_pc.write(pc);
      io_debug_rb_inst_0_bits_inst.write(inst);
      io_debug_rb_inst_0_bits_idx.write(instruction_count % 8);
      io_debug_rb_inst_0_bits_data.write(write_val);
      io_debug_rb_inst_0_bits_trap.write(false);

      if (opcode == 0x57) {
        uint32_t val_0 = (rd << 24) | (0 << 16) | 0x5757;
        io_debug_rb_inst_0_bits_vecWrites_0_valid.write(true);
        io_debug_rb_inst_0_bits_vecWrites_0_data.write(val_0);
        io_debug_rb_inst_0_bits_vecWrites_0_idx.write(rd);
        uint32_t val_1 = (rd << 24) | (1 << 16) | 0x5757;
        io_debug_rb_inst_0_bits_vecWrites_1_valid.write(true);
        io_debug_rb_inst_0_bits_vecWrites_1_data.write(val_1);
        io_debug_rb_inst_0_bits_vecWrites_1_idx.write(rd);
        uint32_t val_2 = (rd << 24) | (2 << 16) | 0x5757;
        io_debug_rb_inst_0_bits_vecWrites_2_valid.write(true);
        io_debug_rb_inst_0_bits_vecWrites_2_data.write(val_2);
        io_debug_rb_inst_0_bits_vecWrites_2_idx.write(rd);
        uint32_t val_3 = (rd << 24) | (3 << 16) | 0x5757;
        io_debug_rb_inst_0_bits_vecWrites_3_valid.write(true);
        io_debug_rb_inst_0_bits_vecWrites_3_data.write(val_3);
        io_debug_rb_inst_0_bits_vecWrites_3_idx.write(rd);
        uint32_t val_4 = (rd << 24) | (4 << 16) | 0x5757;
        io_debug_rb_inst_0_bits_vecWrites_4_valid.write(true);
        io_debug_rb_inst_0_bits_vecWrites_4_data.write(val_4);
        io_debug_rb_inst_0_bits_vecWrites_4_idx.write(rd);
        uint32_t val_5 = (rd << 24) | (5 << 16) | 0x5757;
        io_debug_rb_inst_0_bits_vecWrites_5_valid.write(true);
        io_debug_rb_inst_0_bits_vecWrites_5_data.write(val_5);
        io_debug_rb_inst_0_bits_vecWrites_5_idx.write(rd);
        uint32_t val_6 = (rd << 24) | (6 << 16) | 0x5757;
        io_debug_rb_inst_0_bits_vecWrites_6_valid.write(true);
        io_debug_rb_inst_0_bits_vecWrites_6_data.write(val_6);
        io_debug_rb_inst_0_bits_vecWrites_6_idx.write(rd);
        uint32_t val_7 = (rd << 24) | (7 << 16) | 0x5757;
        io_debug_rb_inst_0_bits_vecWrites_7_valid.write(true);
        io_debug_rb_inst_0_bits_vecWrites_7_data.write(val_7);
        io_debug_rb_inst_0_bits_vecWrites_7_idx.write(rd);
      } else {
        io_debug_rb_inst_0_bits_vecWrites_0_valid.write(false);
        io_debug_rb_inst_0_bits_vecWrites_1_valid.write(false);
        io_debug_rb_inst_0_bits_vecWrites_2_valid.write(false);
        io_debug_rb_inst_0_bits_vecWrites_3_valid.write(false);
        io_debug_rb_inst_0_bits_vecWrites_4_valid.write(false);
        io_debug_rb_inst_0_bits_vecWrites_5_valid.write(false);
        io_debug_rb_inst_0_bits_vecWrites_6_valid.write(false);
        io_debug_rb_inst_0_bits_vecWrites_7_valid.write(false);
      }

      if (inst == 0x08000073) { // mpause
        halted = true;
        io_halted.write(true);
        io_trace_halt.write(true);
      } else if (inst == 0x00100073) { // ebreak
        halted = true;
        io_halted.write(true);
        io_trace_halt.write(true);
      } else {
        pc += 4; // increment pc
        io_trace_halt.write(false);
      }
    } else {
      io_debug_rb_inst_0_valid.write(false);
      io_trace_halt.write(false);
    }
  }

  SC_CTOR(VCoreVerification) {
    cycle_count = 0;
    SC_METHOD(process);
    sensitive << clock.pos();
  }
};

#endif  // TESTS_VERILATOR_SIM_CORALNPU_VCOREVERIFICATION_H_
