# Vector Backend Pipeline

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


The `rvv_backend` module is the top-level structural wrapper for the vector execution backend of the CoralNPU. It coordinates the entire lifecycle of vector instructions—from command queueing and decoding, to dispatch, execution, and in-order retirement.

## Top-Level Structural Mapping

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

## Reservation Stations

Each execution unit is preceded by a dedicated Reservation Station (RS) implemented as a `multi_fifo` with a configurable depth (e.g., `ALU_RS_DEPTH`, `PMTRDT_RS_DEPTH`). The `CHAOS_PUSH` parameter is enabled on these FIFOs, and they provide `almost_full` and `almost_empty` signals to backpressure the dispatch unit and prevent overflow.

## Interface Routing Contracts

- **Scalar Core Input**: Instructions enter via `insts_valid_rvs2cq` and `insts_rvs2cq` from the scalar core's RVS unit.
- **Scalar Core Output (Retirement)**: Retirement state is routed back to the scalar core via:
  - `rt_xrf_valid_rvv2rvs` for integer register updates.
  - `async_frd_valid` for floating-point register updates (`ZVE32F_ON`).
  - `wr_vxsat_valid` and `rt2fcsr_write_valid` for CSR state propagation.
- **Trap Handling**: Exception signals (`trap_valid_rvs2rvv`) trigger an immediate pipeline flush (`trap_flush_rvv`), clearing all internal queues and reservation stations.

## Interfaces

| Signal                   | Direction | Width                                      | Description                                               |
| :----------------------- | :-------- | :----------------------------------------- | :-------------------------------------------------------- |
| `clk`                    | Input     | 1-bit                                      | Global clock signal.                                      |
| `rst_n`                  | Input     | 1-bit                                      | Global active-low asynchronous reset signal.              |
| `insts_valid_rvs2cq`     | Input     | `ISSUE_LANE` bits                          | Valid bitmask for incoming vector instructions from RVS.  |
| `insts_rvs2cq`           | Input     | `ISSUE_LANE` `RVVCmd` packets              | Incoming vector instruction payloads from RVS.            |
| `insts_ready_cq2rvs`     | Output    | `ISSUE_LANE` bits                          | Ready signals back to RVS indicating command queue space. |
| `remaining_count_cq2rvs` | Output    | `$clog2(CQ_DEPTH)+1` bits                  | Remaining entries in Command Queue available for RVS.     |
| `uop_lsu_valid_rvv2lsu`  | Output    | `NUM_LSU` bits                             | Valid bitmask for LSU uops sent to LSU.                   |
| `uop_lsu_rvv2lsu`        | Output    | `NUM_LSU` `UOP_RVV2LSU_t` packets          | LSU uop payloads sent to LSU.                             |
| `uop_lsu_ready_lsu2rvv`  | Input     | `NUM_LSU` bits                             | Ready signals from LSU indicating space for uops.         |
| `uop_lsu_valid_lsu2rvv`  | Input     | `NUM_LSU` bits                             | Valid bitmask for LSU feedback/data.                      |
| `uop_lsu_lsu2rvv`        | Input     | `NUM_LSU` `UOP_LSU2RVV_t` packets          | LSU feedback/data payloads.                               |
| `uop_lsu_ready_rvv2lsu`  | Output    | `NUM_LSU` bits                             | Ready signals back to LSU.                                |
| `rt_xrf_valid_rvv2rvs`   | Output    | `NUM_RT_UOP` bits                          | Valid bitmask for retirement updates to XRF.              |
| `rt_rvs_rvv2rvs`         | Output    | `NUM_RT_UOP` `RT2RVS_t` packets            | Retirement update payloads to RVS.                        |
| `rt_rvs_ready_rvs2rvv`   | Input     | `NUM_RT_UOP` bits                          | Ready signals from RVS for retirement updates.            |
| `async_frd_valid`        | Output    | `NUM_RT_UOP` bits                          | Valid bitmask for async FRD (Conditional `ZVE32F_ON`).    |
| `async_frd_addr`         | Output    | `NUM_RT_UOP` `REGFILE_INDEX_WIDTH` packets | Async FRD address (Conditional `ZVE32F_ON`).              |
| `async_frd_data`         | Output    | `NUM_RT_UOP` `XLEN` packets                | Async FRD data (Conditional `ZVE32F_ON`).                 |
| `async_frd_ready`        | Input     | `NUM_RT_UOP` bits                          | Ready signals for async FRD (Conditional `ZVE32F_ON`).    |
| `wr_vxsat_valid`         | Output    | 1-bit                                      | Valid bit for vxsat CSR write.                            |
| `wr_vxsat`               | Output    | `VCSR_VXSAT_WIDTH` bits                    | vxsat write data payload.                                 |
| `wr_vxsat_ready`         | Input     | 1-bit                                      | Ready signal for vxsat CSR write.                         |
| `rt2fcsr_write_valid`    | Output    | 1-bit                                      | Valid bit for FCSR write (Conditional `ZVE32F_ON`).       |
| `rt2fcsr_write_data`     | Output    | `RVFEXP_t`                                 | FCSR write data payload (Conditional `ZVE32F_ON`).        |
| `fcsr2rt_write_ready`    | Input     | 1-bit                                      | Ready signal for FCSR write (Conditional `ZVE32F_ON`).    |
| `trap_valid_rvs2rvv`     | Input     | 1-bit                                      | Valid bit for trap signal from RVS.                       |
| `trap_ready_rvv2rvs`     | Output    | 1-bit                                      | Ready signal for trap signal.                             |
| `vcsr_valid`             | Output    | 1-bit                                      | Valid bit for vcsr output.                                |
| `vector_csr`             | Output    | `RVVConfigState`                           | Vector CSR state of last retired uop.                     |
| `vcsr_ready`             | Input     | 1-bit                                      | Ready signal for vcsr output.                             |
| `rd_valid_rob2rt_o`      | Output    | `NUM_RT_UOP` bits                          | Valid bitmask for retire information to RT.               |
| `rd_rob2rt_o`            | Output    | `NUM_RT_UOP` `ROB2RT_t` packets            | Retire information payloads.                              |
| `rvv_idle`               | Output    | 1-bit                                      | Idle status of rvv_backend.                               |

<!-- mdformat off -->

<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

<!-- prettier-ignore-end -->


> **Traceability:** Generated by Gemini. Derived from upstream commit 28fdd2f4b80b1db06a4025b828807fcdc0e76f88. AI-generated/assisted; RTL is the source of truth.
