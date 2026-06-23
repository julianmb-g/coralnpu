#include <vector>
#include <cstring>
#include <elf.h>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <output_elf> [--address hex] [instructions_hex|--repeat count hex]..." << std::endl;
    return 1;
  }
  
  std::vector<uint32_t> payload;
  uint32_t load_address = 0x00000000;
  for (int i = 2; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--address") {
      if (i + 1 < argc) {
        load_address = std::stoul(argv[i+1], nullptr, 16);
        i += 1;
      } else {
        std::cerr << "--address requires <hex>" << std::endl;
        return 1;
      }
    } else if (arg == "--repeat") {
      if (i + 2 < argc) {
        uint32_t count = std::stoul(argv[i+1]);
        uint32_t inst = std::stoul(argv[i+2], nullptr, 16);
        for (uint32_t c = 0; c < count; ++c) {
          payload.push_back(inst);
        }
        i += 2;
      } else {
        std::cerr << "--repeat requires <count> <hex>" << std::endl;
        return 1;
      }
    } else {
      payload.push_back(std::stoul(arg, nullptr, 16));
    }
  }

  // Default to a single mpause if no instructions provided
  if (payload.empty()) {
    payload.push_back(0x08000073); // mpause
  }

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
  ehdr.e_entry = load_address;
  ehdr.e_phoff = sizeof(Elf32_Ehdr);
  ehdr.e_ehsize = sizeof(Elf32_Ehdr);
  ehdr.e_phentsize = sizeof(Elf32_Phdr);
  ehdr.e_phnum = 1;

  Elf32_Phdr phdr;
  std::memset(&phdr, 0, sizeof(phdr));
  phdr.p_type = PT_LOAD;
  phdr.p_offset = sizeof(Elf32_Ehdr) + sizeof(Elf32_Phdr);
  phdr.p_vaddr = load_address;
  phdr.p_paddr = load_address;
  phdr.p_filesz = payload.size() * sizeof(uint32_t);
  phdr.p_memsz = payload.size() * sizeof(uint32_t);
  phdr.p_flags = PF_R | PF_X;
  phdr.p_align = 4;

  std::ofstream ofs(argv[1], std::ios::binary);
  ofs.write(reinterpret_cast<const char*>(&ehdr), sizeof(ehdr));
  ofs.write(reinterpret_cast<const char*>(&phdr), sizeof(phdr));
  ofs.write(reinterpret_cast<const char*>(payload.data()), payload.size() * sizeof(uint32_t));
  return 0;
}
