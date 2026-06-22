#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./gen_ebreak_elf.cc ./gen_ebreak_elf ./ebreak.elf' EXIT

# Generate a valid ELF that loads at 0x00000000 with ebreak...
echo "Generating ELF at 0x00000000 with ebreak..."
cat << 'EOF' > ./gen_ebreak_elf.cc
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

  uint32_t payload = 0x00100073; // ebreak

  std::ofstream ofs(argv[1], std::ios::binary);
  ofs.write(reinterpret_cast<const char*>(&ehdr), sizeof(ehdr));
  ofs.write(reinterpret_cast<const char*>(&phdr), sizeof(phdr));
  ofs.write(reinterpret_cast<const char*>(&payload), sizeof(payload));
  return 0;
}
EOF

# Compile helper in Podman
echo "Compiling helper in Podman..."
podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -w $PWD localhost/coralnpu g++ ./gen_ebreak_elf.cc -o ./gen_ebreak_elf

# Generate ELF in Podman
echo "Generating ELF in Podman..."
podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -w $PWD localhost/coralnpu ./gen_ebreak_elf $PWD/ebreak.elf

# Run simulator via Bazel in Podman
echo "Running RVVI simulator via Bazel in Podman..."
mkdir -p ./tmp_log
podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_rvvi_sim -- --rvvi_out=\$PWD/trace.rvvi \$PWD/ebreak.elf 2>&1 | tee /tmp/sim.log || (cp /tmp/sim.log ./tmp_log/ebreak_sim.log; find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null; exit 1)"

echo "Checking RVVI trace output..."
if ! grep -q "00100073" trace.rvvi; then
  echo "Trace file missing expected instruction (00100073)"
  exit 1
fi

echo "E2E Ebreak Termination Test PASSED"