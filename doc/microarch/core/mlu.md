# Multiplier Unit (MLU) Architecture

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

> **Intended Audience:** Hardware Developers

The Multiplier Unit (MLU) is responsible for executing integer multiplication instructions within the CoralNPU scalar core. It supports standard RISC-V M-extension 32-bit multiplication operations, providing both lower and upper half products with various sign-extension semantics.

## Supported Operations

The MLU handles the following operations, defined in the `MluOp` Chisel Enum:

- `MUL`: 32-bit $\times$ 32-bit multiplication, returning the lower 32 bits.
- `MULH`: Signed 32-bit $\times$ Signed 32-bit multiplication, returning the upper 32 bits.
- `MULHSU`: Signed 32-bit $\times$ Unsigned 32-bit multiplication, returning the upper 32 bits.
- `MULHU`: Unsigned 32-bit $\times$ Unsigned 32-bit multiplication, returning the upper 32 bits.

## Interface Specifications

The MLU is integrated into the scalar execution pipeline with parameterized multi-lane issue capabilities (`p.instructionLanes`).

| Interface Group | Port  | Type               | Width / Format                | Description                                                                                                                          |
| --------------- | ----- | ------------------ | ----------------------------- | ------------------------------------------------------------------------------------------------------------------------------------ |
| Decode          | `req` | Input (Decoupled)  | Vector of `MluCmd`            | Decoded multiplication requests, one per instruction lane. Contains destination register address (`addr`) and operation type (`op`). |
| Execute         | `rs1` | Input              | Vector of `RegfileReadDataIO` | Read data from source register 1, per instruction lane.                                                                              |
| Execute         | `rs2` | Input              | Vector of `RegfileReadDataIO` | Read data from source register 2, per instruction lane.                                                                              |
| Execute         | `rd`  | Output (Decoupled) | `RegfileWriteDataIO`          | Output result and destination address sent to the register file.                                                                     |

## Pipeline Architecture

The MLU is internally organized into three logical stages. Flow control between stages is managed using 1-entry Chisel `Queue`s with flow-through enabled (`pipe = true`), allowing back-to-back processing.

### Stage 1: Request Arbitration and Selection

- The MLU uses an `Arbiter` to select exactly one valid multiplication request from the incoming `instructionLanes`.
- The chosen request's operation (`op`), destination address (`rd`), and a one-hot encoded lane selector (`sel`) are advanced to Stage 2.

### Stage 2: Multiplication Execution

- The operands `rs1` and `rs2` are multiplexed from the input arrays based on the one-hot lane selector (`sel2in`).
- **Sign-Extension Logic**:
  - `rs2` is treated as signed if the operation is `MULH`.
  - `rs1` is treated as signed if the operation is `MULH` or `MULHSU`.
- The core multiplication calculates a 66-bit signed product (`prod = rs1s * rs2s`).

### Stage 3: Result Formatting and Output

- A multiplexer selects the appropriate 32-bit slice from the 66-bit product based on the operation type:
  - For `MUL`, the lower 32 bits (`prod3in(31, 0)`) are selected.
  - For `MULH`, `MULHSU`, and `MULHU`, the upper 32 bits (`prod3in(63, 32)`) are selected.
- The formatted result is asserted on `io.rd` along with the destination register address.

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-22 - **Upstream Commit:** 5c2647afd951f70d6244ea06b5a8b7fa1fdf2918 - **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Mlu.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
