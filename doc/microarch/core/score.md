# Scalar Core Pipeline Wiring

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

## Pipeline Instantiation and Wiring

The `SCore` module instantiates the core functional units and wires them together. The primary components instantiated are:

| Component             | Module/Implementation      | Description                                                                              |
| :-------------------- | :------------------------- | :--------------------------------------------------------------------------------------- |
| **Fetch**             | `Fetch` or `UncachedFetch` | Fetches instructions. Chosen via `enableFetchL0` parameter.                              |
| **Decode/Dispatch**   | `DispatchV2`               | Decodes instructions and tracks hazards.                                                 |
| **Register File**     | `Regfile` / `FRegfile`     | Integer register file, and conditionally a floating-point register file (`enableFloat`). |
| **ALU**               | `Alu`                      | Integer arithmetic logic unit. Replicated per `instructionLanes`.                        |
| **BRU**               | `Bru`                      | Branch resolution unit. Replicated per `instructionLanes`.                               |
| **MLU**               | `Mlu`                      | Multiplier unit.                                                                         |
| **DVU**               | `Dvu`                      | Divider unit.                                                                            |
| **LSU**               | `Lsu`                      | Load/Store Unit for memory access.                                                       |
| **CSR**               | `Csr`                      | Control and Status Registers.                                                            |
| **Fault Manager**     | `FaultManager`             | Aggregates and prioritizes faults.                                                       |
| **Retirement Buffer** | `RetirementBuffer`         | In-order instruction retirement (can be `mini` if `!useRetirementBuffer`).               |

**Wiring Highlights:**

- **Dispatch to Units**: `dispatch.io` drives the `req` interfaces of the `Alu`, `Bru`, `Mlu`, `Dvu`, and `Lsu`.
- **Execution to Regfile**: Write ports of `Regfile` arbitrate results from execution units (ALU, BRU, LSU, MLU, DVU) and external units like the Vector Core (`rvvcore`) or debug interfaces.
- **Fault Handling**: Hardware exceptions from decode (e.g., undef, branch faults) and execution (memory faults) are routed to the `FaultManager`, which informs the `RetirementBuffer`.

## Retirement Buffer Routing

The `RetirementBuffer` acts as the sync point for committing architectural state.

| Signal Group            | Source / Sink                                     | Description                                                     |
| :---------------------- | :------------------------------------------------ | :-------------------------------------------------------------- |
| **Inputs (Dispatch)**   | `dispatch.io.inst`, `jump`, `branch`              | In-flight instruction tracking.                                 |
| **Target PCs**          | `dispatch.io.bruTarget`, `regfile.io.target.data` | Jump and branch targets.                                        |
| **Write Data (Scalar)** | `regfile.io.writeData`                            | Routes scalar write data tracking for retirement.               |
| **Write Data (Vector)** | `io.rvvcore.get.rd_rob2rt_o`                      | If RVV enabled, tracks vector write data per lane.              |
| **Write Data (Float)**  | `fRegfile.get.io.write_ports`                     | If Float enabled, tracks float write data.                      |
| **Completion Status**   | `lsu.io.storeComplete`, `fault_manager.io.out`    | Flags for memory and exception completion.                      |
| **Backpressure**        | `dispatch.io.retirement_buffer_*`                 | Exposes `nSpace`, `empty`, and `trapPending` to stall dispatch. |

## SCore Interface Boundaries

The `SCore` top-level exposes the following boundaries to the wider system hierarchy:

| Interface Port                     | Direction | Type                   | Description                                              |
| :--------------------------------- | :-------- | :--------------------- | :------------------------------------------------------- |
| `csr`                              | In/Out    | `CsrInOutIO`           | External CSR read/write access.                          |
| `halted`, `fault`, `wfi`           | Output    | `Bool`                 | Core status flags (halted, faulted, wait-for-interrupt). |
| `irq`, `timer_irq`, `software_irq` | Input     | `Bool`                 | External interrupt pins.                                 |
| `dm`                               | In/Out    | `CoreDMIO`             | Debug Module interface (single step, debug PC, resume).  |
| `ibus`                             | In/Out    | `IBusIO`               | Instruction fetch bus.                                   |
| `dbus`, `ebus`                     | In/Out    | `DBusIO`, `EBusIO`     | Data and Execution bus interfaces.                       |
| `rvvcore`                          | In/Out    | `RvvCoreIO`            | Vector core execution interface (Optional).              |
| `iflush`, `dflush`                 | In/Out    | `IFlushIO`, `DFlushIO` | Instruction and Data cache flush controls.               |
| `debug`                            | In/Out    | `DebugIO`              | Cycle, PC, instruction trace and debug output.           |

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------


> **Traceability:** Generated by Gemini. Derived from upstream commit 28fdd2f4b80b1db06a4025b828807fcdc0e76f88. AI-generated/assisted; RTL is the source of truth.
