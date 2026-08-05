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
# Tensor processing engine (TPE)

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

> **Intended Audience:** HW Devs, SW/Compiler Devs

The Tensor Processing Engine (TPE) (`zvt` module) is a matrix/tensor execution unit tightly coupled to the Vector Core (RVV). It implements the custom **Zvt** tensor extensions, supporting high-throughput matrix multiply-accumulate operations.

### Custom zvt instructions

The CoralNPU supports custom **Zvt** configuration instructions that control the TPE state and matrix type (`mtype`):

- `msettn`: Sets the vector length/tile parameter (`vl`).
- `msettm`: Sets the matrix tile mask or type configuration (`tm`).
- `msettk`: Sets the matrix operation parameters or KMAX (`tk`).
- `msetmtype`: Configures the full matrix type.
- `msetmtypei`: Configures matrix type using immediate values.

These instructions configure the matrix execution pipeline before processing standard or extended vector uops.

## Architectural function and observable behavior

The TPE operates as a decoupled coprocessor pipeline alongside main vector units. It receives decoded micro-operations (uops) from the vector dispatcher, managing its execution state, processing element (PE) array, and dedicated accumulator register file. It independently dispatches tensor load/store requests to the Load-Store Unit (LSU) and collects responses.

## Interfaces and connections

TPE interfaces with the vector dispatcher, RVV Reorder Buffer (ROB) for retirement, and LSU for memory accesses.

| Port Name | Direction | Type / Description |
| :--- | :--- | :--- |
| `uopVld` / `uop` / `uopRdy` | In / In / Out | `ZVT_RS_t` - Vector micro-ops from the vector instruction dispatcher. |
| `res_vme2rvv` (valid/ready) | Out | `PU2ROB_t` - Retirement responses sent back to the RVV Reorder Buffer (ROB) to commit instructions. |
| `uop_vme2lsu` (valid/ready) | Out | `UOP_VME2LSU_t` - Memory requests dispatched to the Load-Store Unit for matrix loads/stores. |
| `uop_lsu2vme` (valid/ready) | In | `UOP_LSU2VME_t` - Memory responses and data returned from the LSU. |
| `fpexp` (valid/ready) | Out | `RVFEXP_t` - Floating-point exception flags sent to the core exception handling unit. |
| `zvtBusy` | Out | Indicates if the TPE pipeline, PE Array, or internal FIFOs are actively processing. |
| `flush` | In | Pipeline flush signal for clearing pending state on exceptions or branch mispredictions. |

## Data paths and pipeline state

TPE consists of four sub-components:

1. **TPE Controller (`zvt_ctrl`)**: Decodes uops and sequences execution. Drives `peCmd` vectors to Processing Elements, arbitrates register access, manages LSU request/response FIFOs.
2. **PE Array (`zvt_pe_array`)**: Parallel array of Processing Elements (PEs) for bulk arithmetic. Supports mixed-precision math via multipliers (`zvt_pe_mulbulk`) and adders (`zvt_pe_adder`) for float/integer.
3. **Accumulator (`zvt_acc`)**: Dedicated, large-capacity register file (`zvt_acc_reg`) storing intermediate accumulated matrix multiplication results.
4. **LSU FIFOs (`multi_fifo`)**: Asynchronous queues (`uop_vme2lsu_rs`, `uop_lsu2vme_rs`) decoupling TPE execution from LSU memory latency.

## Edge cases and backpressure

- **Stall & Backpressure**: TPE uses `valid/ready` handshakes (`uopRdy`, `peCmdRdy`, `res_vme2rvv_rdy`). Backpressure propagates to the dispatcher if the accumulator is busy or LSU queues fill (`vmelsuAfull`).
- **Pipeline Flushing**: On `flush`, TPE clears internal LSU FIFOs and aborts in-flight matrix operations in the PE array.
- **Busy State Coherency**: `zvtBusy` remains asserted during computation and while LSU FIFOs contain pending transactions (`!vmelsuAempty || !vmelsuresAempty`).

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/verilog/rvv/design/Zvt/zvt.sv`, `hdl/verilog/rvv/design/Zvt/zvt_ctrl.sv`, `hdl/verilog/rvv/design/Zvt/zvt_pe_array.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.


> **Traceability:** Generated by Gemini. Derived from upstream commit d9622642c63f7eba6e0c9baa7fea2188d32e28e3.