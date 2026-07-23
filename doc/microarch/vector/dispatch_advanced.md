# Vector Dispatch Bypass and Hazard Logic

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


The Vector Dispatch bypass mechanism explicitly routes vector data from pending instructions within the Reorder Buffer (ROB) directly to the execution pipelines, efficiently bypassing the Vector Register File (VRF) to resolve data hazards. This is physically implemented in `rvv_backend_dispatch_bypass.sv`.

## Bypass Selection Matrix

Data forwarding is structured as a two-dimensional selection matrix `[ROB_DEPTH][VLENB]`, allowing precise byte-level resolution. For each byte lane, the bypass is active if:

1. A Read-After-Write (RAW) hazard hit is detected for the operand (`raw_uop_rob.*_hit`).
2. The corresponding byte lane in the pending micro-op (`rob_byp`) is actively written.

A byte is considered actively written if its `byte_type` is:

- `BODY_ACTIVE`
- `BODY_INACTIVE` (and the instruction's mask policy dictates 1-injection via `inactive_one`)
- `TAIL` (and the instruction's tail policy dictates 1-injection via `tail_one`)

## Agnostic 1-Injection Rules

RISC-V Vector specification policies for inactive and tail elements are strictly handled during the bypass multiplexing.

If a byte lane matches the agnostic 1-injection condition (`agnostic[i][j]`), the bypass logic overrides the data from the ROB and directly injects `8'hFF`. This applies to `vs1`, `vs2`, `vd`, and `v0` operands seamlessly before execution.

## Interface Mapping

| Interface     | Type   | Description                                                                          |
| ------------- | ------ | ------------------------------------------------------------------------------------ |
| `uop_operand` | Output | The final operand data routed to the execution units (`UOP_OPN_t`).                  |
| `rob_byp`     | Input  | Array of bypass data and byte lane status from the ROB (`ROB_BYP_t`).                |
| `vrf_byp`     | Input  | Default operand data read directly from the VRF (`UOP_OPN_t`).                       |
| `raw_uop_rob` | Input  | Hazard hit signals identifying ROB entries with data dependencies (`RAW_UOP_ROB_t`). |

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-22 - **Upstream Commit:** 5c2647afd951f70d6244ea06b5a8b7fa1fdf2918 - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_dispatch_bypass.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
