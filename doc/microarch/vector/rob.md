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

> **Intended Audience:** HW Devs, SW/Compiler Devs

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

| Signal | Direction | Width | Description |
| :--- | :---: | :---: | :--- |
| `clk` | Input | 1 | Clock signal. |
| `rst_n` | Input | 1 | Active-low asynchronous reset. |
| `uop_valid_dp2rob` | Input | `NUM_DP_UOP` | Dispatch validity signals for new micro-operations. |
| `uop_dp2rob` | Input | Variable | Micro-operation dispatch data structures from Dispatch unit. |
| `uop_ready_rob2dp` | Output | `NUM_DP_UOP` | Ready signals back to Dispatch unit. |
| `wr_valid_pu2rob` | Input | `NUM_SMPORT` | Writeback validity signals from processing units (PUs). |
| `wr_pu2rob` | Input | Variable | Writeback uop result data from PUs. |
| `rd_valid_rob2rt` | Output | `NUM_RT_UOP` | Retirement validity signals to Retirement unit. |
| `rd_ready_rt2rob` | Input | `NUM_RT_UOP` | Ready signals from Retirement unit. |
| `uop_rob2dp` | Output | `ROB_DEPTH` | Program-ordered bypass data forwarded to Dispatch unit. |
| `trap_flush_rvv` | Output | 1 | Global pipeline flush signal asserted upon precise trap commit. |

## Floating-Point Exception Discard Constraints

The ROB collects floating-point exception flags (`fpexp`) on completing instructions from writeback ports (`wr_pu2rob`) and maintains them inside its internal result memory (`res_mem`). At retirement, these flags are output via `rob2rt_write_data` to the `rvv_backend_retire` module.

However, as a critical architectural design constraint:
- These exception flags are **discarded** at the `RvvCore` boundaries.
- They do **NOT** write to the architectural `fcsr`.
- They do **NOT** propagate across the Chisel `RvvCoreIO` boundary interface.

As a result, vector execution exceptions (such as division by zero or overflow generated in the VFPU) do not register in any architectural status registers, and software cannot inspect vector-initiated floating-point exceptions.

--------------------------------------------------------------------------------

**Provenance & Traceability**
- **Verified As Of:** 2026-07-25
- **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7)
- **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_rob.sv`
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

> **Traceability:** Generated by Gemini. Derived from upstream commit 6a8cc54a67fb4ca7ecda116453fbdc4a97994ebf.
