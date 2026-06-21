#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./gen_real_elf.cc ./gen_real_elf ./real.elf' EXIT

# Generate a valid ELF that loads at 0x80000000
echo "Generating ELF at 0x80000000..."
cat << 'EOF' > ./gen_real_elf.cc
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

# Compile helper in Podman
echo "Compiling helper in Podman..."
podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -w $PWD localhost/coralnpu g++ ./gen_real_elf.cc -o ./gen_real_elf

# Generate ELF in Podman
echo "Generating ELF in Podman..."
podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -w $PWD localhost/coralnpu ./gen_real_elf $PWD/real.elf

# Run the simulator
echo "Running simulator with real ELF via Bazel in Podman..."
podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bazel run //tests/verilator_sim:core_barebones_sim -- $PWD/real.elf

echo "E2E Real Address Loading Test PASSED"