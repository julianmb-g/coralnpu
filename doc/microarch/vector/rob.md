# Vector Backend Reorder Buffer (ROB)

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

> **Intended Audience:** Hardware Developers, Compiler Engineers

## Architecture Overview

The Vector Backend Reorder Buffer (ROB) is responsible for maintaining program order for vector instructions executing out-of-order in the vector processor units (PUs). It acts as the central synchronization point, receiving dispatched micro-operations (uops) and collecting their out-of-order results to ensure they retire in-order.

The ROB is parameterized by its depth (`ROB_DEPTH`), which dictates the maximum number of outstanding vector instructions that can be in flight simultaneously.

## Structural Port Configuration

The ROB is heavily multi-ported to support high-throughput dispatch and retirement:

- **Dispatch Ports (Push):** The ROB can accept up to **2 uops** per cycle from the Vector Dispatch unit (`NUM_DP_UOP`).
- **Writeback Ports (Update):** The ROB can receive up to **9 uop results** per cycle from the various execution PUs (`NUM_SMPORT`). (Note: the architectural limits often bound this to 8 active results simultaneously, but the ROB physical interface provides 9 ports).
- **Retirement Ports (Pop):** The ROB can retire up to **4 uops** per cycle to the writeback unit/Vector Register File (VRF) (`NUM_RT_UOP`).

## In-Order Retirement and Forwarding Mechanism

1. **Uop Allocation:** Upon dispatch, uops are allocated entries in the ROB in program order using a multi-ported FIFO structure. The assigned ROB entry ID (`rob_entry_rob2dp`) accompanies the uop through the execution pipelines.
2. **Result Collection:** As execution units complete, they assert their writeback ports (`wr_valid_pu2rob` and `wr_pu2rob`) alongside their assigned `rob_entry`. The ROB updates its internal result memory (`res_mem`), which stores the valid status, write data, saturation flags (`vsaturate`), and floating-point exception flags (`fpexp`).
3. **Operand Forwarding:** The ROB provides a complete bypass network (`uop_rob2dp`) to the Dispatch unit. It exports all active ROB entries, sorted strictly in program order, allowing subsequent instructions to bypass results that have not yet retired.
4. **In-Order Commit:** The ROB continually evaluates the oldest entries at its read pointer (`uop_rptr`). When the oldest instructions mark their `uop_done` status as true, the ROB asserts `rd_valid_rob2rt` to retire up to 4 instructions simultaneously to the architectural state.

## Trap and Flush Semantics

The ROB manages structural flushes in the event of traps or exceptions originating from the Load/Store Unit (LSU) or other sources, ensuring precise exceptions:

- **Delayed Trap Evaluation:** While the ROB receives trap signals via `trap_valid_rmp2rob` and the precise `trap_rob_entry_rmp2rob`, it does NOT flush the pipeline immediately. A trap is only processed when the faulting instruction becomes the oldest non-retired instruction at the head of the ROB.
- **Flush Propagation:** Once the faulting instruction reaches the retirement point, the ROB asserts `trap_flush_rvv`. This signal acts as a global synchronous reset for the internal FIFOs (clearing the `uop_wptr`, `uop_rptr`, and `entry_valid` states) and flushes the vector backend pipelines to ensure no speculative architectural state is committed.

## Interfaces

| Signal                   | Direction | Width                           | Description                                                        |
| :----------------------- | :-------- | :------------------------------ | :----------------------------------------------------------------- |
| `clk`                    | Input     | 1-bit                           | Clock signal.                                                      |
| `rst_n`                  | Input     | 1-bit                           | Active-low reset signal.                                           |
| `uop_valid_dp2rob`       | Input     | `NUM_DP_UOP` bits               | Valid signal for dispatched uops from Dispatch unit.               |
| `uop_dp2rob`             | Input     | `NUM_DP_UOP` `DP2ROB_t` packets | Dispatched uop information.                                        |
| `uop_ready_rob2dp`       | Output    | `NUM_DP_UOP` bits               | Ready signal to Dispatch unit indicating ROB can accept uops.      |
| `rob_empty`              | Output    | 1-bit                           | Indicates ROB is empty.                                            |
| `rob_entry_rob2dp`       | Output    | `ROB_DEPTH_WIDTH` bits          | Allocated ROB entry ID for the uop.                                |
| `wr_valid_pu2rob`        | Input     | `NUM_SMPORT` bits               | Valid signal for uop results from PU.                              |
| `wr_pu2rob`              | Input     | `NUM_SMPORT` `PU2ROB_t` packets | PU uop result data.                                                |
| `rd_valid_rob2rt`        | Output    | `NUM_RT_UOP` bits               | Valid signal for retiring uops.                                    |
| `rd_rob2rt`              | Output    | `NUM_RT_UOP` `ROB2RT_t` packets | Retiring uop data (to VRF).                                        |
| `rd_ready_rt2rob`        | Input     | `NUM_RT_UOP` bits               | Ready signal from Retire unit.                                     |
| `rob_entry_rob2rt`       | Output    | `ROB_DEPTH_WIDTH` bits          | Retiring ROB entry ID.                                             |
| `uop_rob2dp`             | Output    | `ROB_DEPTH` `ROB2DP_t` packets  | Bypass all ROB entries to Dispatch unit (sorted in program order). |
| `trap_valid_rmp2rob`     | Input     | 1-bit                           | Trap valid signal.                                                 |
| `trap_rob_entry_rmp2rob` | Input     | `ROB_DEPTH_WIDTH` bits          | Faulting instruction's ROB entry ID.                               |
| `trap_ready_rob2rmp`     | Output    | 1-bit                           | Ready signal for trap handshake.                                   |
| `trap_ready_rvv2rvs`     | Output    | 1-bit                           | Ready signal to RVS.                                               |
| `trap_flush_rvv`         | Output    | 1-bit                           | Global flush signal for Vector Backend.                            |

<!-- mdformat off -->
<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-03 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_rob.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- prettier-ignore-end -->
<!-- mdformat on -->

> **Traceability:** Generated by Gemini. Derived from upstream commit 9a1e82634c2b0f3d42310f89cd1484d8f3302ec9.
