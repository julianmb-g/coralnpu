# Vector MAC Engine (Tensor Processing)

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

> **Intended Audience:** Hardware Developers, SW/Compiler Devs

## Overview

The CoralNPU handles tensor processing and matrix multiplication workloads natively within the standard RISC-V Vector (RVV) pipeline. There is no dedicated, standalone "Tensor Processing Engine" (TPE). Instead, all vector multiply-accumulate operations are executed by the highly parallel `rvv_backend_mac_unit`.

## Architecture & Capabilities

The MAC Engine is responsible for executing vector multiply (`vmul`), multiply-add (`vmacc`, `vmadd`), and saturating multiply (`vsmul`) operations.

### Execution Width & Throughput

- **Datapath**: The execution unit is fully `VLEN` wide.
- **Throughput**: The pipeline can sustain up to **256 MAC operations per cycle** for 8-bit elements.

### Supported Precisions (EEW)

The MAC unit internally partitions the `VLEN` datapath to support multiple element widths natively via dynamic configuration.

- **EEW Configuration**: The element width for each operation is configured on a per-instruction basis via the `vs2_eew` field in the `rs2mac_uop_data` packet received from the Reservation Station (`hdl/verilog/rvv/design/rvv_backend_mac_unit.sv:L153`). This configuration is derived from the active vector configuration (`vtype`) set by software via `vset{i}vli` instructions, which is processed by the vector frontend/decoder and passed down the pipeline.
- **EEW=8 (8-bit)**: Maximum throughput for quantized int8 models.
- **EEW=16 (16-bit)**: Medium throughput for int16 models.
- **EEW=32 (32-bit)**: High-precision accumulations.

### Rounding & Saturation Semantics

For quantized neural networks, exact rounding and clipping behavior is critical.

- **Saturating Multiplies (`vsmul`)**: The MAC unit provides native hardware support for `vsmul`. If the operation overflows the target precision, the hardware automatically saturates the result to the maximum/minimum representable value and sets the `vxsat` bit in the vector CSR to flag the overflow.
- **Rounding Modes (`vxrm`)**: Hardware rounding increments (`vsmul_round_incr`) are dynamically injected into the datapath based on the active Fixed-Point Rounding Mode (`vxrm` CSR), ensuring bit-exact compliance with the RISC-V Vector specification and compiler expectations.

## Interfaces

| Signal              | Direction | Width             | Description                                                         |
| :------------------ | :-------- | :---------------- | :------------------------------------------------------------------ |
| `clk`               | Input     | 1-bit             | Clock signal.                                                       |
| `rst_n`             | Input     | 1-bit             | Active-low reset signal.                                            |
| `rs2mac_uop_valid`  | Input     | 1-bit             | Valid signal for the instruction from Reservation Station.          |
| `rs2mac_uop_data`   | Input     | `MUL_RS_t` packet | Instruction data from Reservation Station (operands, opcode, etc.). |
| `mac_pipe_vld_en`   | Input     | 1-bit             | Pipeline valid enable.                                              |
| `mac_pipe_data_en`  | Input     | 1-bit             | Pipeline data enable.                                               |
| `trap_flush_rvv`    | Input     | 1-bit             | Trap flush signal to clear pipeline.                                |
| `mac2rob_uop_valid` | Output    | 1-bit             | Valid signal for the result sent to Reorder Buffer (ROB).           |
| `mac2rob_uop_data`  | Output    | `PU2ROB_t` packet | Result data sent to ROB (write data, saturation flags, etc.).       |

<!-- mdformat off -->
<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

<!-- prettier-ignore-end -->

> **Provenance & Traceability** - **Verified As Of:** 2026-07-03 - **Upstream Commit:** [f5f6c88d3dff8cb198cd89420919b6863667f3e0](https://github.com/google/coralnpu/commit/f5f6c88d3dff8cb198cd89420919b6863667f3e0) - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_mac_unit.sv:L1` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
<!-- mdformat on -->
