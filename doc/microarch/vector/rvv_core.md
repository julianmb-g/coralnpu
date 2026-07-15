# Vector Core Pipeline (RvvCore)

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


The `RvvCore` module serves as the primary top-level integration wrapper for the CoralNPU vector execution pipeline. It bridges the instruction dispatch and scalar register file interfaces from the host scalar core (`SCore`) to the dedicated vector backend.

## System Architecture and Implementation

`RvvCore` manages the decoupling between scalar frontend dispatch and vector execution. It acts as the boundary where decoupled vector instructions, configuration state (vtype, vl), and scalar operands are consumed. It instantiates the `RvvFrontEnd` to manage configuration and structural hazard tracking, subsequently forwarding decoupled vector micro-operations (uops) to the vector backend.

## Hardware/Software Interfaces

### Dispatch and Configuration

| Signal Bundle                             | Direction    | Description                                                                                    |
| ----------------------------------------- | ------------ | ---------------------------------------------------------------------------------------------- |
| `inst_valid` / `inst_data` / `inst_ready` | Input/Output | Decoupled valid/ready handshake for incoming vector instructions from the scalar decode stage. |
| `vstart`, `vxrm`, `vxsat`, `frm`          | Input        | Immediate architectural CSR states determining execution configuration and rounding modes.     |
| `config_state_valid` / `config_state`     | Output       | Pushes updated vector configuration state back to the scalar core for synchronization.         |

### Operand Delivery

| Signal Bundle                      | Direction | Description                                                                                                                        |
| ---------------------------------- | --------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| `reg_read_valid` / `reg_read_data` | Input     | Values sourced from the scalar integer register file required for vector address generation, strides, or scalar-vector operations. |
| `freg_read_data`                   | Input     | Values sourced from the scalar floating-point register file for mixed-precision vector operations.                                 |

### Data Paths & Memory (LSU)

| Signal Bundle       | Direction | Description                                                                               |
| ------------------- | --------- | ----------------------------------------------------------------------------------------- |
| `uop_lsu_*_rvv2lsu` | Output    | Address, data, and mask generation delivered from the vector core to the Load/Store Unit. |
| `uop_lsu_*_lsu2rvv` | Input     | Load responses and acknowledgment signals returned from the LSU to the vector backend.    |

### State Writeback and Exception Handling

| Signal Bundle                     | Direction | Description                                                                                                                           |
| --------------------------------- | --------- | ------------------------------------------------------------------------------------------------------------------------------------- |
| `async_rd_*` / `async_frd_*`      | Output    | Asynchronous writeback paths delivering scalar or floating point results back to the scalar core (e.g., reductions, scalar extracts). |
| `trap_valid_o` / `trap_data_o`    | Output    | Exception signals to initiate pipeline flushes and trap vector exceptions in the scalar core.                                         |
| `wr_vxsat_valid_o` / `wr_vxsat_o` | Output    | Sticky saturation flag updates triggered by vector instructions.                                                                      |

<!-- mdformat off -->

<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

<!-- prettier-ignore-end -->

**Provenance & Traceability** - **Verified As Of:** 2026-07-05 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/verilog/rvv/design/RvvCore.sv`, `hdl/chisel/src/coralnpu/rvv/RvvCore.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->

> **Traceability:** Generated by Gemini. Derived from upstream commit 9a1e82634c2b0f3d42310f89cd1484d8f3302ec9.
