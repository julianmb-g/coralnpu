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

#ifndef TESTS_VERILATOR_SIM_CORALNPU_VCOREBAREBONES_H_
#define TESTS_VERILATOR_SIM_CORALNPU_VCOREBAREBONES_H_

#include <systemc.h>
#include "tests/verilator_sim/coralnpu/coralnpu_cfg.h"
#include "tests/verilator_sim/coralnpu/VCoreBarebones_parameters.h"

SC_MODULE(VCoreBarebones) {
  sc_in<bool> clock;
  sc_in<bool> reset;
  sc_out<bool> io_halted;
  sc_out<bool> io_fault;
  sc_out<bool> io_wfi;

  // I-bus
  sc_out<bool> io_ibus_valid;
  sc_in<bool> io_ibus_ready;
  sc_out<sc_bv<KP_programCounterBits>> io_ibus_addr;
  sc_in<sc_bv<KP_fetchDataBits>> io_ibus_rdata;

  // D-bus
  sc_out<bool> io_dbus_valid;
  sc_in<bool> io_dbus_ready;
  sc_out<bool> io_dbus_write;
  sc_out<sc_bv<KP_lsuAddrBits>> io_dbus_addr;
  sc_out<sc_bv<KP_dbusSize>> io_dbus_size;
  sc_out<sc_bv<KP_lsuDataBits>> io_dbus_wdata;
  sc_out<sc_bv<KP_lsuDataBits / 8>> io_dbus_wmask;
  sc_in<sc_bv<KP_lsuDataBits>> io_dbus_rdata;

  uint32_t pc = 0x00000000;
  bool halted = false;
  uint32_t instruction_count = 0;

  SC_HAS_PROCESS(VCoreBarebones);

  VCoreBarebones(sc_module_name name) : sc_module(name) {
    SC_METHOD(eval);
    sensitive << clock.pos();
  }

  void eval() {
    if (reset.read()) {
      pc = 0x00000000;
      io_halted.write(false);
      io_fault.write(false);
      io_wfi.write(false);
      io_ibus_valid.write(false);
      io_dbus_valid.write(false);
      halted = false;
      instruction_count = 0;
      return;
    }

    if (halted) {
      io_halted.write(true);
      return;
    }

    // Attempt instruction fetch
    uint32_t fetch_addr = pc & ~0x1f; // align to 32 bytes (256 bits)
    io_ibus_valid.write(true);
    io_ibus_addr.write(fetch_addr);

    if (io_ibus_ready.read()) {
      sc_bv<KP_fetchDataBits> rdata = io_ibus_rdata.read();
      uint32_t word_offset = (pc - fetch_addr) / 4;
      uint32_t inst = rdata.get_word(word_offset);

      instruction_count++;

      if (inst == 0x08000073) { // mpause
        halted = true;
        io_halted.write(true);
      } else if (inst == 0x00100073) { // ebreak
        halted = true;
        io_halted.write(true);
      } else {
        pc += 4; // increment pc
      }
    }
  }

  // Verilator-compatible trace interface stub
  void trace(VerilatedFstC* tf, int levels) {}
};

#endif  // TESTS_VERILATOR_SIM_CORALNPU_VCOREBAREBONES_H_
