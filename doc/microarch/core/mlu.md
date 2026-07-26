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

> **Intended Audience:** HW Devs

The Multiplier Unit (MLU) is responsible for executing integer multiplication instructions within the CoralNPU scalar core. It supports standard RISC-V M-extension 32-bit multiplication operations, providing both lower and upper half products with various sign-extension semantics.

## Supported Operations

The MLU handles the following operations, defined in the `MluOp` Chisel Enum ([Source](hdl/chisel/src/coralnpu/scalar/Mlu.scala)):

- `MUL`: 32-bit $\times$ 32-bit multiplication, returning the lower 32 bits.
- `MULH`: Signed 32-bit $\times$ Signed 32-bit multiplication, returning the upper 32 bits.
- `MULHSU`: Signed 32-bit $\times$ Unsigned 32-bit multiplication, returning the upper 32 bits.
- `MULHU`: Unsigned 32-bit $\times$ Unsigned 32-bit multiplication, returning the upper 32 bits.

## Interface Specifications

The MLU is integrated into the scalar execution pipeline with parameterized multi-lane issue capabilities (`p.instructionLanes`).

| Interface Group | Port  | Type               | Description |
| --------------- | ----- | ------------------ | ----------- |
| Decode | `req` | `Flipped(Decoupled)` | Issue request containing target register address and `MluOp` per lane. |
| Execute | `rs1` | `Flipped(RegfileReadDataIO)` | Register file read data for source operand 1. |
| Execute | `rs2` | `Flipped(RegfileReadDataIO)` | Register file read data for source operand 2. |
| Execute | `rd` | `Decoupled(Flipped(RegfileWriteDataIO))` | Result output to register file write port. |

## Pipeline Stages

The MLU is a pipelined unit that processes instructions over three logical stages:

1. **Stage 1 (Select & Decode)**: An `Arbiter` selects an incoming request from the multiple instruction lanes. The target register, operation, and one-hot lane selection mask are registered in a 1-entry `Queue` buffer (`stage2Input`).
2. **Stage 2 (Multiplication)**: Source operands `rs1` and `rs2` are multiplexed from the active lane. Depending on the `MluOp`, the operands are appropriately sign-extended to 33-bits. The core hardware multiplier computes the full 66-bit signed product (`prod = rs1s * rs2s`), which is then buffered into a 1-entry `Queue` (`stage3Input`).
3. **Stage 3 (Output)**: A multiplexer extracts either the lower 32 bits (`MUL`) or the upper 32 bits (`MULH`, `MULHSU`, `MULHU`) of the product and drives it to the `rd` port with a registered valid signal.

--------------------------------------------------------------------------------

**Provenance & Traceability**
- **Verified As Of:** 2026-07-25
- **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7)
- **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Mlu.scala`
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
