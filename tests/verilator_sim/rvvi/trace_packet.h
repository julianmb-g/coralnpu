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

#include <cstddef>
#include <cstdint>

namespace mpact::sim::riscv::rvvi {

struct alignas(64) TracePacket {
  uint64_t pc; // 64-bit Program Counter (for 'I' type)
  union {
    uint8_t raw_bytes[32]; // 32-byte payload for register writes or disassembly
                           // string (type 'I')
    uint64_t raw_words[4];
  };
  uint32_t inst;   // 32-bit instruction opcode (for 'I' type)
  uint16_t offset; // Byte offset within the register (for multi-chunk writes)
  uint16_t total_size; // Total size of the register in bytes
  uint8_t type;     // 'I' (Instruction), 'R' (Register Chunk), 'E' (Terminate)
  uint8_t reg_type; // 'X' (GPR), 'F' (FPR), 'V' (Vector), or 0 if none
  uint16_t reg_index;  // Register index (0-31 or 12-bit CSR address)
  uint8_t chunk_size;  // Active length of the value chunk in bytes (up to 32)
  uint8_t padding[11]; // Padding to ensure exactly 64 bytes size and cache
                       // alignment
};

static_assert(sizeof(TracePacket) == 64,
              "TracePacket must be exactly 64 bytes");
static_assert(offsetof(TracePacket, pc) == 0,
              "TracePacket.pc must be at offset 0");

} // namespace mpact::sim::riscv::rvvi

#endif // TESTS_VERILATOR_SIM_RVVI_TRACE_PACKET_H_
