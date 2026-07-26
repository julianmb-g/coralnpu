# Fault Manager (FaultManager)

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

The `FaultManager` module acts as the centralized exception aggregation, prioritization, and metadata generation unit for the CoralNPU scalar core. It evaluates synchronous fault signals from all instruction lanes, memory access faults, and instruction fetch faults in the same clock cycle to determine the highest-priority trap and forward the appropriate RISC-V exception metadata to the pipeline.

## Architectural Purpose

The `FaultManager` provides three primary functions:
1. **Fault Prioritization**: Aggregates fault signals across multiple instruction lanes, instruction fetch buffers, the memory subsystem, and the vector coprocessor. It prioritizes these faults in a strict hierarchical order.
2. **Metadata Generation**: Calculates the exact RISC-V architectural exception registers (`mepc`, `mcause`, and `mtval`) for the highest-priority fault.
3. **Pipeline Control**: Asserts the `decode` pipeline redirect signal to flush the current execution context and steer control flow to the designated trap handler.

## Interface Definition

The physical ports of the `FaultManager` module map external subsystems directly to the prioritization logic:

| Port Name | Direction | Width / Type | Description |
| :--- | :--- | :--- | :--- |
| `io.in.fault` | Input | `Vec(Lanes, Bundle)` | Lane-specific fault signals (csr, jal, jalr, bxx, undef, rvv) |
| `io.in.pc` | Input | `Vec(Lanes, PCBundle)`| Program counters corresponding to instructions in each lane |
| `io.in.undef` | Input | `Vec(Lanes, InstBundle)`| Original instruction bits for undef and illegal instructions |
| `io.in.jal` | Input | `Vec(Lanes, PCBundle)`| Jump target addresses computed by JAL instructions |
| `io.in.jalr` | Input | `Vec(Lanes, PCBundle)`| Jump target addresses computed by JALR instructions |
| `io.in.memory_fault` | Input | `Valid(FaultInfo)` | Exception details from the load/store data memory subsystem |
| `io.in.rvv_fault` | Input | `Valid(OutputBundle)` | Exception details forwarded from the vector coprocessor backend |
| `io.in.fetchFault` | Input | `Valid(PC)` | Exception address indicating an instruction fetch access fault |
| `io.out` | Output | `Valid(OutputBundle)` | Prioritized exception output containing `mepc`, `mcause`, `mtval`, and `decode` |

## Exception Prioritization Hierarchy

When multiple exceptions occur concurrently, a strict prioritization hierarchy selects a single active exception. The `FaultManager` evaluates exceptions in the following descending order:

1. **Memory Load Fault** (Highest Priority)
2. **Memory Store Fault**
3. **Vector Backend Fault** (`rvv_fault` from the vector execution engine)
4. **Instruction Lane Fault** (Inline faults occurring during decode/dispatch)
5. **Instruction Fetch Access Fault** (Lowest Priority)

### Multi-Lane Prioritization

If multiple instruction lanes report inline faults simultaneously, a `PriorityEncoder` selects the lowest-indexed lane (where Lane 0 has the highest priority and Lane N-1 has the lowest). 

Within the selected lane, the active exception or jump redirection is resolved according to individual pipeline signals (`csr`, `jal`, `jalr`, `bxx`, `undef`, or `rvv` dispatch).

## Architectural Exception Metadata

Based on the prioritized exception source, the `FaultManager` populates the control register values and control signals:

| Selected Fault Source | mcause | mtval Contents | decode Signal |
| :--- | :--- | :--- | :--- |
| Memory Load Fault | `5.U` (Load access fault) | Faulting memory address | `false.B` |
| Memory Store Fault | `7.U` (Store access fault) | Faulting memory address | `false.B` |
| Vector Backend Fault (`rvv_fault`) | From vector backend (default: `2.U`) | From vector backend | `false.B` |
| Illegal CSR Instruction | `2.U` (Illegal instruction) | `0.U` | `true.B` |
| JAL Target Redirection | `0.U` (Redirection) | Target address | `true.B` |
| JALR Target Redirection | `0.U` (Redirection) | Target address (masked: `addr & ~1.U`) | `true.B` |
| Conditional Branch Redirection | `0.U` (Redirection) | `0.U` | `true.B` |
| Illegal/Undefined Instruction | `2.U` (Illegal instruction) | Faulting instruction word | `true.B` |
| RVV Dispatch Fault | `2.U` (Illegal instruction) | Faulting instruction word | `true.B` |
| Instruction Fetch Access Fault | `1.U` (Instruction access fault) | `0.U` | `false.B` |

### Exception PC (mepc) Resolution

The instruction pointer stored in `mepc` represents the precise location of the fault:
- **Memory Subsystem Faults**: Set to the program counter of the faulting instruction (`io.in.memory_fault.bits.epc`).
- **Vector Backend Faults**: Forwarded directly from the backend vector controller (`io.in.rvv_fault.bits.mepc`).
- **Instruction Lane Faults**: Set to the program counter of the highest-priority lane containing an active fault (`io.in.pc(first_fault).pc`).
- **Instruction Fetch Faults**: Set to the address that triggered the memory fetch exception (`io.in.fetchFault.bits`).

## Vector Exceptions: Dispatch vs. Backend

The `FaultManager` distinguishes between two classes of vector exceptions:
- **RVV Dispatch Faults**: Synchronous exceptions detected at dispatch time (e.g., trying to execute a vector instruction when the vector unit is disabled or configuring invalid vector state). These are routed via `io.in.fault(x).rvv` and resolved as inline instruction lane faults (priority level 4). They result in an illegal instruction exception (`mcause` = 2) with the faulting instruction loaded in `mtval`.
- **Vector Backend Faults (rvv_fault)**: Asynchronous, deferred execution exceptions occurring within the vector coprocessor backend (e.g., division by zero or vector memory access faults). These are forwarded directly from the vector core to `io.in.rvv_fault`. Because they occur during execution, they have higher priority than pipeline-decode faults (priority level 3), and their metadata is preserved and forwarded directly to the architectural registers.

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-25 - **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7) - **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/FaultManager.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

> **Traceability:** Generated by Gemini. Derived from upstream commit 6a8cc54a67fb4ca7ecda116453fbdc4a97994ebf.
