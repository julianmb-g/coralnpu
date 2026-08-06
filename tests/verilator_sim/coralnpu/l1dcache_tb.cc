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

#include "tests/verilator_sim/sysc_tb.h"

#ifndef L1DCACHEBANK
#include "VL1DCache.h"
constexpr int kDBusBankAdj = 0;
using DUT_Class = VL1DCache;
#else
#include "VL1DCacheBank.h"
constexpr int kDBusBankAdj = 1;
using DUT_Class = VL1DCacheBank;
#endif

#include "tests/verilator_sim/coralnpu/coralnpu_cfg.h"

template <typename T> struct get_port_width;

template <typename T> struct get_port_width<T &> : get_port_width<T> {};

template <int W> struct get_port_width<sc_core::sc_in<sc_dt::sc_bv<W>>> {
  static const int value = W;
};

template <int W> struct get_port_width<sc_core::sc_out<sc_dt::sc_bv<W>>> {
  static const int value = W;
};

constexpr int kLineSize = kVector / 8;
constexpr int kLineBase = ~(kLineSize - 1);
constexpr int kLineOffset = kLineSize - 1;
constexpr int kVLenB = kVector / 8;
constexpr int kVLenW = kVLenB / sizeof(int32_t);

class L1DCacheTb : public SyscTb {
public:
  static const int DBUS_ADDR_WIDTH =
      get_port_width<decltype(DUT_Class::io_dbus_addr)>::value;
  static const int AXI_ADDR_WIDTH =
      get_port_width<decltype(DUT_Class::io_axi_read_addr_bits_addr)>::value;
  sc_out<bool> io_flush_valid;
  sc_in<bool> io_flush_ready;
  sc_out<bool> io_flush_all;
  sc_out<bool> io_flush_clean;

  sc_out<bool> io_dbus_valid;
  sc_in<bool> io_dbus_ready;
  sc_out<bool> io_dbus_write;
  sc_out<sc_bv<kDbusBits>> io_dbus_size;
  sc_out<sc_bv<DBUS_ADDR_WIDTH>> io_dbus_addr;
  sc_out<sc_bv<DBUS_ADDR_WIDTH>> io_dbus_adrx;
  sc_in<sc_bv<kVector>> io_dbus_rdata;
  sc_in<sc_bv<32>> io_dbus_pc;
  sc_out<sc_bv<kVector>> io_dbus_wdata;
  sc_out<sc_bv<kVector / 8>> io_dbus_wmask;

  sc_in<bool> io_axi_read_addr_valid;
  sc_out<bool> io_axi_read_addr_ready;
  sc_in<sc_bv<kL1DAxiId - kDBusBankAdj>> io_axi_read_addr_bits_id;
  sc_in<sc_bv<AXI_ADDR_WIDTH>> io_axi_read_addr_bits_addr;
  sc_in<sc_bv<4>> io_axi_read_addr_bits_region;
  sc_in<sc_bv<4>> io_axi_read_addr_bits_qos;
  sc_in<sc_bv<3>> io_axi_read_addr_bits_prot;
  sc_in<sc_bv<4>> io_axi_read_addr_bits_cache;
  sc_in<bool> io_axi_read_addr_bits_lock;
  sc_in<sc_bv<2>> io_axi_read_addr_bits_burst;
  sc_in<sc_bv<3>> io_axi_read_addr_bits_size;
  sc_in<sc_bv<8>> io_axi_read_addr_bits_len;

  sc_out<bool> io_axi_read_data_valid;
  sc_in<bool> io_axi_read_data_ready;
  sc_out<sc_bv<2>> io_axi_read_data_bits_resp;
  sc_out<sc_bv<kL1DAxiId - kDBusBankAdj>> io_axi_read_data_bits_id;
  sc_out<sc_bv<kL1DAxiBits>> io_axi_read_data_bits_data;
  sc_out<bool> io_axi_read_data_bits_last;

  sc_in<bool> io_axi_write_addr_valid;
  sc_out<bool> io_axi_write_addr_ready;
  sc_in<sc_bv<kL1DAxiId - kDBusBankAdj>> io_axi_write_addr_bits_id;
  sc_in<sc_bv<AXI_ADDR_WIDTH>> io_axi_write_addr_bits_addr;
  sc_in<sc_bv<4>> io_axi_write_addr_bits_region;
  sc_in<sc_bv<4>> io_axi_write_addr_bits_qos;
  sc_in<sc_bv<3>> io_axi_write_addr_bits_prot;
  sc_in<sc_bv<4>> io_axi_write_addr_bits_cache;
  sc_in<bool> io_axi_write_addr_bits_lock;
  sc_in<sc_bv<2>> io_axi_write_addr_bits_burst;
  sc_in<sc_bv<3>> io_axi_write_addr_bits_size;
  sc_in<sc_bv<8>> io_axi_write_addr_bits_len;

  sc_in<bool> io_axi_write_data_valid;
  sc_out<bool> io_axi_write_data_ready;
  sc_in<sc_bv<kL1DAxiStrb>> io_axi_write_data_bits_strb;
  sc_in<sc_bv<kL1DAxiBits>> io_axi_write_data_bits_data;
  sc_in<bool> io_axi_write_data_bits_last;

  sc_out<bool> io_axi_write_resp_valid;
  sc_in<bool> io_axi_write_resp_ready;
  sc_out<sc_bv<2>> io_axi_write_resp_bits_resp;
  sc_out<sc_bv<kL1DAxiId - kDBusBankAdj>> io_axi_write_resp_bits_id;

  sc_in<bool> io_volt_sel;

  using SyscTb::SyscTb;

  void Posedge() override {
    // dbus
#ifdef L1DCACHEBANK
    // Checks a bank cache line.
    if (dbus_resp_pipeline) {
      dbus_resp_pipeline = false;
      uint32_t addr = dbus_resp_addr;
      uint32_t size = dbus_resp_size;
      for (uint32_t i = 0; i < kVLenB && size; ++i) {
        uint8_t ref = dbus_resp_data[i];
        uint8_t dut = io_dbus_rdata.read().get_word(i / 4) >> (8 * i);
        if (ref != dut) {
          printf("DDD(%u) %08x : %02x %02x\n", i, (addr & ~(kVLenB - 1)) + i,
                 ref, dut);
        }
        Check(ref == dut, "dbus read data");
      }
    }
#else
    if (dbus_resp_pipeline) {
      dbus_resp_pipeline = false;
      uint32_t addr = dbus_resp_addr;
      uint32_t size = dbus_resp_size;
      for (uint32_t j = addr; j < addr + size; ++j) {
        uint32_t i = j & (kVLenB - 1);
        uint8_t ref = dbus_resp_data[i];
        uint8_t dut = io_dbus_rdata.read().get_word(i / 4) >> (8 * i);
        Check(ref == dut, "dbus read data");
      }
    }
#endif

    if (io_dbus_valid && io_dbus_ready && !io_dbus_write) {
      dbus_active = false;
      dbus_resp_pipeline = true;
      dbus_resp_addr = io_dbus_addr.read().get_word(0);
      dbus_resp_size = io_dbus_size.read().get_word(0);
#ifdef L1DCACHEBANK
      ReadBus(dbus_resp_addr & kLineBase, kVLenB, dbus_resp_data);
#else
      ReadBus(dbus_resp_addr, kVLenB, dbus_resp_data);
#endif
      history_t cmd({dbus_resp_addr});
      history.Write(cmd);
      if (history.Count() > 16) {
        history.Remove();
      }
    }

    if (io_dbus_valid && io_dbus_ready && io_dbus_write) {
      dbus_active = false;

      uint32_t addr = io_dbus_addr.read().get_word(0);
      int size = io_dbus_size.read().get_word(0);
      uint8_t wdata[kVLenB];
      uint32_t *p_wdata = reinterpret_cast<uint32_t *>(wdata);
      for (int i = 0; i < kVLenW; ++i) {
        p_wdata[i] = io_dbus_wdata.read().get_word(i);
      }
      const uint32_t linemask = kVLenB - 1;
#ifdef L1DCACHEBANK
      const uint32_t linebase = addr & ~linemask;
#endif
      for (int i = 0; i < size; ++i, ++addr) {
        const uint32_t lineoffset = addr & linemask;
        if (io_dbus_wmask.read().get_bit(lineoffset)) {
#ifdef L1DCACHEBANK
          WriteBus(linebase + lineoffset, wdata[lineoffset]);
#else
          WriteBus(addr, wdata[lineoffset]);
#endif
        }
      }
    }

    if (io_flush_valid && io_flush_ready) {
      flush_valid = false;
      flush_all = false;
      flush_clean = false;
    }

    if (++flush_count > 5000 && !dbus_active && !flush_valid) {
      // Flush controls must not change during handshake.
      flush_count = 0;
      flush_valid = true;
      flush_all = SyscTbRandBool();
      flush_clean = SyscTbRandBool();
    }

    io_flush_valid = flush_valid;
    io_flush_all = flush_all;
    io_flush_clean = flush_clean;

    history_t dbus;
    if (!io_dbus_valid || !dbus_active) { // latch transaction
      bool valid = SyscTbRandBool() && !flush_valid;
      bool write = RandInt(0, 3) == 0;
      bool newaddr = RandInt(0, 3) == 0 || !history.Rand(dbus);
      uint32_t addr =
          newaddr ? RandUint32() : (dbus.addr + RandInt(-kVLenB, kVLenB));
      // TODO(b/295973540): avoids a raxi() crash.
      addr = std::min(0xffffff00u, addr);
      if (kDBusBankAdj) {
        addr &= 0x7fffffff;
      }
      if (RandInt(0, 7) == 0) {
        addr &= 0x3fff;
      }
#ifdef L1DCACHEBANK
      int size = RandInt(1, kVLenB);
#else
      int size = RandInt(0, kVLenB);
#endif
      io_dbus_valid = valid;
      io_dbus_write = write;
      io_dbus_addr = addr;
      io_dbus_adrx = addr + kVLenB;
      io_dbus_size = size;
      if (valid) {
        dbus_active = true;
        CheckAddr(addr, size);
      }

      sc_bv<kVector> wdata = 0;
      sc_bv<kVector / 8> wmask = 0;

      if (write) {
        for (int i = 0; i < kVLenW; ++i) {
          wdata.set_word(i, RandUint32());
        }
        const uint32_t linemask = kVLenB - 1;
        const uint32_t lineoffset = addr & linemask;
        const bool all = SyscTbRandBool();
        for (int i = 0; i < size; ++i) {
          if (all || SyscTbRandBool()) {
            wmask.set_bit((i + lineoffset) & linemask, sc_dt::Log_1);
          }
        }
      }

      io_dbus_wdata.write(wdata);
      io_dbus_wmask.write(wmask);
    }

    timeout = io_dbus_ready ? 0 : timeout + io_dbus_valid;
    Check(timeout < 10000, "dbus timeout");

    // axi_read_addr
    io_axi_read_addr_ready = SyscTbRandBool();

    if (io_axi_read_addr_valid && io_axi_read_addr_ready) {
      uint32_t id = io_axi_read_addr_bits_id.read().get_word(0);
      uint32_t addr = io_axi_read_addr_bits_addr.read().get_word(0);
      response_t r({id, addr});
      resp.Write(r);
    }

    // axi_read_data
    io_axi_read_data_valid = false;
    io_axi_read_data_bits_id = 0;
    io_axi_read_data_bits_data = 0;

    if (io_axi_read_data_valid && io_axi_read_data_ready) {
      Check(resp.Remove(), "no response to erase");
    }

    response_t r;
    resp.Shuffle();
    if (resp.Next(r)) {
      io_axi_read_data_valid = SyscTbRandBool();
      io_axi_read_data_bits_id = r.id;
      uint32_t addr = r.addr;
      sc_bv<kL1DAxiBits> out;
      for (int i = 0; i < axiw; ++i) {
        uint32_t data;
        ReadAxi(addr, 4, reinterpret_cast<uint8_t *>(&data));
        out.set_word(i, data);
        addr += 4;
      }
      io_axi_read_data_bits_data = out;
    }

    // axi_write_addr
    bool writedataready = SyscTbRandBool();

    io_axi_write_addr_ready = writedataready;

    if (io_axi_write_addr_valid && io_axi_write_addr_ready) {
      axiwaddr_t p;
      p.id = io_axi_write_addr_bits_id.read().get_word(0);
      p.addr = io_axi_write_addr_bits_addr.read().get_word(0);
      waddr.Write(p);
    }

    // axi_write_data
    io_axi_write_data_ready = writedataready;

    if (io_axi_write_data_valid && io_axi_write_data_ready) {
      axiwdata_t p;
      uint32_t *ptr = reinterpret_cast<uint32_t *>(p.data);
      for (int i = 0; i < axiw; ++i, ++ptr) {
        ptr[0] = io_axi_write_data_bits_data.read().get_word(i);
      }
      for (int i = 0; i < axib; ++i) {
        p.mask[i] = io_axi_write_data_bits_strb.read().get_bit(i);
      }
      wdata.Write(p);
    }

    // axi_write_resp
    if (io_axi_write_resp_valid && io_axi_write_resp_ready) {
      wresp.Remove();
    }

    axiwaddr_t wr;
    io_axi_write_resp_valid = RandInt(0, 4) == 0 && wresp.Next(wr);
    io_axi_write_resp_bits_id = wr.id;

    // Process axi data write, and populate response.
    axiwaddr_t wa;
    axiwdata_t wd;
    if (waddr.Next(wa) && wdata.Next(wd)) {
      waddr.Remove();
      wdata.Remove();
      wresp.Write(wa);

      uint32_t addr = wa.addr;
      for (int i = 0; i < axib; ++i, ++addr) {
        if (wd.mask[i]) {
          WriteAxi(addr, wd.data[i]);
        }
      }
    }
  }

private:
  struct history_t {
    uint32_t addr;
  };

  struct response_t {
    uint32_t id;
    uint32_t addr;
  };

  struct axiwaddr_t {
    uint32_t id;
    uint32_t addr;
  };

  struct axiwdata_t {
    uint8_t data[kL1DAxiBits / 8];
    bool mask[kL1DAxiBits / 8];
  };

  const int axib = kL1DAxiBits / 8;
  const int axiw = kL1DAxiBits / 32;

  int timeout = 0;
  int flush_count = 0;
  bool flush_valid = false;
  bool flush_all = false;
  bool flush_clean = false;

  bool dbus_active = false;
  bool dbus_resp_pipeline = false;
  uint32_t dbus_resp_addr = 0;
  uint32_t dbus_resp_size = 0;
  uint8_t dbus_resp_data[kVector / 8];
  Fifo<response_t> resp;
  Fifo<history_t> history;
  Fifo<axiwaddr_t> waddr;
  Fifo<axiwdata_t> wdata;
  Fifo<axiwaddr_t> wresp;

  std::map<uint32_t, uint8_t[kLineSize]> mem_bus;
  std::map<uint32_t, uint8_t[kLineSize]> mem_axi;

  void _CheckAddr(uint32_t addr, uint8_t size) {
    const uint32_t paddr = addr & kLineBase;
    if (mem_bus.find(paddr) == mem_bus.end()) {
      uint8_t data[kLineSize];
      uint32_t *p_data = reinterpret_cast<uint32_t *>(data);
      for (int i = 0; i < kLineSize / 4; ++i) {
        p_data[i] = rand(); // NOLINT(runtime/threadsafe_fn)
      }
      memcpy(mem_bus[paddr], data, kLineSize);
      memcpy(mem_axi[paddr], data, kLineSize);
    }
  }

  void CheckAddr(uint32_t addr, uint8_t size) {
    _CheckAddr(addr, size);
    _CheckAddr(addr + kLineSize, size);
  }

  template <int outsz>
  void _Read(uint32_t addr, uint8_t size, uint8_t *data,
             std::map<uint32_t, uint8_t[kLineSize]> &m) {
    const uint32_t laddr = addr & kLineBase;
    const uint32_t loffset = addr & kLineOffset;
    const uint32_t doffset = addr & (outsz - 1);
#ifndef L1DCACHEBANK
    uint32_t start = addr;
    uint32_t end = std::min(addr + size, laddr + kLineSize);
    int size0 = end - start;
    int size1 = size - size0;
#endif

    memset(data, 0xCC, outsz);
#ifdef L1DCACHEBANK
    assert(doffset == 0);
    memcpy(data + doffset, m[laddr] + loffset, outsz);
#else
    memcpy(data + doffset, m[laddr] + loffset, size0);
    if (!size1)
      return;
    memcpy(data, m[laddr + kLineSize], size1);
#endif
  }

  void _Write(uint32_t addr, uint8_t data,
              std::map<uint32_t, uint8_t[kLineSize]> &m) {
    const uint32_t laddr = addr & kLineBase;
    const uint32_t loffset = addr & kLineOffset;

    m[laddr][loffset] = data;
  }

  void ReadBus(uint32_t addr, uint8_t size, uint8_t *data) {
    _Read<kVector / 8>(addr, size, data, mem_bus);
  }

  void ReadAxi(uint32_t addr, uint8_t size, uint8_t *data) {
    _Read<4>(addr, size, data, mem_axi);
  }

  void WriteBus(uint32_t addr, uint8_t data) { _Write(addr, data, mem_bus); }

  void WriteAxi(uint32_t addr, uint8_t data) { _Write(addr, data, mem_axi); }
};

static void L1DCacheTest(absl::string_view name, int loops, bool trace) {
  sc_signal<bool> clock;
  sc_signal<bool> reset;

  sc_signal<bool> io_flush_valid;
  sc_signal<bool> io_flush_ready;
  sc_signal<bool> io_flush_all;
  sc_signal<bool> io_flush_clean;

  sc_signal<bool> io_dbus_valid;
  sc_signal<bool> io_dbus_ready;
  sc_signal<bool> io_dbus_write;
  sc_signal<sc_bv<kDbusBits>> io_dbus_size;
  sc_signal<sc_bv<L1DCache_tb::DBUS_ADDR_WIDTH>> io_dbus_addr;
  sc_signal<sc_bv<L1DCache_tb::DBUS_ADDR_WIDTH>> io_dbus_adrx;
  sc_signal<sc_bv<kVector>> io_dbus_rdata;
  sc_signal<sc_bv<32>> io_dbus_pc;
  sc_signal<sc_bv<kVector>> io_dbus_wdata;
  sc_signal<sc_bv<kVector / 8>> io_dbus_wmask;

  sc_signal<bool> io_axi_read_addr_valid;
  sc_signal<bool> io_axi_read_addr_ready;
  sc_signal<sc_bv<kL1DAxiId - kDBusBankAdj>> io_axi_read_addr_bits_id;
  sc_signal<sc_bv<L1DCache_tb::AXI_ADDR_WIDTH>> io_axi_read_addr_bits_addr;
  sc_signal<sc_bv<4>> io_axi_read_addr_bits_region;
  sc_signal<sc_bv<4>> io_axi_read_addr_bits_qos;
  sc_signal<sc_bv<3>> io_axi_read_addr_bits_prot;
  sc_signal<sc_bv<4>> io_axi_read_addr_bits_cache;
  sc_signal<bool> io_axi_read_addr_bits_lock;
  sc_signal<sc_bv<2>> io_axi_read_addr_bits_burst;
  sc_signal<sc_bv<3>> io_axi_read_addr_bits_size;
  sc_signal<sc_bv<8>> io_axi_read_addr_bits_len;

  sc_signal<bool> io_axi_read_data_valid;
  sc_signal<bool> io_axi_read_data_ready;
  sc_signal<sc_bv<2>> io_axi_read_data_bits_resp;
  sc_signal<sc_bv<kL1DAxiId - kDBusBankAdj>> io_axi_read_data_bits_id;
  sc_signal<sc_bv<kL1DAxiBits>> io_axi_read_data_bits_data;
  sc_signal<bool> io_axi_read_data_bits_last;

  sc_signal<bool> io_axi_write_addr_valid;
  sc_signal<bool> io_axi_write_addr_ready;
  sc_signal<sc_bv<kL1DAxiId - kDBusBankAdj>> io_axi_write_addr_bits_id;
  sc_signal<sc_bv<L1DCache_tb::AXI_ADDR_WIDTH>> io_axi_write_addr_bits_addr;
  sc_signal<sc_bv<4>> io_axi_write_addr_bits_region;
  sc_signal<sc_bv<4>> io_axi_write_addr_bits_qos;
  sc_signal<sc_bv<3>> io_axi_write_addr_bits_prot;
  sc_signal<sc_bv<4>> io_axi_write_addr_bits_cache;
  sc_signal<bool> io_axi_write_addr_bits_lock;
  sc_signal<sc_bv<2>> io_axi_write_addr_bits_burst;
  sc_signal<sc_bv<3>> io_axi_write_addr_bits_size;
  sc_signal<sc_bv<8>> io_axi_write_addr_bits_len;

  sc_signal<bool> io_axi_write_data_valid;
  sc_signal<bool> io_axi_write_data_ready;
  sc_signal<sc_bv<kL1DAxiStrb>> io_axi_write_data_bits_strb;
  sc_signal<sc_bv<kL1DAxiBits>> io_axi_write_data_bits_data;
  sc_signal<bool> io_axi_write_data_bits_last;

  sc_signal<bool> io_axi_write_resp_valid;
  sc_signal<bool> io_axi_write_resp_ready;
  sc_signal<sc_bv<2>> io_axi_write_resp_bits_resp;
  sc_signal<sc_bv<kL1DAxiId - kDBusBankAdj>> io_axi_write_resp_bits_id;

  sc_signal<bool> io_volt_sel;

  L1DCacheTb tb("L1DCacheTb", loops, true /*random*/);
#ifdef L1DCACHEBANK
  VL1DCacheBank l1dcache(std::string(name).c_str());
#else
  VL1DCache l1dcache(std::string(name).c_str());
#endif

  if (trace) {
    tb.Trace(&l1dcache);
  }

  l1dcache.clock(tb.clock);
  l1dcache.reset(tb.reset);

  BIND2(tb, l1dcache, io_flush_valid);
  BIND2(tb, l1dcache, io_flush_ready);
  BIND2(tb, l1dcache, io_flush_all);
  BIND2(tb, l1dcache, io_flush_clean);

  BIND2(tb, l1dcache, io_dbus_valid);
  BIND2(tb, l1dcache, io_dbus_ready);
  BIND2(tb, l1dcache, io_dbus_write);
  BIND2(tb, l1dcache, io_dbus_size);
  BIND2(tb, l1dcache, io_dbus_addr);
  BIND2(tb, l1dcache, io_dbus_adrx);
  BIND2(tb, l1dcache, io_dbus_rdata);
  BIND2(tb, l1dcache, io_dbus_pc);
  BIND2(tb, l1dcache, io_dbus_wdata);
  BIND2(tb, l1dcache, io_dbus_wmask);

  BIND2(tb, l1dcache, io_axi_read_addr_valid);
  BIND2(tb, l1dcache, io_axi_read_addr_ready);
  BIND2(tb, l1dcache, io_axi_read_addr_bits_id);
  BIND2(tb, l1dcache, io_axi_read_addr_bits_addr);
  BIND2(tb, l1dcache, io_axi_read_addr_bits_region);
  BIND2(tb, l1dcache, io_axi_read_addr_bits_qos);
  BIND2(tb, l1dcache, io_axi_read_addr_bits_prot);
  BIND2(tb, l1dcache, io_axi_read_addr_bits_cache);
  BIND2(tb, l1dcache, io_axi_read_addr_bits_lock);
  BIND2(tb, l1dcache, io_axi_read_addr_bits_burst);
  BIND2(tb, l1dcache, io_axi_read_addr_bits_size);
  BIND2(tb, l1dcache, io_axi_read_addr_bits_len);

  BIND2(tb, l1dcache, io_axi_read_data_valid);
  BIND2(tb, l1dcache, io_axi_read_data_ready);
  BIND2(tb, l1dcache, io_axi_read_data_bits_resp);
  BIND2(tb, l1dcache, io_axi_read_data_bits_id);
  BIND2(tb, l1dcache, io_axi_read_data_bits_data);
  BIND2(tb, l1dcache, io_axi_read_data_bits_last);

  BIND2(tb, l1dcache, io_axi_write_addr_valid);
  BIND2(tb, l1dcache, io_axi_write_addr_ready);
  BIND2(tb, l1dcache, io_axi_write_addr_bits_id);
  BIND2(tb, l1dcache, io_axi_write_addr_bits_addr);
  BIND2(tb, l1dcache, io_axi_write_addr_bits_region);
  BIND2(tb, l1dcache, io_axi_write_addr_bits_qos);
  BIND2(tb, l1dcache, io_axi_write_addr_bits_prot);
  BIND2(tb, l1dcache, io_axi_write_addr_bits_cache);
  BIND2(tb, l1dcache, io_axi_write_addr_bits_lock);
  BIND2(tb, l1dcache, io_axi_write_addr_bits_burst);
  BIND2(tb, l1dcache, io_axi_write_addr_bits_size);
  BIND2(tb, l1dcache, io_axi_write_addr_bits_len);

  BIND2(tb, l1dcache, io_axi_write_data_valid);
  BIND2(tb, l1dcache, io_axi_write_data_ready);
  BIND2(tb, l1dcache, io_axi_write_data_bits_strb);
  BIND2(tb, l1dcache, io_axi_write_data_bits_data);
  BIND2(tb, l1dcache, io_axi_write_data_bits_last);

  BIND2(tb, l1dcache, io_axi_write_resp_valid);
  BIND2(tb, l1dcache, io_axi_write_resp_ready);
  BIND2(tb, l1dcache, io_axi_write_resp_bits_resp);
  BIND2(tb, l1dcache, io_axi_write_resp_bits_id);

  BIND2(tb, l1dcache, io_volt_sel);

  tb.Start();
}

int sc_main(int argc, char *argv[]) {
  L1DCacheTest(SyscTb::GetName(argv[0]), 1000000, false);
  return 0;
}
