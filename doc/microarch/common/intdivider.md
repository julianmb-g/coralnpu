# Integer divider

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
>
> **Intended Audience:** HW Devs

The CoralNPU includes a standalone, iterative integer divider primitive (`intdivider`) used to compute quotient and remainder for varying datatypes. It handles both signed and unsigned division through an iterative shift-and-subtract algorithm.

## Parameters

| Parameter   | Type         | Default | Description                                                                                               |
| ----------- | ------------ | ------- | --------------------------------------------------------------------------------------------------------- |
| `DIV_WIDTH` | `logic[7:0]` | `8'd32` | Width of the dividend, divisor, quotient, and remainder in bits.                                          |

## Interfaces

| Port Name          | I/O    | Width      | Description                                                                                               |
| ------------------ | ------ | ---------- | --------------------------------------------------------------------------------------------------------- |
| `clk`              | Input  | 1          | Global clock.                                                                                             |
| `rst_n`            | Input  | 1          | Active-low asynchronous reset.                                                                            |
| `div_valid`        | Input  | 1          | Indicates that a new division operation is valid.                                                         |
| `div_ready`        | Output | 1          | Indicates that the divider is ready to accept a new division operation.                                   |
| `opcode`           | Input  | 1          | Specifies operation type (signed vs unsigned).                                                            |
| `src2_dividend`    | Input  | `DIV_WIDTH`| Dividend operand.                                                                                         |
| `src1_divisor`     | Input  | `DIV_WIDTH`| Divisor operand.                                                                                          |
| `result_quotient`  | Output | `DIV_WIDTH`| Result quotient.                                                                                          |
| `result_remainder` | Output | `DIV_WIDTH`| Result remainder.                                                                                         |
| `result_valid`     | Output | 1          | Indicates that the result is valid.                                                                       |
| `result_ready`     | Input  | 1          | Indicates that the consumer is ready to accept the result.                                                |
| `trap_flush_rvv`   | Input  | 1          | Global trap-flush signal.                                                                                 |

## Architecture function summary

The `intdivider` module provides hardware support for integer division and remainder calculations. It is capable of handling special cases such as division by zero and signed overflow (e.g., `-2^(WIDTH-1) / -1`).

[Source: `hdl/verilog/rvv/common/intdivider.sv`](../../../hdl/verilog/rvv/common/intdivider.sv)

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/verilog/rvv/common/intdivider.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
