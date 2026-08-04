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

# Vector dispatch operand extraction and byte type generation

> **Intended Audience:** HW Devs

The Vector Dispatch unit contains dedicated sub-modules responsible for routing Vector Register File (VRF) read data to the appropriate execution unit pipelines and generating per-element byte types. These mechanisms handle the dynamic effective execution width (EEW) scaling, vector masking, and structural routing limitations of the VRF.

## Vrf routing limitations and operand extraction

The `rvv_backend_dispatch_operand` module maps the 6 read ports of the VRF (`rd_data_vrf2dp[5:0]`) to up to 3 micro-ops per cycle. The extraction logic heavily relies on the `uop_class` to correctly route data based on whether an instruction expects a vector, scalar, or immediate in each operand slot.

### Interface

| Signal           | Direction | Width               | Description                                                             |
| :--------------- | :-------- | :------------------ | :---------------------------------------------------------------------- |
| `vrf_byp`        | Output    | `NUM_DP_UOP`        | Bypassed UOP operands to the next stage.                                |
| `uop_uop2dp`     | Input     | `NUM_DP_UOP`        | UOP queue data input to the dispatch unit.                              |
| `rd_data_vrf2dp` | Input     | `NUM_DP_VRF * VLEN` | Read data from the Vector Register File ports.                          |
| `v0_mask_vrf2dp` | Input     | `VLEN` (128 bits)   | The v0 mask register read data for masking operations.                  |

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_dispatch_operand.sv`, `hdl/verilog/rvv/design/rvv_backend_dispatch_opr_byte_type.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
