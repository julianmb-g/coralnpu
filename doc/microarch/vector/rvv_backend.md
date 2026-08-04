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

# Vector backend pipeline

> **Intended Audience:** HW Devs

The `rvv_backend` module is the top-level structural wrapper for the vector execution backend of the CoralNPU. It coordinates the entire lifecycle of vector instructions—from command queueing and decoding, to dispatch, execution, and in-order retirement.

## Top-level structural mapping

The `rvv_backend` instantiates and wires together the following major subsystems:

1. **Frontend & Decoding (`DE1`, `DE2`)**:

   - Instructions arrive via the `insts_rvs2cq` interface and are buffered in the Command Queue (`CQ`).

   - The `rvv_backend_decode` (DE1) and `rvv_backend_decode_de2` (DE2) stages expand incoming commands into micro-operations (`UOP_QUEUE_t`).
   - Micro-operations are queued in the UOP Queue (`UQ`) before dispatch.

2. **Dispatch Unit (`DP`)**:

   - `rvv_backend_dispatch` routes decoded micro-operations to the appropriate reservation stations based on structural and RAW hazard checks.

   - It also queries the Vector Register File (`VRF`) for operand availability and allocates entries in the Re-Order Buffer (`ROB`).

3. **Execution Units (`PU`)**:

   - The backend supports multiple specialized execution pipelines, each decoupled via its own reservation station:

     - **ALU**: Integer arithmetic and bitwise operations (`rvv_backend_alu`).
     - **PMTRDT**: Permutation and reduction operations (`rvv_backend_pmtrdt`).

     - **MULMAC**: Integer multiplication and multiply-accumulate (`rvv_backend_mulmac`).
     - **DIV**: Integer and floating-point division (`rvv_backend_div`).

     - **FALU** (Conditional `ZVE32F_ON`): Floating-point ALU array (`rvv_backend_falu`).

4. **Memory Interface (`LSU`)**:

   - Load/Store operations are routed to the `rvv_backend_lsu_remap` unit, which manages tracking and trap generation.

   - The backend interfaces externally with the core LSU via the `uop_lsu` handshakes.

5. **Arbitration & Re-Order Buffer (`ARB`, `ROB`)**:

   - Results from the execution units are arbitrated (`rvv_backend_arb`) and written into the `rvv_backend_rob`.

   - The ROB ensures precise exceptions and in-order retirement of vector instructions.

6. **Retirement & Register Files (`RT`, `VRF`)**:

   - `rvv_backend_retire` processes graduating instructions from the ROB.
   - It updates the scalar core's Architectural Register Files (`XRF`, `FRF`) and CSRs (`vxsat`, `fcsr`, `vcsr`).

   - `rvv_backend_vrf` serves as the vector register file, handling operand reads from the dispatch stage and writebacks from the retirement stage.

## Reservation stations

Each execution unit is preceded by a dedicated Reservation Station (RS) implemented as a `multi_fifo` with a configurable depth (e.g., `ALU_RS_DEPTH`, `PMTRDT_RS_DEPTH`). The `CHAOS_PUSH` parameter is enabled on these FIFOs, and they provide `almost_full` and `almost_empty` signals to backpressure the dispatch unit and prevent overflow.

## Interface routing contracts

- **Scalar Core Input**: Instructions enter via `insts_valid_rvs2cq` and `insts_rvs2cq` from the scalar core's RVS unit.

- **Scalar Core Output (Retirement)**: Retirement state is routed back to the scalar core via:
  - `rt_xrf_valid_rvv2rvs` for integer register updates.

  - `async_frd_valid` for floating-point register updates (`ZVE32F_ON`).
  - `wr_vxsat_valid` and `rt2fcsr_write_valid` for CSR state propagation.

- **Trap Handling**: Exception signals (`trap_valid_rvs2rvv`) trigger an immediate pipeline flush (`trap_flush_rvv`), clearing all internal queues and reservation stations.

## Interfaces

| Signal | Direction | Width | Description |
| :--- | :--- | :--- | :--- |
| `clk` | Input | 1 | Clock signal |
| `rst_n` | Input | 1 | Active-low reset |
| `insts_valid_rvs2cq` | Input | `` `ISSUE_LANE `` | Valid signals for instructions from scalar core |
| `insts_rvs2cq` | Input | `` RVVCmd [`ISSUE_LANE-1:0] `` | Vector instructions from scalar core |
| `insts_ready_cq2rvs` | Output | `` `ISSUE_LANE `` | Ready signals to scalar core |
| `remaining_count_cq2rvs` | Output | `` $clog2(`CQ_DEPTH)+1 `` | Remaining entries in command queue |
| `uop_lsu_valid_rvv2lsu` | Output | `` `NUM_LSU `` | Valid signals for LSU uops |
| `uop_lsu_rvv2lsu` | Output | `` UOP_RVV2LSU_t [`NUM_LSU-1:0] `` | LSU uops to memory system |
| `uop_lsu_ready_lsu2rvv` | Input | `` `NUM_LSU `` | Ready signals from LSU |
| `uop_lsu_valid_lsu2rvv` | Input | `` `NUM_LSU `` | Valid signals for returning LSU uops |
| `uop_lsu_lsu2rvv` | Input | `` UOP_LSU2RVV_t [`NUM_LSU-1:0] `` | Returning LSU uops from memory |
| `uop_lsu_ready_rvv2lsu` | Output | `` `NUM_LSU `` | Ready signals for returning LSU uops |
| `rt_xrf_valid_rvv2rvs` | Output | `` `NUM_RT_UOP `` | Valid signals for scalar register writeback |
| `rt_rvs_rvv2rvs` | Output | `` RT2RVS_t [`NUM_RT_UOP-1:0] `` | Scalar register writeback data |
| `rt_rvs_ready_rvs2rvv` | Input | `` `NUM_RT_UOP `` | Ready signals for scalar register writeback |
| `trap_valid_rvs2rvv` | Input | 1 | Trap flush signal from scalar core |
| `trap_ready_rvv2rvs` | Output | 1 | Trap flush ready signal |
| `rvv_idle` | Output | 1 | Vector backend idle indicator |

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.


> **Traceability:** Generated by Gemini. Derived from upstream commit d9622642c63f7eba6e0c9baa7fea2188d32e28e3.