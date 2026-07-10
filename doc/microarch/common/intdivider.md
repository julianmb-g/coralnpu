# Dedicated Integer Divider

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

The CoralNPU includes a standalone, iterative integer divider primitive (`intdivider`) used to compute quotient and remainder for varying datatypes. It handles both signed and unsigned division through an iterative shift-and-subtract algorithm.

## Parameters

| Parameter   | Type         | Default | Description                                                                                               |
| ----------- | ------------ | ------- | --------------------------------------------------------------------------------------------------------- |
| `DIV_WIDTH` | `logic[7:0]` | 32      | Operand bit width. Parameterizable, but explicitly supports only 8, 16, and 32 (generate block bindings). |

## Interfaces

| Port               | Direction | Width       | Description                                                                                                                                                                                             |
| ------------------ | --------- | ----------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `clk`              | Input     | 1           | Clock signal.                                                                                                                                                                                           |
| `rst_n`            | Input     | 1           | Active-low reset.                                                                                                                                                                                       |
| `opcode`           | Input     | 1           | Division type: `0` for Signed (`DIV_SIGN`), `1` for Unsigned (`DIV_ZERO`).                                                                                                                              |
| `div_valid`        | Input     | 1           | Valid signal for input operands. Must remain high during computation (`DIV_WORKING`) and result presentation (`DIV_PRINT`) until `result_ready` is asserted; dropping it abandons the operation/result. |
| `div_ready`        | Output    | 1           | Ready signal indicating divider is in `DIV_IDLE` state.                                                                                                                                                 |
| `src2_dividend`    | Input     | `DIV_WIDTH` | The dividend operand.                                                                                                                                                                                   |
| `src1_divisor`     | Input     | `DIV_WIDTH` | The divisor operand.                                                                                                                                                                                    |
| `result_quotient`  | Output    | `DIV_WIDTH` | The computed quotient.                                                                                                                                                                                  |
| `result_remainder` | Output    | `DIV_WIDTH` | The computed remainder.                                                                                                                                                                                 |
| `result_valid`     | Output    | 1           | Valid signal indicating the division has completed.                                                                                                                                                     |
| `result_ready`     | Input     | 1           | Ready signal from consumer to accept the result.                                                                                                                                                        |
| `trap_flush_rvv`   | Input     | 1           | Trap flush signal to clear the internal state.                                                                                                                                                          |

## Execution Architecture

The `intdivider` operates using a 3-state Finite State Machine (FSM):

1. **`DIV_IDLE`**: Waits for `div_valid`. Handles sign conversions for signed operations and checks for division-by-zero or signed overflow (-2^(W-1) / -1). It also includes an operand-reuse optimization: if the new operands exactly match the previously computed operands, it bypasses computation.
2. **`DIV_WORKING`**: Executes the iterative division. The initial step counts leading zeros (`f_clzb*` functions) in the dividend. The core loop performs 3 unrolled shift-and-subtract steps (`f_div_step`) per clock cycle. If fewer than 3 steps are remaining to complete the division, the module uses intermediate results from step 1 or 2.
3. **`DIV_PRINT`**: Asserts `result_valid` and applies final sign-inversion if the original operation was signed and required a negative result. Waits for `result_ready` to return to `DIV_IDLE`.

<!-- mdformat off -->

<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

<!-- prettier-ignore-end -->

**Provenance & Traceability** - **Verified As Of:** 2026-07-06 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/verilog/rvv/common/intdivider.sv:L2` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
