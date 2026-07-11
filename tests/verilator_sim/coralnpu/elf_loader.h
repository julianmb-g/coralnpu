#ifndef TESTS_VERILATOR_SIM_CORALNPU_ELF_LOADER_H_
#define TESTS_VERILATOR_SIM_CORALNPU_ELF_LOADER_H_

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <cstring>

#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "tests/verilator_sim/coralnpu/core_if.h"
#include "tests/verilator_sim/elf.h"

inline bool LoadElfToMemory(const std::string& file_name, Core_if& memory_interface, uint32_t& entry_point) {
  int fd = open(file_name.c_str(), O_RDONLY);
  if (fd < 0) {
    LOG(ERROR) << "Failed to open ELF file: " << file_name;
    return false;
  }
  struct stat sb;
  if (fstat(fd, &sb) != 0) {
    close(fd);
    return false;
  }
  auto file_size = sb.st_size;
  auto file_data = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (file_data == MAP_FAILED) {
    close(fd);
    return false;
  }
  close(fd);

  uint32_t elf_magic = 0x464c457f;
  uint8_t* data8 = reinterpret_cast<uint8_t*>(file_data);
  bool load_ok = true;
  if (memcmp(file_data, &elf_magic, sizeof(elf_magic)) == 0) {
    entry_point = ::LoadElf(data8,
              [&memory_interface, &load_ok](void* dest, const void* src, size_t count) {
                uint64_t addr = reinterpret_cast<uint64_t>(dest);
                if (!memory_interface.Write(addr, count, reinterpret_cast<const uint8_t*>(src))) {
                  LOG(ERROR) << absl::StrFormat("[FATAL] ELF load violation. Requested: [0x%08lx - 0x%08lx]. Available: %s. Delta: Exceeds bounds by 0x%lx bytes.", addr, addr + count, memory_interface.GetProfileBounds(), memory_interface.GetOverflowDelta(addr, count));
                  load_ok = false;
                }
                return dest;
              });
    munmap(file_data, file_size);
    return load_ok;
  }
  munmap(file_data, file_size);
  return false;
}

#endif  // TESTS_VERILATOR_SIM_CORALNPU_ELF_LOADER_H_
