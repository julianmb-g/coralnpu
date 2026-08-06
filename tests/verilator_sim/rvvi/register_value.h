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

#ifndef TESTS_VERILATOR_SIM_RVVI_REGISTER_VALUE_H_
#define TESTS_VERILATOR_SIM_RVVI_REGISTER_VALUE_H_

#include "absl/types/span.h"
#include <cstddef>
#include <cstdint>

namespace mpact::sim::riscv::rvvi {

class RegisterValue {
public:
  RegisterValue() : reg_type_(0), index_(0), current_size_(0) {
    for (size_t i = 0; i < 256; ++i) {
      data_[i] = 0;
    }
  }

  uint8_t RegType() const { return reg_type_; }
  void SetRegType(uint8_t reg_type) { reg_type_ = reg_type; }

  uint16_t Index() const { return index_; }
  void SetIndex(uint16_t index) { index_ = index; }

  uint16_t CurrentSize() const { return current_size_; }
  void SetCurrentSize(uint16_t size) { current_size_ = size; }

  uint8_t *DataPtr() { return data_; }
  const uint8_t *DataPtr() const { return data_; }

  absl::Span<const uint8_t> Data() const {
    return absl::MakeConstSpan(data_, current_size_);
  }

private:
  uint8_t reg_type_;
  uint16_t index_;
  uint16_t current_size_;
  uint8_t data_[256];
};

} // namespace mpact::sim::riscv::rvvi

#endif // TESTS_VERILATOR_SIM_RVVI_REGISTER_VALUE_H_
