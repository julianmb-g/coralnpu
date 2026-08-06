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

#include "tests/verilator_sim/coralnpu/coralnpu_cfg.h"
#include "tests/verilator_sim/coralnpu/memory_if.h"
#include "tests/verilator_sim/fifo.h"
#include <cstdlib>
#include <cstring>

static inline bool RandBool() { return (std::rand() % 2) == 0; }

static inline bool RandBoolIbus() { return RandBool(); }

static inline bool RandBoolDbus() { return RandBool(); }

// ScalarCore Memory Interface.
class CoreIf : public MemoryIf {
public:
  sc_in<bool> io_ibus_valid;
  sc_out<bool> io_ibus_ready;
  sc_in<sc_bv<KP_programCounterBits>> io_ibus_addr;
  sc_out<sc_bv<KP_fetchDataBits>> io_ibus_rdata;

  sc_out<bool> io_ibus_fault_valid;
  sc_out<bool> io_ibus_fault_bits_write;
  sc_out<sc_bv<KP_programCounterBits>> io_ibus_fault_bits_addr;
  sc_out<sc_bv<KP_programCounterBits>> io_ibus_fault_bits_epc;

  sc_in<bool> io_dbus_valid;
  sc_out<bool> io_dbus_ready;
  sc_in<bool> io_dbus_write;
  sc_in<sc_bv<KP_lsuAddrBits>> io_dbus_addr;
  sc_in<sc_bv<KP_lsuAddrBits>> io_dbus_adrx;
  sc_in<sc_bv<KP_dbusSize>> io_dbus_size;
  sc_in<sc_bv<KP_lsuDataBits>> io_dbus_wdata;
  sc_in<sc_bv<KP_lsuDataBits / 8>> io_dbus_wmask;
  sc_out<sc_bv<KP_lsuDataBits>> io_dbus_rdata;

  sc_out<bool> io_ebus_fault_valid;
  sc_out<bool> io_ebus_fault_bits_write;
  sc_out<sc_bv<32>> io_ebus_fault_bits_addr;
  sc_out<sc_bv<32>> io_ebus_fault_bits_epc;

  CoreIf(sc_module_name n, const char *bin, absl::string_view profile = "all")
      : MemoryIf(n, bin, /* limit= */ -1, profile) {
    for (int i = 0; i < KP_lsuDataBits / 32; ++i) {
      runused_.set_word(i, 0);
    }
  }

  using MemoryIf::Read;
  using MemoryIf::Write;

  bool Read(uint32_t addr, int bytes, uint8_t *data) override {
    if (!MemoryIf::Read(addr, bytes, data)) {
      pending_exit_code_ = 65;
      last_fault_addr_ = addr;
      last_fault_size_ = bytes;
      return false;
    }
    return true;
  }

  bool Write(uint32_t addr, int bytes, const uint8_t *data) override {
    if (!MemoryIf::Write(addr, bytes, data)) {
      pending_exit_code_ = 65;
      last_fault_addr_ = addr;
      last_fault_size_ = bytes;
      return false;
    }
    return true;
  }

  void Eval() override {
    if (reset.read()) {
      io_ibus_ready = false;
    } else if (clock.read()) {
      cycle_++;

      io_ebus_fault_valid = false;
      io_ibus_ready = RandBoolIbus();
      io_dbus_ready = RandBoolDbus();

      // Instruction bus read.
      if (io_ibus_valid.read() && io_ibus_ready.read()) {
        sc_bv<256> rdata;
        uint32_t addr = io_ibus_addr.read().get_word(0);
        uint32_t words[256 / 32];
        if (Read(addr, 256 / 8, reinterpret_cast<uint8_t *>(words))) {
          for (int i = 0; i < 256 / 32; ++i) {
            rdata.set_word(i, words[i]);
          }
          io_ibus_rdata = rdata;
        } else {
          io_ibus_fault_valid = true;
          io_ibus_fault_bits_write = false;
          io_ibus_fault_bits_addr = 0;
          io_ibus_fault_bits_epc = addr;
          pending_exit_code_ = 65;
          sc_stop();
        }
      } else {
        io_ibus_fault_valid = false;
      }

      // Data bus read.
      if (io_dbus_valid.read() && io_dbus_ready.read() &&
          !io_dbus_write.read()) {
        sc_bv<KP_lsuDataBits> rdata;
        uint32_t addr = io_dbus_addr.read().get_word(0);
        uint32_t dbus_size = io_dbus_size.read().get_word(0);
        constexpr int kLsuBytes = KP_lsuDataBits / 8;
        uint32_t words[kLsuBytes / 4] = {0};
        uint8_t *data8 = reinterpret_cast<uint8_t *>(words);
        if (Read(addr, dbus_size, data8)) {
          for (int i = 0; i < KP_lsuDataBits / 32; ++i) {
            rdata.set_word(i, words[i]);
          }
          io_dbus_rdata = rdata;
        } else {
          io_ebus_fault_valid = true;
          io_ebus_fault_bits_write = false;
          io_ebus_fault_bits_addr = addr;
          io_ebus_fault_bits_epc = 0;
          pending_exit_code_ = 65;
          sc_stop();
        }
      }

      // Data bus write.
      if (io_dbus_valid.read() && io_dbus_ready.read() &&
          io_dbus_write.read()) {
        uint32_t addr = io_dbus_addr.read().get_word(0);
        constexpr int kLsuBytes = KP_lsuDataBits / 8;
        uint8_t bytes[kLsuBytes] = {0};
        uint32_t mask = io_dbus_wmask.read().get_word(0);
        uint32_t wdata_words[kLsuBytes / 4] = {0};
        sc_bv<KP_lsuDataBits> wdata = io_dbus_wdata;
        for (int i = 0; i < KP_lsuDataBits / 32; ++i) {
          wdata_words[i] = wdata.get_word(i);
        }
        std::memcpy(bytes, wdata_words, kLsuBytes);
        for (int i = 0; i < kLsuBytes; ++i) {
          if ((mask >> i) & 1) {
            uint8_t val = bytes[i];
            if (!Write(addr + i, 1, &val)) {
              io_ebus_fault_valid = true;
              io_ebus_fault_bits_write = true;
              io_ebus_fault_bits_addr = addr + i;
              io_ebus_fault_bits_epc = 0;
              pending_exit_code_ = 65;
              sc_stop();
            }
          }
        }
      }
    }
  }

  uint32_t LastFaultAddr() const { return last_fault_addr_; }
  int LastFaultSize() const { return last_fault_size_; }

protected:
  uint32_t cycle_ = 0;
  uint32_t last_fault_addr_ = 0;
  int last_fault_size_ = 0;

  struct Rtcm {
    uint32_t cycle;
    uint32_t id : 7;
    sc_bv<KP_lsuDataBits> data;
  };

  Fifo<Rtcm> rtcm_[2];
  sc_bv<KP_lsuDataBits> runused_;
};

// Zero-Latency Bare Core Interface.
class BareCoreInterface : public CoreIf {
public:
  BareCoreInterface(sc_module_name n, const char *bin,
                    absl::string_view profile = "all")
      : CoreIf(n, bin, profile) {}

  void Eval() override {
    if (reset.read()) {
      io_ibus_ready = false;
    } else if (clock.read()) {
      cycle_++;

      io_ebus_fault_valid = false;
      io_ibus_ready = true; // Unconditional zero-latency ready response
      io_dbus_ready = true; // Unconditional zero-latency ready response

      // Instruction bus read.
      if (io_ibus_valid.read() && io_ibus_ready.read()) {
        sc_bv<256> rdata;
        uint32_t addr = io_ibus_addr.read().get_word(0);
        uint32_t words[256 / 32];
        if (Read(addr, 256 / 8, reinterpret_cast<uint8_t *>(words))) {
          for (int i = 0; i < 256 / 32; ++i) {
            rdata.set_word(i, words[i]);
          }
          io_ibus_rdata = rdata;
        } else {
          io_ibus_fault_valid = true;
          io_ibus_fault_bits_write = false;
          io_ibus_fault_bits_addr = 0;
          io_ibus_fault_bits_epc = addr;
          pending_exit_code_ = 65;
          sc_stop();
        }
      } else {
        io_ibus_fault_valid = false;
      }

      // Data bus read.
      if (io_dbus_valid.read() && io_dbus_ready.read() &&
          !io_dbus_write.read()) {
        sc_bv<KP_lsuDataBits> rdata;
        uint32_t addr = io_dbus_addr.read().get_word(0);
        uint32_t dbus_size = io_dbus_size.read().get_word(0);
        constexpr int kLsuBytes = KP_lsuDataBits / 8;
        uint32_t words[kLsuBytes / 4] = {0};
        uint8_t *data8 = reinterpret_cast<uint8_t *>(words);
        if (Read(addr, dbus_size, data8)) {
          for (int i = 0; i < KP_lsuDataBits / 32; ++i) {
            rdata.set_word(i, words[i]);
          }
          io_dbus_rdata = rdata;
        } else {
          io_ebus_fault_valid = true;
          io_ebus_fault_bits_write = false;
          io_ebus_fault_bits_addr = addr;
          io_ebus_fault_bits_epc = 0;
          pending_exit_code_ = 65;
          sc_stop();
        }
      }

      // Data bus write.
      if (io_dbus_valid.read() && io_dbus_ready.read() &&
          io_dbus_write.read()) {
        uint32_t addr = io_dbus_addr.read().get_word(0);
        constexpr int kLsuBytes = KP_lsuDataBits / 8;
        uint8_t bytes[kLsuBytes] = {0};
        uint32_t mask = io_dbus_wmask.read().get_word(0);
        uint32_t wdata_words[kLsuBytes / 4] = {0};
        sc_bv<KP_lsuDataBits> wdata = io_dbus_wdata;
        for (int i = 0; i < KP_lsuDataBits / 32; ++i) {
          wdata_words[i] = wdata.get_word(i);
        }
        std::memcpy(bytes, wdata_words, kLsuBytes);
        for (int i = 0; i < kLsuBytes; ++i) {
          if ((mask >> i) & 1) {
            uint8_t val = bytes[i];
            if (!Write(addr + i, 1, &val)) {
              io_ebus_fault_valid = true;
              io_ebus_fault_bits_write = true;
              io_ebus_fault_bits_addr = addr + i;
              io_ebus_fault_bits_epc = 0;
              pending_exit_code_ = 65;
              sc_stop();
            }
          }
        }
      }
    }
  }
};

#endif // TESTS_VERILATOR_SIM_CORALNPU_CORE_IF_H_
