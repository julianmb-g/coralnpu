# Fault Manager

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


The `FaultManager` aggregates and prioritizes hardware exceptions from the instruction pipeline, memory subsystem, and vector execution units.

## Interfaces

| Port Name         | Direction | Type / Width                | Description                                                                  |
| :---------------- | :-------- | :-------------------------- | :--------------------------------------------------------------------------- |
| `in.fault`        | Input     | `Vec[Bundle]`               | Per-lane fault flags (`csr`, `jal`, `jalr`, `bxx`, `undef`, `rvv`).          |
| `in.pc`           | Input     | `Vec[UInt]`                 | Per-lane program counters for identifying the faulting instruction PC.       |
| `in.memory_fault` | Input     | `Valid(FaultInfo)`          | Exception details from the load/store unit (LSU).                            |
| `in.rvv_fault`    | Input     | `Valid(FaultManagerOutput)` | Vector unit exception details (optional, based on configuration).            |
| `in.undef`        | Input     | `Vec[UInt]`                 | Per-lane raw instruction data, captured for `mtval` on illegal instructions. |
| `in.jal`          | Input     | `Vec[UInt]`                 | Per-lane JAL targets, captured for instruction address misaligned traps.     |
| `in.jalr`         | Input     | `Vec[UInt]`                 | Per-lane JALR targets, captured for instruction address misaligned traps.    |
| `in.fetchFault`   | Input     | `Valid(UInt)`               | Instruction access fault details from the fetch unit.                        |
| `out`             | Output    | `Valid(FaultManagerOutput)` | Aggregated and prioritized trap state (`mepc`, `mcause`, `mtval`, `decode`). |

## Fault Prioritization & Mappings

## Vector Fault Partial State (`vstart`) Handling

The current implementation of the `FaultManager` does not capture or preserve the vector partial execution state (`vstart`). If a vector instruction faults mid-execution (e.g., during an out-of-bounds memory access by a scatter/gather operation), the exact element index where the fault occurred is lost. The `FaultManager` will report the base PC of the faulting vector instruction via `mepc`, but software recovery of the partially completed vector operation is currently unsupported.

The following table details the precise structural logic mapping hardware events to RISC-V exception codes (`mcause`), trap values (`mtval`), and program counters (`mepc`), strictly based on the MuxCase priority encoder in `FaultManager.scala`:

| Fault Condition                        | `mcause` (Exception Code)           | `mtval` (Trap Value)       | `mepc` Source           |
| :------------------------------------- | :---------------------------------- | :------------------------- | :---------------------- |
| **Load Fault** (`memory_fault` read)   | `5` (Load Access Fault)             | `memory_fault.bits.addr`   | `memory_fault.bits.epc` |
| **Store Fault** (`memory_fault` write) | `7` (Store Access Fault)            | `memory_fault.bits.addr`   | `memory_fault.bits.epc` |
| **RVV Fault** (`rvv_fault`)            | RVV exception or `2` (Illegal Inst) | RVV trap value or `0`      | RVV fault PC or `0`     |
| **CSR Fault**                          | `2` (Illegal Instruction)           | `0`                        | Faulting Instruction PC |
| **JAL Misaligned**                     | `0` (Inst Addr Misaligned)          | `jal.target`               | Faulting Instruction PC |
| **JALR Misaligned**                    | `0` (Inst Addr Misaligned)          | `jalr.target & 0xFFFFFFFE` | Faulting Instruction PC |
| **BXX Misaligned**                     | `0` (Inst Addr Misaligned)          | `0`                        | Faulting Instruction PC |
| **Undef Instruction**                  | `2` (Illegal Instruction)           | `undef.inst` (Raw Inst)    | Faulting Instruction PC |
| **RVV Dispatch Fault**                 | `2` (Illegal Instruction)           | `undef.inst` (Raw Inst)    | Faulting Instruction PC |
| **Instruction Access Fault**           | `1` (Inst Access Fault)             | `0`                        | `fetchFault.bits`       |

## Pipeline Interfaces

For all instruction faults (CSR, JAL, JALR, BXX, Undef, RVV Dispatch), the `decode` flag is set to `true`, indicating the fault originated during the decode/dispatch stage of the execution pipeline. The `first_fault` priority encoder guarantees that if multiple lanes encounter a fault simultaneously, the fault from the lowest-indexed instruction lane is reported.

## Relevant Testbenches

The following C++ (Cocotb) tests validate the fault handling and prioritization mechanisms within the standalone IP simulation environment:

- [`tests/cocotb/exceptions/illegal.cc`](../../../tests/cocotb/exceptions/illegal.cc)
- [`tests/cocotb/exceptions/instr_align_0.cc`](../../../tests/cocotb/exceptions/instr_align_0.cc)
- [`tests/cocotb/exceptions/instr_fault.cc`](../../../tests/cocotb/exceptions/instr_fault.cc)
- [`tests/cocotb/exceptions/load_fault_0.cc`](../../../tests/cocotb/exceptions/load_fault_0.cc)
- [`tests/cocotb/exceptions/store_fault_0.cc`](../../../tests/cocotb/exceptions/store_fault_0.cc)
- [`tests/cocotb/unreachable_prefetch_fault.cc`](../../../tests/cocotb/unreachable_prefetch_fault.cc)
- [`tests/cocotb/vector_store_fault.cc`](../../../tests/cocotb/vector_store_fault.cc)

<!-- mdformat off -->

<!-- prettier-ignore -->
--------------------------------------------------------------------------------


> **Traceability:** Generated by Gemini. Derived from upstream commit 28fdd2f4b80b1db06a4025b828807fcdc0e76f88. AI-generated/assisted; RTL is the source of truth.
