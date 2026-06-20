#!/bin/bash
set -e

# Build the simulators
echo "Building simulators..."

# Compile Barebones
g++ -std=c++20 -pthread -DCORALNPU_SIMD=128 -I. -I/usr/local/google/home/julianmb/.gemini/tmp/coralnpu-verilator-core -DVERILATOR_MODEL=VCoreBarebones -include cstdlib -include verilated_fst_c.h -c tests/verilator_sim/coralnpu/core_barebones_tb.cc -o /tmp/core_barebones_tb.o
g++ -std=c++20 -pthread -DCORALNPU_SIMD=128 -I. -I/usr/local/google/home/julianmb/.gemini/tmp/coralnpu-verilator-core -c tests/verilator_sim/elf.cc -o /tmp/elf.o

# Create main wrapper if not exists
cat << 'EOF' > /tmp/main.cc
#include <iostream>
extern int sc_main(int argc, char* argv[]);
int main(int argc, char* argv[]) {
  return sc_main(argc, argv);
}
EOF
g++ -std=c++20 -pthread -c /tmp/main.cc -o /tmp/main.o

g++ /tmp/core_barebones_tb.o /tmp/elf.o /tmp/main.o -o /tmp/core_barebones_tb_bin

# Compile RVVI
g++ -std=c++20 -pthread -DCORALNPU_SIMD=128 -I. -I/usr/local/google/home/julianmb/.gemini/tmp/coralnpu-verilator-core -DVERILATOR_MODEL=VCoreVerification -include cstdlib -include verilated_fst_c.h -c tests/verilator_sim/coralnpu/core_rvvi_tb.cc -o /tmp/core_rvvi_tb.o
g++ -std=c++20 -pthread -DCORALNPU_SIMD=128 -I. -I/usr/local/google/home/julianmb/.gemini/tmp/coralnpu-verilator-core -c tests/verilator_sim/rvvi/trace_daemon.cc -o /tmp/trace_daemon.o

g++ /tmp/core_rvvi_tb.o /tmp/elf.o /tmp/trace_daemon.o -o /tmp/core_rvvi_tb_bin

# Generate a valid ELF that loads at 0x80000000 with 600,000 NOPs
echo "Generating ELF at 0x80000000 with 600,000 NOPs..."
cat << 'EOF' > /tmp/gen_timeout_elf.cc
#include <vector>
#include <cstring>
#include <elf.h>
#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) {
  if (argc < 2) return 1;
  
  const int NUM_NOPS = 600000;
  const int PAYLOAD_SIZE = NUM_NOPS * 4;

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
  phdr.p_filesz = PAYLOAD_SIZE;
  phdr.p_memsz = PAYLOAD_SIZE;
  phdr.p_flags = PF_R | PF_X;
  phdr.p_align = 4;

  std::vector<uint32_t> payload(NUM_NOPS, 0x00000013); // NOPs

  std::ofstream ofs(argv[1], std::ios::binary);
  ofs.write(reinterpret_cast<const char*>(&ehdr), sizeof(ehdr));
  ofs.write(reinterpret_cast<const char*>(&phdr), sizeof(phdr));
  ofs.write(reinterpret_cast<const char*>(payload.data()), PAYLOAD_SIZE);
  return 0;
}
EOF

g++ /tmp/gen_timeout_elf.cc -o /tmp/gen_timeout_elf
/tmp/gen_timeout_elf /tmp/timeout.elf

# Run Barebones simulator and expect timeout
echo "Running Barebones simulator (expecting timeout)..."
set +e
/tmp/core_barebones_tb_bin /tmp/timeout.elf > /tmp/barebones_out.log 2>&1
EXIT_CODE=$?
set -e

cat /tmp/barebones_out.log

if [ $EXIT_CODE -ne 1 ]; then
  echo "Barebones simulator did not exit with code 1 (Exit Code: $EXIT_CODE)"
  exit 1
fi

if ! grep -q "Simulation TIMEOUT" /tmp/barebones_out.log; then
  echo "Barebones simulator output missing 'Simulation TIMEOUT'"
  exit 1
fi

echo "Barebones simulator timed out as expected."

# Run RVVI simulator and expect timeout
echo "Running RVVI simulator (expecting timeout)..."
set +e
/tmp/core_rvvi_tb_bin /tmp/timeout.elf > /tmp/rvvi_out.log 2>&1
EXIT_CODE=$?
set -e

cat /tmp/rvvi_out.log

if [ $EXIT_CODE -ne 1 ]; then
  echo "RVVI simulator did not exit with code 1 (Exit Code: $EXIT_CODE)"
  exit 1
fi

if ! grep -q "Simulation TIMEOUT" /tmp/rvvi_out.log; then
  echo "RVVI simulator output missing 'Simulation TIMEOUT'"
  exit 1
fi

echo "RVVI simulator timed out as expected."

echo "E2E Timeout Verification Test PASSED"
