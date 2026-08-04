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

# Vector LSU remap unit

> **Intended Audience:** HW Devs

The Vector LSU Remap Unit (`rvv_backend_lsu_remap`) acts as the synchronization layer between the out-of-order execution backend and the Load/Store Unit (LSU). It matches asynchronous LSU responses with their original in-flight micro-op metadata.

## Response tracking and mapping

The module accepts inputs from `NUM_LSU` parallel ports, taking both in-flight metadata (`LSU_MAP_INFO_t` via `mapinfo`) and physical execution results (`UOP_LSU_t` via `lsu_res`).

- **ROB Entry Mapping:** The `rob_entry` associated with the completed operation is extracted directly from the `mapinfo` and forwarded to the Reorder Buffer (ROB) along with the write data (`w_data`).

- **Completion Validation:** A result is considered valid (`result_valid`) and ready to be routed to the ROB if:
  - For `IS_LOAD` operations: The LSU signals `vregfile_write_valid`.

  - For `IS_STORE` operations: The LSU signals `lsu_vstore_last`.

## Trap generation logic

If the LSU encounters an exception during memory access (e.g., an unaligned access or memory fault), it asserts `trap_valid` within the `lsu_res` payload.

- The remap unit intercepts this signal and prioritizes it over normal completion.

- It asserts `trap_valid_rmp2rob` and forwards the specific `rob_entry` (`trap_rob_entry_rmp2rob`) directly to the ROB.

- This allows the ROB to flag the exception precisely at the faulting instruction.

## Dequeue (pop) semantics

Entries are popped (`pop_mapinfo`, `pop_lsu_res`) from the tracking queues only when the ROB acknowledges the result (`result_ready`) or when a trap is successfully handed off (`trap_ready_rob2rmp`), ensuring no state is dropped during backpressure.

## Interfaces

| Signal                   | Direction | Type                           | Description                                 |
| :----------------------- | :-------- | :----------------------------- | :------------------------------------------ |
| `mapinfo_valid`          | Input     | `logic [NUM_LSU-1:0]`          | Indicates valid map info metadata.          |
| `mapinfo`                | Input     | `LSU_MAP_INFO_t [NUM_LSU-1:0]` | The in-flight metadata for the load/store.  |
| `pop_mapinfo`            | Output    | `logic [NUM_LSU-1:0]`          | Dequeue signal for the map info tracking queue. |
| `lsu_res_valid`          | Input     | `logic [NUM_LSU-1:0]`          | Indicates a valid result returned from LSU. |
| `lsu_res`                | Input     | `UOP_LSU_t [NUM_LSU-1:0]`      | The physical execution result from the LSU. |
| `pop_lsu_res`            | Output    | `logic [NUM_LSU-1:0]`          | Dequeue signal for the LSU result queue.    |
| `result_valid`           | Output    | `logic [NUM_LSU-1:0]`          | Valid signal for the mapped ROB result.     |
| `result`                 | Output    | `PU2ROB_t [NUM_LSU-1:0]`       | Result payload destined for the ROB.        |
| `result_ready`           | Input     | `logic [NUM_LSU-1:0]`          | Acknowledgment from the ROB.                |
| `trap_valid_rmp2rob`     | Output    | `logic`                        | Asserts if an exception/fault occurred.     |
| `trap_rob_entry_rmp2rob` | Output    | `logic [ROB_DEPTH_WIDTH-1:0]`  | Faulting instruction's ROB entry index.     |
| `trap_ready_rob2rmp`     | Input     | `logic`                        | Acknowledgment from ROB for the trap.       |

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_lsu_remap.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
