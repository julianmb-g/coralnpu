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

# Vector MAC engine

> **Intended Audience:** HW Devs, SW/Compiler Devs

## Overview

The CoralNPU features a standard vector Multiply-Accumulate (MAC) engine within its RISC-V Vector (RVV) pipeline. Implemented via the `rvv_backend_mac_unit`, this unit handles vector multiply, multiply-add, and fixed-point saturating arithmetic operations.

## Architecture & capabilities

The MAC engine is responsible for executing vector multiply (`vmul`), multiply-add (`vmacc`, `vmadd`), and saturating multiply (`vsmul`) operations.

### Execution width & throughput

- **Datapath**: The execution unit is fully `VLEN` wide (VLEN = 128).
- **Throughput**: The pipeline can sustain up to **256 MAC operations per cycle** for 8-bit elements.

### Supported precisions (EEW)

The MAC unit internally partitions the `VLEN` datapath to support multiple element widths natively via dynamic configuration.

- **EEW Configuration**: The element width for each operation is configured on a per-instruction basis via the `vs2_eew` field in the `rs2mac_uop_data` packet received from the Reservation Station (`hdl/verilog/rvv/design/rvv_backend_mac_unit.sv:L153`). This configuration is derived from the active vector configuration (`vtype`) set by software via `vset{i}vli` instructions, which is processed by the vector frontend/decoder and passed down the pipeline.
- **EEW=8 (8-bit)**: Maximum throughput for quantized int8 models.
- **EEW=16 (16-bit)**: Medium throughput for int16 models.
- **EEW=32 (32-bit)**: High-precision accumulations.

### Rounding & saturation semantics

For quantized neural networks, exact rounding and clipping behavior is critical.

- **Saturating Multiplies (`vsmul`)**: The MAC unit provides native hardware support for `vsmul`. If the operation overflows the target precision, the hardware automatically saturates the result to the maximum/minimum representable value and sets the `vxsat` bit in the vector CSR to flag the overflow.
- **Rounding Modes (`vxrm`)**: Hardware rounding increments (`vsmul_round_incr`) are dynamically injected into the datapath based on the active Fixed-Point Rounding Mode (`vxrm` CSR), ensuring bit-exact compliance with the RISC-V Vector specification and compiler expectations.

## Interfaces

| Signal              | Direction | Width             | Description                                                         |
| :------------------ | :-------- | :---------------- | :------------------------------------------------------------------ |
| `clk`               | Input     | 1 bit             | Global clock signal.                                                |
| `rst_n`             | Input     | 1 bit             | Global active-low reset signal.                                     |
| `rs2mac_uop_valid`  | Input     | 1 bit             | Valid signal indicating a new micro-operation from the RS.          |
| `rs2mac_uop_data`   | Input     | `MUL_RS_t`        | Data payload of the micro-operation.                                |
| `mac_pipe_vld_en`   | Input     | 1 bit             | Pipeline valid enable signal.                                       |
| `mac_pipe_data_en`  | Input     | 1 bit             | Pipeline data enable signal.                                        |
| `trap_flush_rvv`    | Input     | 1 bit             | Global trap/flush signal.                                           |
| `mac2rob_uop_valid` | Output    | 1 bit             | Valid signal for the result submitted to the ROB.                   |
| `mac2rob_uop_data`  | Output    | `PU2ROB_t`        | Output data payload submitted to the ROB.                           |

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_mac_unit.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
