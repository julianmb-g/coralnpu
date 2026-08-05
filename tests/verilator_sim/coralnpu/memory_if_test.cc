// Copyright 2026 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tests/verilator_sim/coralnpu/memory_if.h"

#include <string>

#include "absl/status/status.h"
#include "gtest/gtest.h"
#include <systemc>

class MemoryIfTest : public ::testing::Test {
 protected:
  MemoryIfTest() : mem_if_("memory_if_test", "/dev/null") {}

  Memory_if mem_if_;
};

TEST_F(MemoryIfTest, IsValidAddress_DefaultProfile) {
  // Default profile: ITCM [0x0, 0x2000), DTCM [0x10000, 0x18000)
  Memory_if default_if("default_if_test", "/dev/null", -1, "default");
  EXPECT_TRUE(default_if.IsValidAddress(0x0000, 1));
  EXPECT_TRUE(default_if.IsValidAddress(0x10000, 1));
  EXPECT_TRUE(default_if.IsValidAddress(0x0000, 0x2000));

  EXPECT_FALSE(default_if.IsValidAddress(0x1FFF, 2)); // Crosses boundary
  EXPECT_FALSE(default_if.IsValidAddress(0x2000, 1));
  EXPECT_FALSE(default_if.IsValidAddress(0x18000, 1));
}

TEST_F(MemoryIfTest, IsValidAddress_HighMemProfile) {
  // HighMem profile: [0x100000, 0x200000)
  Memory_if highmem_if("highmem_if_test", "/dev/null", -1, Memory_if::kHighMem);
  EXPECT_TRUE(highmem_if.IsValidAddress(0x100000, 1));
  EXPECT_TRUE(highmem_if.IsValidAddress(0x1FFFFF, 1));
  EXPECT_TRUE(highmem_if.IsValidAddress(0x100000, 0x100000));

  EXPECT_FALSE(highmem_if.IsValidAddress(0x000000, 1));
  EXPECT_FALSE(highmem_if.IsValidAddress(0x200000, 1));
}

TEST_F(MemoryIfTest, IsValidAddress_Overflow) {
  EXPECT_FALSE(mem_if_.IsValidAddress(0xFFFFFFFF, 2));
}

TEST_F(MemoryIfTest, GetOverflowDelta) {
  EXPECT_EQ(mem_if_.GetOverflowDelta(0xFFFFFFFF, 2), 1);
  EXPECT_EQ(mem_if_.GetOverflowDelta(0xFFFFFFFE, 3), 1);
  EXPECT_EQ(mem_if_.GetOverflowDelta(0x100000, 0x100000), 0);
}

int sc_main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
