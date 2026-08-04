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

# Execution pipeline

> **Intended Audience:** HW Devs

The CoralNPU scalar core (`SCore`) implements an in-order execution pipeline responsible for scalar operations, control flow, and vector instruction dispatch to the RVV backend.

## Pipeline stages

| Stage | Module | Description | Source Reference |
| :--- | :--- | :--- | :--- |
| **Fetch** | `Fetch` / `UncachedFetch` | Retrieves instructions from the instruction memory via the I-Bus. Handles branch prediction and flushing. | [`hdl/chisel/src/coralnpu/scalar/Fetch.scala`](../../hdl/chisel/src/coralnpu/scalar/Fetch.scala) |
| **Decode & Dispatch** | `DispatchV2` | Decodes instructions, checks structural hazards (scoreboard), and dispatches to functional units. | [`hdl/chisel/src/coralnpu/scalar/Decode.scala`](../../hdl/chisel/src/coralnpu/scalar/Decode.scala) |
| **Execute** | `Alu`, `Bru`, `Mlu`, `Dvu` | Executes integer arithmetic, branch resolution, multiplication, and division. | [`hdl/chisel/src/coralnpu/scalar/Alu.scala`](../../hdl/chisel/src/coralnpu/scalar/Alu.scala) |
| **Memory** | `Lsu` | Handles load/store operations to the D-Bus and E-Bus, including scalar and vector memory access. | [`hdl/chisel/src/coralnpu/scalar/Lsu.scala`](../../hdl/chisel/src/coralnpu/scalar/Lsu.scala) |
| **Retirement** | `RetirementBuffer` | Tracks in-flight instructions, manages faults, and commits state updates. | [`hdl/chisel/src/coralnpu/RetirementBuffer.scala`](../../hdl/chisel/src/coralnpu/RetirementBuffer.scala) |

## Dataflow

The execution dataflow is managed centrally by the `SCore` module.

- **Operand Fetch:** Source operands (`rs1`, `rs2`) are read from the `Regfile` or `FRegfile` and routed to the respective functional units.

- **Execution & Writeback:** Execution units compute results and assert valid signals. Results are arbitrated and written back to the register files.

- **Vector Dispatch:** Vector instructions are decoded and passed to the RVV core via the `rvvcore` interface. The scalar core manages the CSRs (`vstart`, `vl`, `vtype`) required for vector execution.

[Source: `hdl/chisel/src/coralnpu/scalar/SCore.scala`](../../hdl/chisel/src/coralnpu/scalar/SCore.scala)

## Core interfaces

| Interface | Type | Description |
| :--- | :--- | :--- |
| `ibus` | `IBusIO` | Instruction fetch bus (AXI/Fabric). |
| `dbus` | `DBusIO` | Data memory bus. |
| `ebus` | `EBusIO` | Extended/peripheral bus. |
| `rvvcore` | `RvvCoreIO` | Interface to the Vector (RVV) backend. |
| `csr` | `CsrInOutIO` | CSR control and status interface. |
| `debug` | `DebugIO` | Debug trace interface. |

[Source: `hdl/chisel/src/coralnpu/scalar/SCore.scala`](../../hdl/chisel/src/coralnpu/scalar/SCore.scala)

## State machines and control

The scalar pipeline uses distributed control logic rather than a single overarching FSM. Pipeline stalls are managed via interlocking logic in the `DispatchV2` unit and the `Regfile` scoreboard.

### LSU cell state

The `Lsu` uses an internal FSM (`LsuCellState`) to track the lifecycle of memory transactions:

- `DONE`: Transaction complete.

- `W_DATA`: Waiting for data phase.

- `W_START`: Waiting for transaction start.

- `W_RESP`: Waiting for response.

- `W_WB`: Waiting for writeback.

[Source: `hdl/chisel/src/coralnpu/scalar/Lsu.scala`](../../hdl/chisel/src/coralnpu/scalar/Lsu.scala)

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/SCore.scala`, `hdl/chisel/src/coralnpu/scalar/Lsu.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
