# Divider Unit (DVU) Architecture

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


The Divider Unit (DVU) executes the standard RISC-V integer division and remainder instructions (`DIV`, `DIVU`, `REM`, `REMU`).

## Architecture and Execution Pipeline

Unlike the FMA multiplier, the DVU is a multi-cycle iterative execution unit that computes the result using a radix-2, one-bit-per-cycle restoring division algorithm.

The execution pipeline consists of three phases:

1.  **Setup Phase (1 cycle):**
    - The operands are converted to positive numbers (for signed operations).
    - A leading-zero count (`Clz1`) is performed on the dividend. If the divisor is zero, the leading zero count is forced to 0 and the operation takes the full 32 iterations.
2.  **Iterative Compute Phase (Variable cycles):**
    - The core loop performs one shift-and-subtract operation per cycle (`divide`, `remain`, `denom`).
    - The latency is variable, executing $32 - \text{CLZ}(\text{dividend})$ cycles.
3.  **Completion Phase (1 cycle):**
    - The quotient and remainder are sign-corrected based on the original operand signs.
    - The result is written back to the register file when the write channel (`io.rd`) is ready.

## Early Termination

The DVU implements early termination. By counting the leading zeros of the dividend, the unit skips computation cycles for smaller numbers.

## Edge Cases

- **Division by Zero:**
  - Hardware detects a divisor of 0 (`io.rs2.data === 0.U`).
  - Standard RISC-V semantics are enforced: the quotient evaluates to all 1s (`-1`), and the remainder evaluates to the original dividend.
  - Early termination is bypassed (`clz` forced to 0), and the operation takes the full 32 iteration cycles.

## Interfaces

| Signal             | Direction (DVU Perspective) | Width                      | Description                                            |
| :----------------- | :-------------------------- | :------------------------- | :

--------------------------------------------------------------------------------

**Provenance & Traceability**
- **Verified As Of:** 2026-07-25
- **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7)
- **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Dvu.scala:L39`, `hdl/chisel/src/coralnpu/scalar/Dvu.scala:L51`, `hdl/chisel/src/coralnpu/scalar/Dvu.scala:L78`, `hdl/chisel/src/coralnpu/scalar/Dvu.scala:L83`, `hdl/chisel/src/coralnpu/scalar/Dvu.scala:L99` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
