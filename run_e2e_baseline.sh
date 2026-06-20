#!/bin/bash
set -e

# Build the simulator
echo "Building simulator..."
cat << 'EOF' > /tmp/main.cc
#include <iostream>
extern int sc_main(int argc, char* argv[]);
int main(int argc, char* argv[]) {
  return sc_main(argc, argv);
}
EOF

# We need to compile with -include cstdlib to fix missing rand in fifo.h
# And -DVERILATOR_MODEL=VCoreBarebones to use the barebones model
g++ -std=c++20 -pthread -DCORALNPU_SIMD=128 -I. -I/usr/local/google/home/julianmb/.gemini/tmp/coralnpu-verilator-core -DVERILATOR_MODEL=VCoreBarebones -include cstdlib -include verilated_fst_c.h -c tests/verilator_sim/coralnpu/core_barebones_tb.cc -o /tmp/core_barebones_tb.o
g++ -std=c++20 -pthread -DCORALNPU_SIMD=128 -I. -I/usr/local/google/home/julianmb/.gemini/tmp/coralnpu-verilator-core -c tests/verilator_sim/elf.cc -o /tmp/elf.o
g++ -std=c++20 -pthread -c /tmp/main.cc -o /tmp/main.o

g++ /tmp/core_barebones_tb.o /tmp/elf.o /tmp/main.o -o /tmp/core_barebones_tb_bin

# Generate a dummy valid ELF that fits in default memory profile (loads at 0x0)
# This is needed because provided external ELFs load at 0x80000000 which is not mapped in mocks.
echo "Generating dummy ELF..."
cat << 'EOF' > /tmp/gen_elf.cc
#include <vector>
#include <cstring>
#include <elf.h>
#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) {
  if (argc < 2) return 1;
  
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
  ehdr.e_entry = 0x00000000;
  ehdr.e_phoff = sizeof(Elf32_Ehdr);
  ehdr.e_ehsize = sizeof(Elf32_Ehdr);
  ehdr.e_phentsize = sizeof(Elf32_Phdr);
  ehdr.e_phnum = 1;

  Elf32_Phdr phdr;
  std::memset(&phdr, 0, sizeof(phdr));
  phdr.p_type = PT_LOAD;
  phdr.p_offset = sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr);
  phdr.p_vaddr = 0x00000000;
  phdr.p_paddr = 0x00000000;
  phdr.p_filesz = 4;
  phdr.p_memsz = 4;
  phdr.p_flags = PF_R | PF_X;
  phdr.p_align = 4;

  uint32_t payload = 0x08000073; // mpause

  std::ofstream ofs(argv[1], std::ios::binary);
  ofs.write(reinterpret_cast<const char*>(&ehdr), sizeof(ehdr));
  ofs.write(reinterpret_cast<const char*>(&phdr), sizeof(phdr));
  ofs.write(reinterpret_cast<const char*>(&payload), sizeof(payload));
  return 0;
}
EOF

g++ /tmp/gen_elf.cc -o /tmp/gen_elf
/tmp/gen_elf /tmp/dummy.elf

# Run the simulator
echo "Running simulator..."
/tmp/core_barebones_tb_bin /tmp/dummy.elf

echo "E2E Baseline Emulation Test PASSED"
