# Vector Backend FALU Array

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

The Vector Backend FALU Array (`rvv_backend_falu.sv`) decodes and dispatches floating-point micro-operations to the underlying execution units. It acts as a multi-lane routing and arbitration wrapper over multiple `rvv_backend_falu_unit.sv` instantiations.

## Architecture and Execution Units

Each FALU unit encapsulates four distinct execution sub-units to handle specific classes of floating-point operations. The pipeline depth is parameterized by `PIPEREGS` (default: 3).

| Sub-unit              | Underlying IP / Module   | Supported Operations                                                                                                                 |
| --------------------- | ------------------------ | ------------------------------------------------------------------------------------------------------------------------------------ |
| **Add/Mul/FMA**       | `fpnew_fma_multi`        | `VFADD`, `VFSUB`, `VFMUL`, `VFRSUB`, `VFMAC`, `VFNMAC`, `VFMSAC`, `VFNMSAC`, `VFMADD`, `VFNMADD`, `VFMSUB`, `VFNMSUB`, `VFWMACCBF16` |
| **Compare/Non-comp**  | `fpnew_noncomp`          | `VMFEQ`, `VMFNE`, `VMFLT`, `VMFLE`, `VMFGT`, `VMFGE`, `VFSGNJ`, `VFSGNJN`, `VFSGNJX`, `VFMIN`, `VFMAX`, `VFCLASS`                    |
| **Convert**           | `fpnew_cast_multi`       | `VFCVT_XUFV`, `VFCVT_XFV`, `VFCVT_FXUV`, `VFCVT_FXV`, `VFNCVTBF16`, `VFWCVTBF16`, `VFCVT_RTZXUFV`, `VFCVT_RTZXFV`                    |
| **Table (Estimates)** | `rvv_backend_sqrt7_rec7` | `VFRECE7`, `VFRSQRT7` (7-bit reciprocal and square-root estimates via lookup tables)                                                 |

## Lane Arbitration and Routing

The `rvv_backend_falu.sv` wrapper supports dynamic cross-lane routing for up to two micro-operations (`NUM_FMA`).

If `unit0` is unavailable for `uop0`, the logic attempts to dynamically route `uop0` to `unit1`. If successful, it simultaneously attempts to route `uop1` to `unit0`.

## Result Writeback Arbitration

Inside each `rvv_backend_falu_unit.sv`, the results from the four sub-units are arbitrated back to the Reorder Buffer (ROB) using a 4-request Round-Robin Arbiter (`arb_round_robin`). The arbitrated results (`falu_result_vld`, `falu_result`) are passed directly back to the ROB.

## `VFSGNJ` Operand Lane Alignment

The `VFSGNJ` instructions (Sign-Injection) are routed to the `fpnew_noncomp` unit. The operand lane routing specifically maps the inputs to correctly align the sign, exponent, and mantissa fields for the underlying IP's execution path:

`op_i[i] = {src3, src1, src2}`

## Interfaces

| Signal           | Direction | Width                         | Description                                               |
| :--------------- | :-------- | :---------------------------- | :-------------------------------------------------------- |
| `clk`            | Input     | 1-bit                         | Global clock signal.                                      |
| `rst_n`          | Input     | 1-bit                         | Global active-low asynchronous reset signal.              |
| `pop`            | Output    | ``NUM_FMA` bits               | Pop signals back to the instruction queue/FMA RS.         |
| `uop_valid`      | Input     | ``NUM_FMA` bits               | Valid bitmask for incoming micro-operations.              |
| `uop`            | Input     | ``NUM_FMA` `FMA_RS_t` packets | Micro-operation payloads.                                 |
| `result_valid`   | Output    | ``NUM_FMA` bits               | Valid bitmask for result submission to ROB.               |
| `result`         | Output    | ``NUM_FMA` `PU2ROB_t` packets | Result payloads.                                          |
| `result_ready`   | Input     | ``NUM_FMA` bits               | Ready status signals from ROB indicating available space. |
| `trap_flush_rvv` | Input     | 1-bit                         | Global flush signal to reset pipeline during traps.       |

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------

> **Provenance & Traceability** - **Verified As Of:** 2026-07-03 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_falu.sv`, `hdl/verilog/rvv/design/rvv_backend_falu_unit.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
