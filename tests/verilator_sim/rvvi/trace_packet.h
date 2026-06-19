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

#ifndef TESTS_VERILATOR_SIM_RVVI_TRACE_PACKET_H_
#define TESTS_VERILATOR_SIM_RVVI_TRACE_PACKET_H_

#include <cstdint>

namespace mpact::sim::riscv::rvvi {

struct alignas(64) TracePacket {
  uint8_t type; // 'I' (Instruction), 'T' (Trap), 'R' (Register Update), 'E' (Terminate)
  uint8_t padding[7];
  union {
    struct {
      uint64_t pc;
      uint32_t instruction;
      uint32_t padding;
    } inst;
    struct {
      uint64_t value[4]; // Up to 256 bits of vector/float/GPR state
      uint16_t index;
      uint16_t offset;
      uint16_t total_size;
      uint8_t reg_type;  // 'X' (GPR), 'F' (FPR), 'V' (Vector), 'C' (CSR)
      uint8_t size;      // in bytes
    } reg;
  };
};

} // namespace mpact::sim::riscv::rvvi

#endif  // TESTS_VERILATOR_SIM_RVVI_TRACE_PACKET_H_
