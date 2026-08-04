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

# Vector backend ROB writeback arbiter

> **Intended Audience:** HW Devs

The Vector Backend Reorder Buffer (ROB) Writeback Arbiter (`rvv_backend_arb`) routes execution results from the Processing Units (PUs) to the ROB write ports. Because the number of PUs (8 or 10) exceeds the available ROB write ports (4), the arbiter employs a cascaded hybrid static-priority and round-robin arbitration scheme to prevent starvation while prioritizing high-throughput units.

## Arbitration architecture

The arbitration logic uses a two-tiered approach:

1. **Static Priority**: Dedicated ROB ports are assigned to specific high-priority PUs. If a high-priority PU has a valid request (`req`), it is immediately granted the port.

2. **Round-Robin Arbitration**: If a statically mapped PU does not assert a request, the unused ROB port is dynamically allocated to a pool of lower-priority PUs using a round-robin scheme (`arb_round_robin`).

The `arb_round_robin` module implements a fast, combinational priority resolution using a stateful priority register (`prio`). The grant logic (`grant_tmp = {req,req} & ~({req,req} - prio)`) ensures fair distribution among competing lower-priority units.

## Port mapping and priorities

The arbiter supports two configurations based on the `ZVE32F_ON` parameter, mapping 8 or 10 PUs to 4 ROB ports (`NUM_SMPORT = 4`).

### 10 PU configuration (`ZVE32F_ON` enabled)

| Name | Direction | Type/Width | Description |
| :--- | :--- | :--- | :--- |
| `clk` | Input | `logic` | Clock signal |
| `rst_n` | Input | `logic` | Active-low reset signal |
| `req` | Input | ``logic [`NUM_PU-1:0]`` | Request signals from Processing Units |
| `item` | Input | ``PU2ROB_t [`NUM_PU-1:0]`` | Result data items from Processing Units |
| `grant` | Output | ``logic [`NUM_PU-1:0]`` | Arbitration grant signals to Processing Units |
| `result_valid` | Output | ``logic [`NUM_SMPORT-1:0]`` | Valid signals for arbitrated results to ROB |
| `result` | Output | ``PU2ROB_t [`NUM_SMPORT-1:0]`` | Arbitrated result data to ROB |

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_arb.sv`, `hdl/verilog/rvv/common/arb_round_robin.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
