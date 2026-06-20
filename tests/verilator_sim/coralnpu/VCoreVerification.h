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

  uint32_t pc = 0x80000000;
  bool halted = false;
  uint32_t instruction_count = 0;
  int cycle_count;

  void eval() {
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
      cycle_count = 0;
      halted = false;
      instruction_count = 0;
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

      // Emit trace
      io_debug_rb_inst_0_valid.write(true);
      io_debug_rb_inst_0_bits_pc.write(pc);
      io_debug_rb_inst_0_bits_inst.write(inst);
      io_debug_rb_inst_0_bits_idx.write(instruction_count % 8);
      io_debug_rb_inst_0_bits_data.write(0);
      io_debug_rb_inst_0_bits_trap.write(false);

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
