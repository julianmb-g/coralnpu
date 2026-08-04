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
# Vector activation processing

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your risk.

> **Intended Audience:** HW Integrators, SW/Compiler Devs

CoralNPU leverages Vector Core (RVV) execution pipelines instead of a fixed-function hardware Activation Unit.

There is **no dedicated hardware block** for activations. Activation functions map to Vector Arithmetic Logic Unit (VALU) and Vector Multiply-Accumulate (MAC) data paths.

## Architectural function and data paths

Activation workloads execute on the following vector pipeline units:

### 1. linear & clipping activations (relu, relu6)
Clipping functions execute natively in the **Vector ALU**. 
- **Operations:** Vector integer max (`vmax.vx`) and min (`vmin.vx`).
- **Data Path:** `rvv_backend_alu_unit_addsub`
- **Behavior:** Executes in a single cycle per vector group, clipping below 0 (ReLU) or bounding between 0 and 6 (ReLU6).

### 2. saturating integer activations
Provides native saturating arithmetic for quantized models requiring overflow bounding.
- **Operations:** Saturating adds (`vsadd`) and subtracts (`vssub`).
- **Data Path:** `rvv_backend_alu_unit_addsub`
- **Pipeline State:** If saturated, hardware updates the `vxsat` bit in the Vector CSR, pollable by software.

### Edge cases and backpressure

Activations share primary vector pipelines (VALU, MAC) and are subject to standard vector arithmetic structural hazards/backpressure:
- Instruction dispatch stalls if target execution unit input queues are full.
- Long-latency transcendental approximations consume execution slots, potentially bottlenecking heavily pipelined models.

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_alu_unit_addsub.sv`, `hdl/verilog/rvv/design/rvv_backend_sqrt7_rec7.sv`, `hdl/verilog/rvv/design/rvv_backend_mac_unit.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
