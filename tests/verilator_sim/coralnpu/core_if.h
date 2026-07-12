// Copyright 2023 Google LLC
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

#ifndef TESTS_VERILATOR_SIM_CORALNPU_CORE_IF_H_
#define TESTS_VERILATOR_SIM_CORALNPU_CORE_IF_H_

#include "tests/verilator_sim/fifo.h"
#include "tests/verilator_sim/coralnpu/coralnpu_cfg.h"
#include "tests/verilator_sim/coralnpu/memory_if.h"

#ifndef KP_programCounterBits
#define KP_programCounterBits 32
#endif

#ifndef KP_fetchDataBits
#define KP_fetchDataBits 256
#endif

#ifndef KP_lsuAddrBits
#define KP_lsuAddrBits 32
#endif

#ifndef KP_lsuDataBits
#define KP_lsuDataBits 256
#endif

#ifndef KP_dbusSize
#define KP_dbusSize 3
#endif

constexpr int kAxiWaitState = 3;

static bool rand_bool() {
  return rand() & 1;
}

static bool rand_bool_ibus() {
  return rand_bool();
}

static bool rand_bool_dbus() {
  return rand_bool();
}

// ScalarCore Memory Interface.
struct Core_if : Memory_if {
  sc_in<bool>         io_ibus_valid;
  sc_out<bool>        io_ibus_ready;
  sc_in<sc_bv<32> >   io_ibus_addr;
  sc_out<sc_bv<KP_fetchDataBits> > io_ibus_rdata;

  sc_out<bool> io_ibus_fault_valid;
  sc_out<bool> io_ibus_fault_bits_write;
  sc_out<sc_bv<32>> io_ibus_fault_bits_addr;
  sc_out<sc_bv<32>> io_ibus_fault_bits_epc;

  sc_in<bool> io_dbus_valid;
  sc_out<bool> io_dbus_ready;
  sc_in<bool> io_dbus_write;
  sc_in<sc_bv<32> > io_dbus_addr;
  sc_in<sc_bv<32> > io_dbus_adrx;
  sc_in<sc_bv<KP_dbusSize> > io_dbus_size;
  sc_in<sc_bv<KP_lsuDataBits> > io_dbus_wdata;
  sc_in<sc_bv<KP_lsuDataBits / 8> > io_dbus_wmask;
  sc_out<sc_bv<KP_lsuDataBits> > io_dbus_rdata;

  sc_out<bool> io_ebus_fault_valid;
  sc_out<bool> io_ebus_fault_bits_write;
  sc_out<sc_bv<32>> io_ebus_fault_bits_addr;
  sc_out<sc_bv<32>> io_ebus_fault_bits_epc;

  Core_if(sc_module_name n, const char* bin, const std::string& profile = "all") : Memory_if(n, bin, /* limit= */ -1, profile) {
    for (int i = 0; i < KP_lsuDataBits / 32; ++i) {
      runused_.set_word(i, 0);
    }
  }

  void eval() {
    if (reset) {
      io_ibus_ready = false;
    } else if (clock.read()) {
      cycle_++;

      io_ebus_fault_valid = false;
      io_ibus_ready = rand_bool_ibus();
      io_dbus_ready = rand_bool_dbus();

      // Instruction bus read.
      if (io_ibus_valid && io_ibus_ready) {
        sc_bv<256> rdata;
        uint32_t addr = io_ibus_addr.read().get_word(0);
        uint32_t words[256 / 32];
        if (Read(addr, 256 / 8, reinterpret_cast<uint8_t*>(words))) {
          for (int i = 0; i < 256 / 32; ++i) {
            rdata.set_word(i, words[i]);
          }

          io_ibus_rdata = rdata;
        } else {
          io_ibus_fault_valid = true;
          io_ibus_fault_bits_write = false;
          io_ibus_fault_bits_addr = 0;
          io_ibus_fault_bits_epc = addr;
        }
      } else {
       io_ibus_fault_valid = false;
      }

      // Data bus read.
      if (io_dbus_valid && io_dbus_ready && !io_dbus_write) {
        sc_bv<KP_lsuDataBits> rdata;
        uint32_t addr = io_dbus_addr.read().get_word(0);
        constexpr int kLsuBytes = KP_lsuDataBits / 8;
        uint32_t words[kLsuBytes / 4] = {0};
        if (Read(addr, kLsuBytes, reinterpret_cast<uint8_t*>(words))) {
          for (int i = 0; i < KP_lsuDataBits / 32; ++i) {
            rdata.set_word(i, words[i]);
          }
          io_dbus_rdata = rdata;
        } else {
          io_ebus_fault_valid = true;
          io_ebus_fault_bits_write = false;
          io_ebus_fault_bits_addr = addr;
          io_ebus_fault_bits_epc = 0; // PC not easily available here
        }
      }

      // Data bus write.
      if (io_dbus_valid && io_dbus_ready && io_dbus_write) {
        uint32_t addr = io_dbus_addr.read().get_word(0);
        constexpr int kLsuBytes = KP_lsuDataBits / 8;
        uint8_t bytes[kLsuBytes] = {0};
        uint32_t mask = io_dbus_wmask.read().get_word(0);
        uint32_t wdata_words[kLsuBytes / 4] = {0};
        sc_bv<KP_lsuDataBits> wdata = io_dbus_wdata;
        for (int i = 0; i < KP_lsuDataBits / 32; ++i) {
          wdata_words[i] = wdata.get_word(i);
        }
        memcpy(bytes, wdata_words, kLsuBytes);
        for (int i = 0; i < kLsuBytes; ++i) {
          if ((mask >> i) & 1) {
            uint8_t val = bytes[i];
            if (!Write(addr + i, 1, &val)) {
              io_ebus_fault_valid = true;
              io_ebus_fault_bits_write = true;
              io_ebus_fault_bits_addr = addr + i;
              io_ebus_fault_bits_epc = 0;
            }
          }
        }
      }

      rtcm_t tcm_read;
      sc_bv<KP_lsuDataBits> rdata;
    }
  }

  int pending_exit_code() const { return pending_exit_code_; }
  void SetPendingExitCode(int code) { pending_exit_code_ = code; }

 private:
  uint32_t cycle_ = 0;
  int pending_exit_code_ = 0;

  struct rtcm_t {
    uint32_t cycle;
    uint32_t id : 7;
    sc_bv<KP_lsuDataBits> data;
  };

  fifo_t<rtcm_t> rtcm_[2];
  sc_bv<KP_lsuDataBits> runused_;
};

#endif  // TESTS_VERILATOR_SIM_CORALNPU_CORE_IF_H_
