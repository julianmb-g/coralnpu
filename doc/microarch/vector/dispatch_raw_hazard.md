# Vector Dispatch RAW Hazard Tracking

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

> **Intended Audience:** HW Devs


The Vector Dispatch unit implements Read-After-Write (RAW) hazard detection to maintain data coherency for vector instructions in the CoralNPU. This is implemented physically within `rvv_backend_dispatch_raw_uop_rob.sv`.

## RAW Hazard Conditions

The hazard detection logic evaluates an incoming successor micro-op against all predecessor micro-ops currently residing in the Reorder Buffer (ROB). A RAW hazard is explicitly detected for a given vector operand (`vs1`, `vs2`, `vd` acting as `vs3`, or `v0` mask) when the following three conditions are met simultaneously:

1.  **Index Match**: The source index of the successor micro-op matches the write destination index (`w_index`) of a predecessor micro-op.
2.  **Successor Validity**: The successor micro-op requires the vector register (indicated by `vs1_valid`, `vs2_valid`, `vs3_valid`, or `~vm` for unmasked `v0` operations).
3.  **Predecessor Status**: The predecessor micro-op is valid and explicitly targets the Vector Register File (`w_type == VRF`).

## Dispatch Stalling (Wait Generation)

If a RAW hazard is detected, the dispatch stage will stall the successor micro-op by generating a wait signal (e.g., `vs1_wait`). This wait signal is exclusively asserted when the predecessor micro-op has not yet produced valid write data (`~w_valid`). The incoming micro-op remains stalled in dispatch until all data dependencies within the ROB are resolved.

## Interface Mapping

| Interface     | Type   | Description                                                                              |
| ------------- | ------ |

--------------------------------------------------------------------------------

**Provenance & Traceability**
- **Verified As Of:** 2026-07-25
- **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7)
- **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_dispatch_raw_uop_rob.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
