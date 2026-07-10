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

> **Intended Audience:** Hardware Developers

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

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
| :----------------- | :-------------------------- | :------------------------- | :----------------------------------------------------- |
| `io.req.ready`     | Output                      | 1-bit                      | Indicates DVU is ready to accept a new request.        |
| `io.req.valid`     | Input                       | 1-bit                      | Indicates a valid request is present.                  |
| `io.req.bits.addr` | Input                       | `log2Ceil(scalarRegCount)` | Destination register address for the operation.        |
| `io.req.bits.op`   | Input                       | 2-bit (`DvuOp`)            | Operation type: `DIV`, `DIVU`, `REM`, or `REMU`.       |
| `io.rs1.valid`     | Input                       | 1-bit                      | Indicates valid data on Operand 1.                     |
| `io.rs1.data`      | Input                       | `XLEN`                     | Operand 1 data (Dividend).                             |
| `io.rs2.valid`     | Input                       | 1-bit                      | Indicates valid data on Operand 2.                     |
| `io.rs2.data`      | Input                       | `XLEN`                     | Operand 2 data (Divisor).                              |
| `io.rd.ready`      | Input                       | 1-bit                      | Indicates downstream is ready to accept the result.    |
| `io.rd.valid`      | Output                      | 1-bit                      | Indicates valid result is available.                   |
| `io.rd.bits.addr`  | Output                      | `log2Ceil(scalarRegCount)` | Destination register address (pipelined from request). |
| `io.rd.bits.data`  | Output                      | `XLEN`                     | Result data (Quotient or Remainder).                   |

## Verification

This hardware block is validated as part of the top-level simulation and verification environment.

- [CoralNPU Top-Level Testbench](../../../tests/uvm/tb/coralnpu_tb_top.sv)
- [Core Mini AXI Verilator Testbench](../../../tests/verilator_sim/coralnpu/core_mini_axi_tb.cc)

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-08 - **Upstream Commit:** 035ec635c72146b99536f71197d75a0618f40bf1 - **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Dvu.scala:L39`, `hdl/chisel/src/coralnpu/scalar/Dvu.scala:L51`, `hdl/chisel/src/coralnpu/scalar/Dvu.scala:L78`, `hdl/chisel/src/coralnpu/scalar/Dvu.scala:L83`, `hdl/chisel/src/coralnpu/scalar/Dvu.scala:L99` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
<!-- mdformat on -->
> **Traceability:** Generated by Gemini. Derived from upstream commit 7d6e35cff17732cad8f99b866333423ea1f4d750.
