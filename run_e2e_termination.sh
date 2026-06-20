#!/bin/bash
set -e

# Build the RVVI simulator
echo "Building RVVI simulator for Termination Test..."

# Compile
g++ -std=c++20 -pthread -DCORALNPU_SIMD=128 -I. -I/usr/local/google/home/julianmb/.gemini/tmp/coralnpu-verilator-core -DVERILATOR_MODEL=VCoreVerification -include cstdlib -include verilated_fst_c.h -c tests/verilator_sim/coralnpu/core_rvvi_tb.cc -o /tmp/core_rvvi_tb.o
g++ -std=c++20 -pthread -DCORALNPU_SIMD=128 -I. -I/usr/local/google/home/julianmb/.gemini/tmp/coralnpu-verilator-core -c tests/verilator_sim/elf.cc -o /tmp/elf.o
g++ -std=c++20 -pthread -DCORALNPU_SIMD=128 -I. -I/usr/local/google/home/julianmb/.gemini/tmp/coralnpu-verilator-core -c tests/verilator_sim/rvvi/trace_daemon.cc -o /tmp/trace_daemon.o

g++ /tmp/core_rvvi_tb.o /tmp/elf.o /tmp/trace_daemon.o -o /tmp/core_rvvi_tb_bin

# Generate a valid ELF that loads at 0x80000000
echo "Generating ELF at 0x80000000..."
cat << 'EOF' > /tmp/gen_rvvi_elf.cc
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
  ehdr.e_entry = 0x80000000;
  ehdr.e_phoff = sizeof(Elf32_Ehdr);
  ehdr.e_ehsize = sizeof(Elf32_Ehdr);
  ehdr.e_phentsize = sizeof(Elf32_Phdr);
  ehdr.e_phnum = 1;

  Elf32_Phdr phdr;
  std::memset(&phdr, 0, sizeof(phdr));
  phdr.p_type = PT_LOAD;
  phdr.p_offset = sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr);
  phdr.p_vaddr = 0x80000000;
  phdr.p_paddr = 0x80000000;
  phdr.p_filesz = 8;
  phdr.p_memsz = 8;
  phdr.p_flags = PF_R | PF_X;
  phdr.p_align = 4;

  uint32_t payload[2];
  payload[0] = 0x00000013; // nop
  payload[1] = 0x08000073; // mpause

  std::ofstream ofs(argv[1], std::ios::binary);
  ofs.write(reinterpret_cast<const char*>(&ehdr), sizeof(ehdr));
  ofs.write(reinterpret_cast<const char*>(&phdr), sizeof(phdr));
  ofs.write(reinterpret_cast<const char*>(payload), sizeof(payload));
  return 0;
}
EOF

g++ /tmp/gen_rvvi_elf.cc -o /tmp/gen_rvvi_elf
/tmp/gen_rvvi_elf /tmp/rvvi.elf

# Run the simulator
echo "Running RVVI simulator..."
/tmp/core_rvvi_tb_bin /tmp/rvvi.elf

echo "Checking RVVI trace output for graceful termination..."
if ! tail -n 1 trace.rvvi | grep -q "08000073"; then
  echo "Trace file's last line is NOT the mpause instruction!"
  tail -n 1 trace.rvvi
  exit 1
fi

echo "E2E Graceful Termination Test PASSED"
