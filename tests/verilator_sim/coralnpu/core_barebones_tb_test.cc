#include <gtest/gtest.h>
#include <vector>
#include <cstring>
#include "tests/verilator_sim/elf.h"

// Mock Memory Interface
class MockMemory {
 public:
  static constexpr size_t kMemSize = 0x10000; // 64KB
  MockMemory() : data_(kMemSize, 0) {}
  uint8_t* GetPtr() { return data_.data(); }
  
  static void* Copy(void* dest, const void* src, size_t count) {
    return std::memcpy(dest, src, count);
  }

 private:
  std::vector<uint8_t> data_;
};

TEST(BarebonesMemoryTest, LoadElf) {
  MockMemory mem;
  // This is a placeholder test. Actual ELF loading requires a valid ELF binary.
  // For now, we verify that LoadElf handles a null/invalid pointer gracefully 
  // or at least doesn't crash, and check the interface.
  uint32_t entry_point = LoadElf(mem.GetPtr(), MockMemory::Copy);
  // Expectation depends on what LoadElf does with invalid data.
  // Assuming it returns 0 for failure.
  EXPECT_EQ(entry_point, 0);
}
