#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./gen_rvvi_elf.cc ./gen_rvvi_elf ./rvvi.elf' EXIT

echo "Generating ELF at 0x00000000..."
cat << 'EOF' > ./gen_rvvi_elf.cc
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
  phdr.p_filesz = 8;
  phdr.p_memsz = 8;
  phdr.p_flags = PF_R | PF_X;
  phdr.p_align = 4;

  // Generate 5000 NOPs followed by 1 mpause
  const int num_nops = 5000;
  std::vector<uint32_t> payload(num_nops + 1);
  for (int i = 0; i < num_nops; ++i) {
    payload[i] = 0x00000013; // nop
  }
  payload[num_nops] = 0x08000073; // mpause

  std::ofstream ofs(argv[1], std::ios::binary);
  ofs.write(reinterpret_cast<const char*>(&ehdr), sizeof(ehdr));
  
  phdr.p_filesz = payload.size() * sizeof(uint32_t);
  phdr.p_memsz = payload.size() * sizeof(uint32_t);
  ofs.write(reinterpret_cast<const char*>(&phdr), sizeof(phdr));
  ofs.write(reinterpret_cast<const char*>(payload.data()), payload.size() * sizeof(uint32_t));
  return 0;
}
EOF

# Compile helper in Podman
echo "Compiling helper in Podman..."
podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -w $PWD localhost/coralnpu g++ ./gen_rvvi_elf.cc -o ./gen_rvvi_elf

# Generate ELF in Podman
echo "Generating ELF in Podman..."
podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -w $PWD localhost/coralnpu ./gen_rvvi_elf $PWD/rvvi.elf

# Run simulator via Bazel in Podman with DelayFormatter, explicit output path, and Bazel cache mapping
echo "Running RVVI Backpressure simulator via Bazel in Podman..."
mkdir -p ./tmp_log
time podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run --copt=-DDELAY_FORMATTER //tests/verilator_sim:core_rvvi_sim -- --rvvi_out=\$PWD/trace.rvvi \$PWD/rvvi.elf 2>&1 | tee /tmp/sim.log || (cp /tmp/sim.log ./tmp_log/backpressure_sim.log; find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null; exit 1)"

echo "Checking trace output..."
# Verify 5000 nops and 1 mpause are present
nop_count=$(grep -c "00000013" trace.rvvi)
if [ "$nop_count" -lt 5000 ]; then
  echo "Trace file missing expected NOP instructions: expected at least 5000, found $nop_count"
  exit 1
fi
if ! grep -q "08000073" trace.rvvi; then
  echo "Trace file missing expected instruction (08000073)"
  exit 1
fi

echo "E2E Backpressure Resilience Test PASSED"
