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

#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "gtest/gtest.h"

#include "VCoreBarebones.h"
#include "hdl/chisel/src/coralnpu/VCoreBarebones_parameters.h"
#include "tests/verilator_sim/coralnpu/memory_if.h"
#include "tests/verilator_sim/sysc_tb.h"
#include "tests/verilator_sim/elf_loader_adapter.h"
#include "tests/verilator_sim/util.h"

namespace {

// Local definitions to avoid conflict with system elf.h and elfio
struct LocalElf32_Ehdr {
  unsigned char e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry;
  uint32_t e_phoff;
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
};

struct LocalElf32_Phdr {
  uint32_t p_type;
  uint32_t p_offset;
  uint32_t p_vaddr;
  uint32_t p_paddr;
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint32_t p_flags;
  uint32_t p_align;
};

constexpr uint8_t  ELFMAG0 = 0x7f;
constexpr uint8_t  ELFMAG1 = 'E';
constexpr uint8_t  ELFMAG2 = 'L';
constexpr uint8_t  ELFMAG3 = 'F';
constexpr uint8_t  ELFCLASS32 = 1;
constexpr uint8_t  ELFDATA2LSB = 1;
constexpr uint8_t  EV_CURRENT = 1;
constexpr uint16_t ET_EXEC = 2;
constexpr uint16_t EM_RISCV = 243;
constexpr uint32_t PT_LOAD = 1;

class TestMemoryIf : public MemoryIf {
 public:
  TestMemoryIf(sc_module_name n, absl::string_view profile)
      : MemoryIf(n, /* bin= */ nullptr, /* limit= */ 0, profile) {}
  void Eval() override {}
};

TEST(MemoryIfTest, GetOverflowDeltaDefault) {
  TestMemoryIf mem("mem", "default");
  // ITCM: [0x00000000 - 0x00002000], DTCM: [0x00010000 - 0x00018000]
  
  // Valid ITCM
  EXPECT_EQ(mem.GetOverflowDelta(0x0000, 4), 0);
  EXPECT_EQ(mem.GetOverflowDelta(0x1FFC, 4), 0);
  
  // Overflow ITCM
  EXPECT_EQ(mem.GetOverflowDelta(0x1FFC, 8), 4);
  
  // Valid DTCM
  EXPECT_EQ(mem.GetOverflowDelta(0x10000, 4), 0);
  EXPECT_EQ(mem.GetOverflowDelta(0x17FFC, 4), 0);
  
  // Overflow DTCM
  EXPECT_EQ(mem.GetOverflowDelta(0x17FFC, 8), 4);
  
  // Gap Access
  EXPECT_EQ(mem.GetOverflowDelta(0x2000, 4), 4);
  EXPECT_EQ(mem.GetOverflowDelta(0x8000, 4), 4);
  EXPECT_EQ(mem.GetOverflowDelta(0xFFFC, 8), 4); // 4 bytes in gap
}

TEST(MemoryIfTest, GetOverflowDeltaHighMem) {
  TestMemoryIf mem("mem", "highmem");
  // ITCM: [0x00100000 - 0x00200000], DTCM: [0x00100000 - 0x00100000]
  
  // Valid
  EXPECT_EQ(mem.GetOverflowDelta(0x100000, 4), 0);
  EXPECT_EQ(mem.GetOverflowDelta(0x1FFFFC, 4), 0);
  
  // Overflow
  EXPECT_EQ(mem.GetOverflowDelta(0x1FFFFC, 8), 4);
  EXPECT_EQ(mem.GetOverflowDelta(0x0FFFC, 8), 8); // Entirely before
  EXPECT_EQ(mem.GetOverflowDelta(0x200000, 4), 4); // Entirely after
}

class MockMemory {
 public:
  bool Write(uint64_t addr, size_t count, const uint8_t* src) {
    if (addr + count > data_.size()) {
      data_.resize(addr + count, 0);
    }
    std::memcpy(data_.data() + addr, src, count);
    return true;
  }
  const std::vector<uint8_t>& data() const { return data_; }
 private:
  std::vector<uint8_t> data_;
};

TEST(BarebonesMemoryTest, LoadElf) {
  // Create a temporary dummy ELF file for testing.
  const std::string dummy_elf = "/tmp/test.elf";
  std::vector<uint8_t> elf_data(1024, 0);
  LocalElf32_Ehdr ehdr = {};
  ehdr.e_ident[0] = ELFMAG0;
  ehdr.e_ident[1] = ELFMAG1;
  ehdr.e_ident[2] = ELFMAG2;
  ehdr.e_ident[3] = ELFMAG3;
  ehdr.e_ident[4] = ELFCLASS32;
  ehdr.e_ident[5] = ELFDATA2LSB;
  ehdr.e_ident[6] = EV_CURRENT;
  ehdr.e_type = ET_EXEC;
  ehdr.e_machine = EM_RISCV;
  ehdr.e_version = EV_CURRENT;
  ehdr.e_entry = 0x00010000;
  ehdr.e_phoff = sizeof(LocalElf32_Ehdr);
  ehdr.e_phnum = 1;
  ehdr.e_phentsize = sizeof(LocalElf32_Phdr);
  std::memcpy(elf_data.data(), &ehdr, sizeof(ehdr));

  LocalElf32_Phdr phdr = {};
  phdr.p_type = PT_LOAD;
  phdr.p_offset = 512;
  phdr.p_vaddr = 0x00010000;
  phdr.p_paddr = 0x00010000;
  phdr.p_filesz = 4;
  phdr.p_memsz = 4;
  std::memcpy(elf_data.data() + sizeof(ehdr), &phdr, sizeof(phdr));

  uint32_t payload = 0xdeadbeef;
  std::memcpy(elf_data.data() + 512, &payload, sizeof(payload));

  std::ofstream ofs(dummy_elf, std::ios::binary);
  ofs.write(reinterpret_cast<const char*>(elf_data.data()), elf_data.size());
  ofs.close();

  MockMemory mem;
  TestMemoryIf mem_if("mem", "default");
  mpact::sim::util::MemoryIfDebugAdapter mem_adapter(&mem_if);
  mpact::sim::util::ElfProgramLoader loader(&mem_adapter);

  // In testing we need to bridge the MockMemory through the TestMemoryIf,
  // or simply bypass MockMemory entirely since TestMemoryIf works!
  auto entry_point_or = loader.LoadProgram(dummy_elf);
  ASSERT_TRUE(entry_point_or.ok());
  EXPECT_EQ(entry_point_or.value(), 0x00010000);
  
  uint32_t loaded_payload = 0;
  mem_if.Read(0x00010000, 4, reinterpret_cast<uint8_t*>(&loaded_payload));
  EXPECT_EQ(loaded_payload, 0xdeadbeef);
}

TEST(BarebonesMemoryTest, LoadElfInvalid) {
  const std::string dummy_elf = "/tmp/invalid.elf";
  std::vector<uint8_t> elf_data(1024, 0); // Invalid data
  std::ofstream ofs(dummy_elf, std::ios::binary);
  ofs.write(reinterpret_cast<const char*>(elf_data.data()), elf_data.size());
  ofs.close();

  MockMemory mem;
  TestMemoryIf mem_if("mem", "default");
  mpact::sim::util::MemoryIfDebugAdapter mem_adapter(&mem_if);
  mpact::sim::util::ElfProgramLoader loader(&mem_adapter);
  auto entry_point_or = loader.LoadProgram(dummy_elf);
  EXPECT_FALSE(entry_point_or.ok());
}

TEST(BarebonesMemoryTest, ElfLoaderExitOnViolation) {
  const std::string dummy_elf = "/tmp/highmem_test.elf";
  std::vector<uint8_t> elf_data(1024, 0);
  LocalElf32_Ehdr ehdr = {};
  std::memset(&ehdr, 0, sizeof(ehdr));
  ehdr.e_ident[0] = ELFMAG0;
  ehdr.e_ident[1] = ELFMAG1;
  ehdr.e_ident[2] = ELFMAG2;
  ehdr.e_ident[3] = ELFMAG3;
  ehdr.e_ident[4] = ELFCLASS32;
  ehdr.e_ident[5] = ELFDATA2LSB;
  ehdr.e_ident[6] = EV_CURRENT;
  ehdr.e_type = ET_EXEC;
  ehdr.e_machine = EM_RISCV;
  ehdr.e_entry = 0x00100000; // Highmem address
  ehdr.e_phoff = sizeof(LocalElf32_Ehdr);
  ehdr.e_phnum = 1;
  ehdr.e_phentsize = sizeof(LocalElf32_Phdr);
  std::memcpy(elf_data.data(), &ehdr, sizeof(ehdr));

  LocalElf32_Phdr phdr = {};
  phdr.p_type = PT_LOAD;
  phdr.p_offset = 512;
  phdr.p_vaddr = 0x00100000;
  phdr.p_paddr = 0x00100000;
  phdr.p_filesz = 4;
  phdr.p_memsz = 4;
  std::memcpy(elf_data.data() + sizeof(ehdr), &phdr, sizeof(phdr));

  std::ofstream ofs(dummy_elf, std::ios::binary);
  ofs.write(reinterpret_cast<const char*>(elf_data.data()), elf_data.size());
  ofs.close();

  TestMemoryIf mem("mem", "default");
  mpact::sim::util::MemoryIfDebugAdapter mem_adapter(&mem);
  mpact::sim::util::ElfProgramLoader loader(&mem_adapter);

  auto status_or = loader.LoadProgram(dummy_elf);
  EXPECT_FALSE(status_or.ok()) << status_or.status();
}

struct DummyModelSignals {
  sc_clock clock{"clock", 10, SC_NS};
  sc_signal<bool> reset{"reset"};
  sc_signal<bool> io_halted{"io_halted"};
  sc_signal<bool> io_fault{"io_fault"};
  sc_signal<bool> io_wfi{"io_wfi"};
  sc_signal<bool> io_irq{"io_irq"};
  sc_signal<bool> io_timer_irq{"io_timer_irq"};
  sc_signal<bool> io_software_irq{"io_software_irq"};
  sc_signal<bool> io_debug_req{"io_debug_req"};
  sc_signal<bool> io_iflush_ready{"io_iflush_ready"};
  sc_signal<bool> io_dflush_ready{"io_dflush_ready"};
  sc_signal<bool> io_ibus_valid{"io_ibus_valid"};
  sc_signal<bool> io_ibus_ready{"io_ibus_ready"};
  sc_signal<sc_bv<KP_programCounterBits>> io_ibus_addr{"io_ibus_addr"};
  sc_signal<sc_bv<KP_fetchDataBits>> io_ibus_rdata{"io_ibus_rdata"};
  sc_signal<bool> io_dbus_valid{"io_dbus_valid"};
  sc_signal<bool> io_dbus_ready{"io_dbus_ready"};
  sc_signal<bool> io_dbus_write{"io_dbus_write"};
  sc_signal<sc_bv<32>> io_dbus_addr{"io_dbus_addr"};
  sc_signal<sc_bv<KP_dbusSize>> io_dbus_size{"io_dbus_size"};
  sc_signal<sc_bv<KP_lsuDataBits>> io_dbus_wdata{"io_dbus_wdata"};
  sc_signal<sc_bv<KP_lsuDataBits / 8>> io_dbus_wmask{"io_dbus_wmask"};
  sc_signal<sc_bv<KP_lsuDataBits>> io_dbus_rdata{"io_dbus_rdata"};
  sc_signal<bool> io_ibus_fault_valid{"io_ibus_fault_valid"};
  sc_signal<bool> io_ibus_fault_bits_write{"io_ibus_fault_bits_write"};
  sc_signal<sc_bv<KP_programCounterBits>> io_ibus_fault_bits_addr{"io_ibus_fault_bits_addr"};
  sc_signal<sc_bv<KP_programCounterBits>> io_ibus_fault_bits_epc{"io_ibus_fault_bits_epc"};
  sc_signal<sc_bv<32>> io_dbus_adrx{"io_dbus_adrx"};
  sc_signal<sc_bv<KP_programCounterBits>> io_dbus_pc{"io_dbus_pc"};
#if KP_exposeDebugPorts
  sc_signal<sc_bv<4>> io_debug_en{"io_debug_en"};
  sc_signal<sc_bv<32>> io_debug_cycles{"io_debug_cycles"};
#endif
  sc_signal<bool> io_iflush_valid{"io_iflush_valid"};
  sc_signal<sc_bv<KP_programCounterBits>> io_iflush_pcNext{"io_iflush_pcNext"};
  sc_signal<bool> io_dflush_valid{"io_dflush_valid"};
  sc_signal<bool> io_dflush_all{"io_dflush_all"};
  sc_signal<bool> io_dflush_clean{"io_dflush_clean"};

  sc_signal<bool> io_ebus_dbus_valid{"io_ebus_dbus_valid"};
  sc_signal<bool> io_ebus_dbus_ready{"io_ebus_dbus_ready"};
  sc_signal<bool> io_ebus_dbus_write{"io_ebus_dbus_write"};
  sc_signal<bool> io_ebus_internal{"io_ebus_internal"};
  sc_signal<bool> io_ebus_fault_valid{"io_ebus_fault_valid"};
  sc_signal<bool> io_ebus_fault_bits_write{"io_ebus_fault_bits_write"};
  sc_signal<sc_bv<KP_programCounterBits>> io_ebus_dbus_pc{"io_ebus_dbus_pc"};
  sc_signal<sc_bv<32>> io_ebus_dbus_addr{"io_ebus_dbus_addr"};
  sc_signal<sc_bv<32>> io_ebus_dbus_adrx{"io_ebus_dbus_adrx"};
  sc_signal<sc_bv<5>> io_ebus_dbus_size{"io_ebus_dbus_size"};
  sc_signal<sc_bv<128>> io_ebus_dbus_wdata{"io_ebus_dbus_wdata"};
  sc_signal<sc_bv<16>> io_ebus_dbus_wmask{"io_ebus_dbus_wmask"};
  sc_signal<sc_bv<128>> io_ebus_dbus_rdata{"io_ebus_dbus_rdata"};
  sc_signal<sc_bv<32>> io_ebus_fault_bits_addr{"io_ebus_fault_bits_addr"};
  sc_signal<sc_bv<KP_programCounterBits>> io_ebus_fault_bits_epc{"io_ebus_fault_bits_epc"};

  sc_signal<bool> io_dm_debug_req{"io_dm_debug_req"};
  sc_signal<bool> io_dm_resume_req{"io_dm_resume_req"};
  sc_signal<bool> io_dm_csr_valid{"io_dm_csr_valid"};
  sc_signal<bool> io_dm_csr_rd_valid{"io_dm_csr_rd_valid"};
  sc_signal<bool> io_dm_scalar_rd_ready{"io_dm_scalar_rd_ready"};
  sc_signal<bool> io_dm_scalar_rd_valid{"io_dm_scalar_rd_valid"};
  sc_signal<bool> io_dm_float_rd_valid{"io_dm_float_rd_valid"};
  sc_signal<sc_bv<5>> io_dm_csr_bits_addr{"io_dm_csr_bits_addr"};
  sc_signal<sc_bv<12>> io_dm_csr_bits_index{"io_dm_csr_bits_index"};
  sc_signal<sc_bv<5>> io_dm_csr_bits_rs1{"io_dm_csr_bits_rs1"};
  sc_signal<sc_bv<2>> io_dm_csr_bits_op{"io_dm_csr_bits_op"};
  sc_signal<sc_bv<32>> io_dm_csr_rs1{"io_dm_csr_rs1"};
  sc_signal<sc_bv<32>> io_dm_csr_rd_bits{"io_dm_csr_rd_bits"};
  sc_signal<sc_bv<5>> io_dm_scalar_rd_bits_addr{"io_dm_scalar_rd_bits_addr"};
  sc_signal<sc_bv<32>> io_dm_scalar_rd_bits_data{"io_dm_scalar_rd_bits_data"};
  sc_signal<sc_bv<5>> io_dm_scalar_rs_idx{"io_dm_scalar_rs_idx"};
  sc_signal<sc_bv<32>> io_dm_scalar_rs_data{"io_dm_scalar_rs_data"};
  sc_signal<sc_bv<5>> io_dm_float_rd_addr{"io_dm_float_rd_addr"};
  sc_signal<sc_bv<23>> io_dm_float_rd_data_mantissa{"io_dm_float_rd_data_mantissa"};
  sc_signal<sc_bv<8>> io_dm_float_rd_data_exponent{"io_dm_float_rd_data_exponent"};
  sc_signal<sc_bv<5>> io_dm_float_rs_addr{"io_dm_float_rs_addr"};
  sc_signal<sc_bv<23>> io_dm_float_rs_data_mantissa{"io_dm_float_rs_data_mantissa"};
  sc_signal<sc_bv<8>> io_dm_float_rs_data_exponent{"io_dm_float_rs_data_exponent"};
  sc_signal<bool> io_dm_debug_mode{"io_dm_debug_mode"};
  sc_signal<bool> io_dm_float_rd_data_sign{"io_dm_float_rd_data_sign"};
  sc_signal<bool> io_dm_float_rs_data_sign{"io_dm_float_rs_data_sign"};
  sc_signal<bool> io_dm_float_rs_valid{"io_dm_float_rs_valid"};

#if KP_exposeDebugPorts
#define DECLARE_DEBUG_ADDR(x) sc_signal<sc_bv<32>> io_debug_addr_##x{"io_debug_addr_" #x};
#define DECLARE_DEBUG_INST(x) sc_signal<sc_bv<32>> io_debug_inst_##x{"io_debug_inst_" #x};
#define DECLARE_DEBUG_DISPATCH(x) \
  sc_signal<bool> io_debug_dispatch_##x##_instFire{"io_debug_dispatch_" #x "_instFire"}; \
  sc_signal<sc_bv<KP_programCounterBits>> io_debug_dispatch_##x##_instAddr{"io_debug_dispatch_" #x "_instAddr"}; \
  sc_signal<sc_bv<32>> io_debug_dispatch_##x##_instInst{"io_debug_dispatch_" #x "_instInst"};

  CORALNPU_SIM_REPEAT_4(DECLARE_DEBUG_ADDR)
  CORALNPU_SIM_REPEAT_4(DECLARE_DEBUG_INST)
  CORALNPU_SIM_REPEAT_4(DECLARE_DEBUG_DISPATCH)

#undef DECLARE_DEBUG_ADDR
#undef DECLARE_DEBUG_INST
#undef DECLARE_DEBUG_DISPATCH

#define DECLARE_REGFILE_WRITE_ADDR(x) \
  sc_signal<bool> io_debug_regfile_writeAddr_##x##_valid{"io_debug_regfile_writeAddr_" #x "_valid"}; \
  sc_signal<sc_bv<5>> io_debug_regfile_writeAddr_##x##_bits{"io_debug_regfile_writeAddr_" #x "_bits"};

#define DECLARE_REGFILE_WRITE_DATA(x) \
  sc_signal<bool> io_debug_regfile_writeData_##x##_valid{"io_debug_regfile_writeData_" #x "_valid"}; \
  sc_signal<sc_bv<5>> io_debug_regfile_writeData_##x##_bits_addr{"io_debug_regfile_writeData_" #x "_bits_addr"}; \
  sc_signal<sc_bv<32>> io_debug_regfile_writeData_##x##_bits_data{"io_debug_regfile_writeData_" #x "_bits_data"};

  CORALNPU_SIM_REPEAT_4(DECLARE_REGFILE_WRITE_ADDR)
  CORALNPU_SIM_REPEAT_6(DECLARE_REGFILE_WRITE_DATA)

#undef DECLARE_REGFILE_WRITE_ADDR
#undef DECLARE_REGFILE_WRITE_DATA

#define DECLARE_CSR_OUT(x) sc_signal<sc_bv<32>> io_csr_out_value_##x{"io_csr_out_value_" #x};
  CORALNPU_SIM_REPEAT_17(DECLARE_CSR_OUT)
#undef DECLARE_CSR_OUT

#define DECLARE_CSR_IN(x) sc_signal<sc_bv<32>> io_csr_in_value_##x{"io_csr_in_value_" #x};
  CORALNPU_SIM_REPEAT_13(DECLARE_CSR_IN)
#undef DECLARE_CSR_IN

#define DECLARE_DEBUG_RB(x) \
  sc_signal<bool> io_debug_rb_inst_##x##_valid{"io_debug_rb_inst_" #x "_valid"}; \
  sc_signal<sc_bv<KP_programCounterBits>> io_debug_rb_inst_##x##_bits_pc{"io_debug_rb_inst_" #x "_bits_pc"}; \
  sc_signal<sc_bv<32>> io_debug_rb_inst_##x##_bits_inst{"io_debug_rb_inst_" #x "_bits_inst"}; \
  sc_signal<sc_bv<7>> io_debug_rb_inst_##x##_bits_idx{"io_debug_rb_inst_" #x "_bits_idx"}; \
  sc_signal<sc_bv<128>> io_debug_rb_inst_##x##_bits_data{"io_debug_rb_inst_" #x "_bits_data"}; \
  sc_signal<bool> io_debug_rb_inst_##x##_bits_trap{"io_debug_rb_inst_" #x "_bits_trap"};

  CORALNPU_SIM_REPEAT_8(DECLARE_DEBUG_RB)
#undef DECLARE_DEBUG_RB

#define DECLARE_DEBUG_RB_VEC_SLOT(x) \
  sc_signal<bool> io_debug_rb_inst_##x##_bits_vecWrites_0_valid{"io_debug_rb_inst_" #x "_bits_vecWrites_0_valid"}; \
  sc_signal<sc_bv<128>> io_debug_rb_inst_##x##_bits_vecWrites_0_bits_data{"io_debug_rb_inst_" #x "_bits_vecWrites_0_bits_data"}; \
  sc_signal<sc_bv<5>> io_debug_rb_inst_##x##_bits_vecWrites_0_bits_idx{"io_debug_rb_inst_" #x "_bits_vecWrites_0_bits_idx"}; \
  sc_signal<bool> io_debug_rb_inst_##x##_bits_vecWrites_1_valid{"io_debug_rb_inst_" #x "_bits_vecWrites_1_valid"}; \
  sc_signal<sc_bv<128>> io_debug_rb_inst_##x##_bits_vecWrites_1_bits_data{"io_debug_rb_inst_" #x "_bits_vecWrites_1_bits_data"}; \
  sc_signal<sc_bv<5>> io_debug_rb_inst_##x##_bits_vecWrites_1_bits_idx{"io_debug_rb_inst_" #x "_bits_vecWrites_1_bits_idx"}; \
  sc_signal<bool> io_debug_rb_inst_##x##_bits_vecWrites_2_valid{"io_debug_rb_inst_" #x "_bits_vecWrites_2_valid"}; \
  sc_signal<sc_bv<128>> io_debug_rb_inst_##x##_bits_vecWrites_2_bits_data{"io_debug_rb_inst_" #x "_bits_vecWrites_2_bits_data"}; \
  sc_signal<sc_bv<5>> io_debug_rb_inst_##x##_bits_vecWrites_2_bits_idx{"io_debug_rb_inst_" #x "_bits_vecWrites_2_bits_idx"}; \
  sc_signal<bool> io_debug_rb_inst_##x##_bits_vecWrites_3_valid{"io_debug_rb_inst_" #x "_bits_vecWrites_3_valid"}; \
  sc_signal<sc_bv<128>> io_debug_rb_inst_##x##_bits_vecWrites_3_bits_data{"io_debug_rb_inst_" #x "_bits_vecWrites_3_bits_data"}; \
  sc_signal<sc_bv<5>> io_debug_rb_inst_##x##_bits_vecWrites_3_bits_idx{"io_debug_rb_inst_" #x "_bits_vecWrites_3_bits_idx"}; \
  sc_signal<bool> io_debug_rb_inst_##x##_bits_vecWrites_4_valid{"io_debug_rb_inst_" #x "_bits_vecWrites_4_valid"}; \
  sc_signal<sc_bv<128>> io_debug_rb_inst_##x##_bits_vecWrites_4_bits_data{"io_debug_rb_inst_" #x "_bits_vecWrites_4_bits_data"}; \
  sc_signal<sc_bv<5>> io_debug_rb_inst_##x##_bits_vecWrites_4_bits_idx{"io_debug_rb_inst_" #x "_bits_vecWrites_4_bits_idx"}; \
  sc_signal<bool> io_debug_rb_inst_##x##_bits_vecWrites_5_valid{"io_debug_rb_inst_" #x "_bits_vecWrites_5_valid"}; \
  sc_signal<sc_bv<128>> io_debug_rb_inst_##x##_bits_vecWrites_5_bits_data{"io_debug_rb_inst_" #x "_bits_vecWrites_5_bits_data"}; \
  sc_signal<sc_bv<5>> io_debug_rb_inst_##x##_bits_vecWrites_5_bits_idx{"io_debug_rb_inst_" #x "_bits_vecWrites_5_bits_idx"}; \
  sc_signal<bool> io_debug_rb_inst_##x##_bits_vecWrites_6_valid{"io_debug_rb_inst_" #x "_bits_vecWrites_6_valid"}; \
  sc_signal<sc_bv<128>> io_debug_rb_inst_##x##_bits_vecWrites_6_bits_data{"io_debug_rb_inst_" #x "_bits_vecWrites_6_bits_data"}; \
  sc_signal<sc_bv<5>> io_debug_rb_inst_##x##_bits_vecWrites_6_bits_idx{"io_debug_rb_inst_" #x "_bits_vecWrites_6_bits_idx"}; \
  sc_signal<bool> io_debug_rb_inst_##x##_bits_vecWrites_7_valid{"io_debug_rb_inst_" #x "_bits_vecWrites_7_valid"}; \
  sc_signal<sc_bv<128>> io_debug_rb_inst_##x##_bits_vecWrites_7_bits_data{"io_debug_rb_inst_" #x "_bits_vecWrites_7_bits_data"}; \
  sc_signal<sc_bv<5>> io_debug_rb_inst_##x##_bits_vecWrites_7_bits_idx{"io_debug_rb_inst_" #x "_bits_vecWrites_7_bits_idx"};

  CORALNPU_SIM_REPEAT_8(DECLARE_DEBUG_RB_VEC_SLOT)
#undef DECLARE_DEBUG_RB_VEC_SLOT

  sc_signal<bool> io_debug_float_writeAddr_valid{"io_debug_float_writeAddr_valid"};
  sc_signal<bool> io_debug_float_writeData_0_valid{"io_debug_float_writeData_0_valid"};
  sc_signal<bool> io_debug_float_writeData_1_valid{"io_debug_float_writeData_1_valid"};
  sc_signal<sc_bv<5>> io_debug_float_writeAddr_bits{"io_debug_float_writeAddr_bits"};
  sc_signal<sc_bv<5>> io_debug_float_writeData_0_bits_addr{"io_debug_float_writeData_0_bits_addr"};
  sc_signal<sc_bv<32>> io_debug_float_writeData_0_bits_data{"io_debug_float_writeData_0_bits_data"};
  sc_signal<sc_bv<5>> io_debug_float_writeData_1_bits_addr{"io_debug_float_writeData_1_bits_addr"};
  sc_signal<sc_bv<32>> io_debug_float_writeData_1_bits_data{"io_debug_float_writeData_1_bits_data"};
  sc_signal<bool> io_debug_dbus_valid{"io_debug_dbus_valid"};
  sc_signal<sc_bv<32>> io_debug_dbus_bits_addr{"io_debug_dbus_bits_addr"};
  sc_signal<sc_bv<128>> io_debug_dbus_bits_wdata{"io_debug_dbus_bits_wdata"};
  sc_signal<bool> io_debug_dbus_bits_write{"io_debug_dbus_bits_write"};
#endif

  void Bind(VCoreBarebones& core) {
    core.clock(clock);
    core.reset(reset);
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
    core.io_debug_en(io_debug_en);
    core.io_debug_cycles(io_debug_cycles);
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
    core.io_dm_float_rs_valid(io_dm_float_rs_valid);
    core.io_dm_debug_mode(io_dm_debug_mode);
    core.io_dm_float_rd_data_sign(io_dm_float_rd_data_sign);
    core.io_dm_float_rs_data_sign(io_dm_float_rs_data_sign);

#if KP_exposeDebugPorts
    core.io_debug_float_writeAddr_valid(io_debug_float_writeAddr_valid);
    core.io_debug_float_writeData_0_valid(io_debug_float_writeData_0_valid);
    core.io_debug_float_writeData_1_valid(io_debug_float_writeData_1_valid);
    core.io_debug_float_writeAddr_bits(io_debug_float_writeAddr_bits);
    core.io_debug_float_writeData_0_bits_addr(io_debug_float_writeData_0_bits_addr);
    core.io_debug_float_writeData_0_bits_data(io_debug_float_writeData_0_bits_data);
    core.io_debug_float_writeData_1_bits_addr(io_debug_float_writeData_1_bits_addr);
    core.io_debug_float_writeData_1_bits_data(io_debug_float_writeData_1_bits_data);
    core.io_debug_dbus_valid(io_debug_dbus_valid);
    core.io_debug_dbus_bits_addr(io_debug_dbus_bits_addr);
    core.io_debug_dbus_bits_wdata(io_debug_dbus_bits_wdata);
    core.io_debug_dbus_bits_write(io_debug_dbus_bits_write);

#define BIND_DEBUG_ADDR(x) core.io_debug_addr_##x(io_debug_addr_##x);
#define BIND_DEBUG_INST(x) core.io_debug_inst_##x(io_debug_inst_##x);
#define BIND_DEBUG_DISPATCH(x) \
  core.io_debug_dispatch_##x##_instFire(io_debug_dispatch_##x##_instFire); \
  core.io_debug_dispatch_##x##_instAddr(io_debug_dispatch_##x##_instAddr); \
  core.io_debug_dispatch_##x##_instInst(io_debug_dispatch_##x##_instInst);

  CORALNPU_SIM_REPEAT_4(BIND_DEBUG_ADDR)
  CORALNPU_SIM_REPEAT_4(BIND_DEBUG_INST)
  CORALNPU_SIM_REPEAT_4(BIND_DEBUG_DISPATCH)

#undef BIND_DEBUG_ADDR
#undef BIND_DEBUG_INST
#undef BIND_DEBUG_DISPATCH

#define BIND_REGFILE_WRITE_ADDR(x) \
  core.io_debug_regfile_writeAddr_##x##_valid(io_debug_regfile_writeAddr_##x##_valid); \
  core.io_debug_regfile_writeAddr_##x##_bits(io_debug_regfile_writeAddr_##x##_bits);

#define BIND_REGFILE_WRITE_DATA(x) \
  core.io_debug_regfile_writeData_##x##_valid(io_debug_regfile_writeData_##x##_valid); \
  core.io_debug_regfile_writeData_##x##_bits_addr(io_debug_regfile_writeData_##x##_bits_addr); \
  core.io_debug_regfile_writeData_##x##_bits_data(io_debug_regfile_writeData_##x##_bits_data);

  CORALNPU_SIM_REPEAT_4(BIND_REGFILE_WRITE_ADDR)
  CORALNPU_SIM_REPEAT_6(BIND_REGFILE_WRITE_DATA)

#undef BIND_REGFILE_WRITE_ADDR
#undef BIND_REGFILE_WRITE_DATA

#define BIND_CSR_OUT(x) core.io_csr_out_value_##x(io_csr_out_value_##x);
  CORALNPU_SIM_REPEAT_17(BIND_CSR_OUT)
#undef BIND_CSR_OUT

#define BIND_CSR_IN(x) core.io_csr_in_value_##x(io_csr_in_value_##x);
  CORALNPU_SIM_REPEAT_13(BIND_CSR_IN)
#undef BIND_CSR_IN

#define BIND_DEBUG_RB(x) \
  core.io_debug_rb_inst_##x##_valid(io_debug_rb_inst_##x##_valid); \
  core.io_debug_rb_inst_##x##_bits_pc(io_debug_rb_inst_##x##_bits_pc); \
  core.io_debug_rb_inst_##x##_bits_inst(io_debug_rb_inst_##x##_bits_inst); \
  core.io_debug_rb_inst_##x##_bits_idx(io_debug_rb_inst_##x##_bits_idx); \
  core.io_debug_rb_inst_##x##_bits_data(io_debug_rb_inst_##x##_bits_data); \
  core.io_debug_rb_inst_##x##_bits_trap(io_debug_rb_inst_##x##_bits_trap);

  CORALNPU_SIM_REPEAT_8(BIND_DEBUG_RB)
#undef BIND_DEBUG_RB

#define BIND_DEBUG_RB_VEC_SLOT(x) \
  core.io_debug_rb_inst_##x##_bits_vecWrites_0_valid(io_debug_rb_inst_##x##_bits_vecWrites_0_valid); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_0_bits_data(io_debug_rb_inst_##x##_bits_vecWrites_0_bits_data); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_0_bits_idx(io_debug_rb_inst_##x##_bits_vecWrites_0_bits_idx); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_1_valid(io_debug_rb_inst_##x##_bits_vecWrites_1_valid); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_1_bits_data(io_debug_rb_inst_##x##_bits_vecWrites_1_bits_data); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_1_bits_idx(io_debug_rb_inst_##x##_bits_vecWrites_1_bits_idx); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_2_valid(io_debug_rb_inst_##x##_bits_vecWrites_2_valid); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_2_bits_data(io_debug_rb_inst_##x##_bits_vecWrites_2_bits_data); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_2_bits_idx(io_debug_rb_inst_##x##_bits_vecWrites_2_bits_idx); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_3_valid(io_debug_rb_inst_##x##_bits_vecWrites_3_valid); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_3_bits_data(io_debug_rb_inst_##x##_bits_vecWrites_3_bits_data); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_3_bits_idx(io_debug_rb_inst_##x##_bits_vecWrites_3_bits_idx); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_4_valid(io_debug_rb_inst_##x##_bits_vecWrites_4_valid); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_4_bits_data(io_debug_rb_inst_##x##_bits_vecWrites_4_bits_data); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_4_bits_idx(io_debug_rb_inst_##x##_bits_vecWrites_4_bits_idx); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_5_valid(io_debug_rb_inst_##x##_bits_vecWrites_5_valid); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_5_bits_data(io_debug_rb_inst_##x##_bits_vecWrites_5_bits_data); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_5_bits_idx(io_debug_rb_inst_##x##_bits_vecWrites_5_bits_idx); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_6_valid(io_debug_rb_inst_##x##_bits_vecWrites_6_valid); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_6_bits_data(io_debug_rb_inst_##x##_bits_vecWrites_6_bits_data); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_6_bits_idx(io_debug_rb_inst_##x##_bits_vecWrites_6_bits_idx); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_7_valid(io_debug_rb_inst_##x##_bits_vecWrites_7_valid); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_7_bits_data(io_debug_rb_inst_##x##_bits_vecWrites_7_bits_data); \
  core.io_debug_rb_inst_##x##_bits_vecWrites_7_bits_idx(io_debug_rb_inst_##x##_bits_vecWrites_7_bits_idx);

  CORALNPU_SIM_REPEAT_8(BIND_DEBUG_RB_VEC_SLOT)
#undef BIND_DEBUG_RB_VEC_SLOT
#endif
  }
};

TEST(CoreBarebonesRtlTest, ModelInstantiationAndSimulation) {
  VCoreBarebones core("core");
  DummyModelSignals signals;
  signals.Bind(core);

  // Initialize signals and bring model out of reset
  signals.reset.write(true);
  signals.io_ibus_ready.write(true);
  signals.io_dbus_ready.write(true);

  sc_start(10, SC_NS);

  signals.reset.write(false);
  sc_start(50, SC_NS);

  // Assert simulation time actually advanced
  EXPECT_GT(sc_time_stamp().value(), 0);
}



} // namespace

int sc_main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
