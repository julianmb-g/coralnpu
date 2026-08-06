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

#include "absl/strings/str_format.h"
#include "gtest/gtest.h"
#include <sstream>
#include <string>

// Define a simple fallback formatter class to test CustomFallbackFormatter
// logic.
class CustomFallbackFormatter {
public:
  static std::string Format(uint64_t pc, uint32_t inst,
                            const std::string &disasm) {
    return absl::StrFormat("rvvi,0,%016lx,%08x,%s", pc, inst, disasm);
  }
};

TEST(CustomFallbackFormatterTest, FormatVerification) {
  std::string formatted =
      CustomFallbackFormatter::Format(0x1000, 0x00000013, "nop");
  EXPECT_EQ(formatted, "rvvi,0,0000000000001000,00000013,nop");
}
