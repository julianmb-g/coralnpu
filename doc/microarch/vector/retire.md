# Vector Retirement Unit

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

The Vector Retirement Unit (`rvv_backend_retire`) serves as the final stage of the vector execution pipeline. It acts as the interface between the Reorder Buffer (ROB) and the architectural state, orchestrating the commitment of up to `NUM_RT_UOP` micro-ops per cycle.

## Overview

The retirement unit is responsible for:

1. Routing execution results to the appropriate register files (VRF, XRF, FRF).
2. Resolving intra-cycle Write-After-Write (WAW) hazards.
3. Updating architectural status registers (`vxsat`, `fcsr`).
4. Handling trap flushes and updating the Vector Control and Status Register (`vcsr`).

## Data Routing and Register Updates

Micro-ops presented by the ROB (`rob2rt_write_data`) include a target destination type (`w_type`). The retirement unit demultiplexes these requests to the corresponding physical register file boundaries:

| Destination (`w_type`) | Target Register File | Interface Signals | Description                                                                                                                     |
| :--------------------- | :------------------- | :---------------- | :------------------------------------------------------------------------------------------------------------------------------ |
| `VRF`                  | Vector Register File | `rt2vrf_write_*`  | Writes up to `VLEN` bits of vector data. Modifies byte-level elements based on the `rt_strobe` mask (`vd_type == BODY_ACTIVE`). |
| `XRF`                  | Scalar Register File | `rt2xrf_write_*`  | Extracts the lower 32 bits (`w_data[31:0]`) for scalar destinations (e.g., reductions, `vmv.x.s`).                              |
| `FRF`                  | Float Register File  | `rt2frf_write_*`  | If `ZVE32F_ON` is defined, routes floating-point scalar results to the FRF.                                                     |

## Write-After-Write (WAW) Hazard Resolution

The vector pipeline allows multiple micro-ops to retire in the same cycle. If two or more micro-ops target the same architectural vector register (`w_index`), a Write-After-Write (WAW) hazard occurs.

The `rvv_backend_retire_waw` sub-module resolves this by evaluating dependencies across the active retirement window (`UOP_NUM`).

- It compares the destination index of each micro-op against all younger micro-ops in the same cycle (`w_index[i] == w_index[UOP_NUM-1]`).
- If a collision (`vd_hit`) is detected, the older micro-op's write valid signal (`hit_waw`) is masked out.
- The latest micro-op writes its data and byte strobes (`res` and `res_strobe`), effectively overriding the older operations without stalling the pipeline.

## Status Flags and Exceptions

During retirement, execution side-effects are aggregated and written to the architectural CSRs:

- **Saturation (`vxsat`)**: The `vxsaturate` bits from each active byte lane (`w_strobe`) are logically OR'd together. If any saturation occurred, the `vxsat` register is updated.
- **Floating-Point Exceptions (`fcsr`)**: If `ZVE32F_ON` is defined, the five floating-point exception flags (Invalid `nv`, Divide-by-Zero `dz`, Overflow `of`, Underflow `uf`, Inexact `nx`) are aggregated across all active lanes and written to the `fcsr`.

## Trap Handling and Pipeline Flush

If the oldest retiring micro-op indicates a trap (`trap_flag[0]`), the retirement process is immediately halted for all subsequent micro-ops.

- `rt2rob_write_ready` is gated for younger micro-ops.
- The `vcsr` is updated with the trap state (`rt2vcsr_write_data`).
- No architectural state (VRF, XRF, FRF, `vxsat`, `fcsr`) is modified by the trapping micro-op or any younger instructions in the window.

<!-- mdformat off -->
<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-22 - **Upstream Commit:** 5c2647afd951f70d6244ea06b5a8b7fa1fdf2918 - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_retire.sv`, `hdl/verilog/rvv/design/rvv_backend_retire_waw.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- prettier-ignore-end -->
<!-- mdformat on -->
