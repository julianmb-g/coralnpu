#include <gtest/gtest.h>
#include <vector>
#include <cstring>
#include <elf.h>
#include "tests/verilator_sim/elf.h"

// Mock Memory Interface
class MockMemory {
 public:
  static constexpr size_t kMemSize = 0x20000; // 128KB to cover 0x10000
  MockMemory() : data_(kMemSize, 0) {}
  uint8_t* GetPtr() { return data_.data(); }
  
  void Write(uint64_t addr, size_t count, const uint8_t* src) {
    if (addr < kMemSize && addr + count <= kMemSize) {
      std::memcpy(data_.data() + addr, src, count);
    }
  }

  const std::vector<uint8_t>& data() const { return data_; }

 private:
  std::vector<uint8_t> data_;
};

TEST(BarebonesMemoryTest, LoadElf) {
  MockMemory mem;

  // Construct a minimal valid ELF 32-bit little-endian
  std::vector<uint8_t> elf_data;
  Elf32_Ehdr ehdr;
  std::memset(&ehdr, 0, sizeof(ehdr));
  ehdr.e_ident[EI_MAG0] = ELFMAG0;
  ehdr.e_ident[EI_MAG1] = ELFMAG1;
  ehdr.e_ident[EI_MAG2] = ELFMAG2;
  ehdr.e_ident[EI_MAG3] = ELFMAG3;
  ehdr.e_ident[EI_CLASS] = ELFCLASS32;
  ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
  ehdr.e_ident[EI_VERSION] = EV_CURRENT;
  ehdr.e_type = ET_EXEC;
  ehdr.e_machine = EM_RISCV;
  ehdr.e_version = EV_CURRENT;
  ehdr.e_entry = 0x00010000;
  ehdr.e_phoff = sizeof(Elf32_Ehdr);
  ehdr.e_ehsize = sizeof(Elf32_Ehdr);
  ehdr.e_phentsize = sizeof(Elf32_Phdr);
  ehdr.e_phnum = 1;
  ehdr.e_shentsize = sizeof(Elf32_Shdr);
  ehdr.e_shnum = 0;
  ehdr.e_shstrndx = SHN_UNDEF;

  Elf32_Phdr phdr;
  std::memset(&phdr, 0, sizeof(phdr));
  phdr.p_type = PT_LOAD;
  phdr.p_offset = sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr);
  phdr.p_vaddr = 0x00010000;
  phdr.p_paddr = 0x00010000;
  phdr.p_filesz = 4;
  phdr.p_memsz = 4;
  phdr.p_flags = PF_R | PF_X;
  phdr.p_align = 4;

  uint32_t payload = 0xdeadbeef;

  elf_data.resize(sizeof(ehdr) + sizeof(phdr) + sizeof(payload));
  std::memcpy(elf_data.data(), &ehdr, sizeof(ehdr));
  std::memcpy(elf_data.data() + sizeof(ehdr), &phdr, sizeof(phdr));
  std::memcpy(elf_data.data() + sizeof(ehdr) + sizeof(phdr), &payload, sizeof(payload));

  auto copy_fn = [&mem](void* dest, const void* src, size_t count) -> void* {
    uint64_t addr = reinterpret_cast<uint64_t>(dest);
    mem.Write(addr, count, reinterpret_cast<const uint8_t*>(src));
    return dest;
  };

  uint32_t entry_point = LoadElf(elf_data.data(), copy_fn);

  EXPECT_EQ(entry_point, 0x00010000);
  
  uint32_t loaded_payload = 0;
  std::memcpy(&loaded_payload, mem.data().data() + 0x00010000, sizeof(loaded_payload));
  EXPECT_EQ(loaded_payload, 0xdeadbeef);
}
