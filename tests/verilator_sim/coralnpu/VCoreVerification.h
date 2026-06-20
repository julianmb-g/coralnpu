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

  // RVVI Trace Ports (mocked)
  sc_out<bool> io_trace_valid;
  sc_out<sc_bv<64>> io_trace_pc;
  sc_out<sc_bv<32>> io_trace_insn;
  sc_out<bool> io_trace_halt; // To simulate mpause

  uint32_t pc = 0x80000000;
  int cycle_count;

  void eval() {
    if (reset.read()) {
      io_halted.write(false);
      io_fault.write(false);
      io_wfi.write(false);
      io_ibus_valid.write(false);
      io_dbus_valid.write(false);
      io_dbus_adrx.write(0);
      io_trace_valid.write(false);
      cycle_count = 0;
      return;
    }
    
    if (io_ibus_fault_valid.read()) {
      io_fault.write(true);
      io_halted.write(true);
      return;
    }
    
    cycle_count++;
    
    // Simulate emitting instructions
    if (cycle_count == 10) {
      io_trace_valid.write(true);
      io_trace_pc.write(0x1000);
      io_trace_insn.write(0x00000013); // nop
      io_trace_halt.write(false);
    } else if (cycle_count == 20) {
      io_trace_valid.write(true);
      io_trace_pc.write(0x1004);
      io_trace_insn.write(0x08000073); // mpause
      io_trace_halt.write(true);
      io_halted.write(true);
    } else {
      io_trace_valid.write(false);
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
