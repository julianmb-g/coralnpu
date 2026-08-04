<!--
 Copyright 2026 Google LLC

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
-->

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

# Instruction details and decoder logic

> **Intended Audience:** SW/Compiler Devs, HW Devs

## High-level instruction encodings overview

The CoralNPU instruction set architecture relies heavily on standard RISC-V Vector (RVV) encodings, supplemented by custom matrix multiplication and tensor extensions. The central vector decoder is responsible for translating the 32-bit architectural instructions (and 16-bit compressed variants) into internal execution micro-operations.

The decoding process parses standard RISC-V vector fields including:
* **funct6**: The primary 6-bit operation code.
* **vm**: The vector mask bit (0 = masked, 1 = unmasked).
* **vs1, vs2, vd**: Source and destination register identifiers.

The primary RVV opcodes handled by the central decoder (`RvvS1DecodeInstruction`) map standard instruction bits directly to operational units. The high-level opcodes evaluated in the first stage correspond to vector arithmetic (`b1010111`) and configuration commands.

### Decoder logic and field definitions

The exact logic mapping the 32-bit instruction string to specific unit sub-decoders (e.g., `s1decode_opivv`, `s1decode_opfvf`) is explicitly defined in the `RvvS1DecodeInstructionBase` class.

| Instruction Category | Bits/Opcodes | Decoder Method |
| :--- | :--- | :--- |
| **Integer Vector-Vector** (OPIVV) | `funct3 = b000` | `s1decode_opivv` |
| **Float Vector-Vector** (OPFVV) | `funct3 = b001` | `s1decode_opfvv` |
| **Integer Vector-Immediate** (OPIVI)| `funct3 = b011` | `s1decode_opivi` |
| **Integer Vector-Scalar** (OPIVX) | `funct3 = b100` | `s1decode_opivx` |
| **Float Vector-Scalar** (OPFVF) | `funct3 = b101` | `s1decode_opfvf` |

## Scheduling constraints and execution side-effects

* **Structural Hazards**: Instructions dispatched to the Vector ALU (VALU) and Vector Floating-Point Unit (VFPU) may stall if the Reorder Buffer (ROB) is full, or if the maximum in-flight micro-operation capacity is reached (`NUM_DP_UOP`).
* **Masking Side-Effects**: Masked instructions (`vm = 0`) must perform a read-modify-write operation on the destination register (`vd`) unless the mask is fully homogeneous. This can introduce artificial Read-After-Write (RAW) data dependencies.
* **Exception Discard**: As noted in the ROB microarchitecture, floating-point execution side-effects (e.g., division by zero, overflow) generated during vector execution are silently discarded at the core boundary and do not update the architectural `fcsr`. This imposes a strict scheduling constraint where the compiler must not rely on architectural traps for vector arithmetic validation (Implementation Reality).

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/coralnpu/rvv/RvvDecode.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
