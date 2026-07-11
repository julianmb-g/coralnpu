# Branch Resolution Unit (BRU)

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

> **Intended Audience:** Hardware Developers

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

The Branch Resolution Unit (BRU) is responsible for calculating branch target addresses, evaluating branch conditions, and orchestrating control flow changes, including traps, exceptions, and interrupts.

## Architectural Overview

The BRU evaluates operands from the integer register file (RS1 and RS2) to resolve conditional branches (e.g., `BEQ`, `BLT`). It also manages unconditional jumps (`JAL`, `JALR`) and handles architectural state updates for system instructions (`ECALL`, `EBREAK`, `MRET`, `WFI`) and asynchronous faults/interrupts.

When instantiated as the first unit in the pipeline (`first = true`), the BRU assumes additional responsibilities for trap handling, interfacing directly with the Control and Status Registers (CSR) and the `FaultManager` to context switch into machine mode and record exception state.

### Supported Operations (`BruOp`)

- **Conditional Branches**: `BEQ`, `BNE`, `BLT`, `BGE`, `BLTU`, `BGEU`
- **Unconditional Jumps**: `JAL`, `JALR`
- **System Instructions** (Pipeline 0 only): `EBREAK`, `ECALL`, `MPAUSE`, `MRET`, `WFI`
- **Fault/Interrupt Handling** (Pipeline 0 only): `FAULT`

## Interface Definitions

The BRU communicates with the instruction decoder, register file, CSRs, and the fault manager.

| Port Name        | Direction | Type                        | Description                                                                                                     |
| ---------------- | --------- | --------------------------- | --------------------------------------------------------------------------------------------------------------- |
| `req`            | Input     | `Valid(BruCmd)`             | Decode cycle request containing operation, PC, immediate target, and link register index.                       |
| `rs1` / `rs2`    | Input     | `RegfileReadDataIO`         | Read data from the integer register file for condition evaluation.                                              |
| `csr`            | In/Out    | `CsrBruIO`                  | (Pipeline 0) Interface to the CSR unit to read `mtvec`/`mepc` and write trap state (`mepc`, `mcause`, `mtval`). |
| `target`         | Input     | `RegfileBranchTargetIO`     | Asynchronous branch target forward from the register file (used for `JALR`).                                    |
| `fault_manager`  | Input     | `Valid(FaultManagerOutput)` | (Pipeline 0) Exception payload from the Fault Manager.                                                          |
| `rd`             | Output    | `Valid(RegfileWriteDataIO)` | Link data writeback (PC+4) for `JAL` and `JALR`.                                                                |
| `taken`          | Output    | `BranchTakenIO`             | Broadcasts if a control flow change is occurring and the resolved target PC.                                    |
| `actually_taken` | Output    | `Bool`                      | High if the branch condition evaluated to true.                                                                 |
| `real_target`    | Output    | `UInt`                      | The resolved target address for the branch.                                                                     |
| `interlock`      | Output    | `Bool`                      | (Pipeline 0) Halts instruction issue for system/fault operations.                                               |

## Trap & Exception Handling

For pipeline 0 instances, the BRU acts as the gateway for trap entry and return:

- **JALR Target Masking**: The `JALR` target PC is masked with `0xFFFFFFFE` to align it.
- **Trap Vectors**: Upon an exception or interrupt, the target PC is redirected to `mtvec` (with the lower bits masked).
- **MRET**: Upon an `MRET`, the target PC is redirected to `mepc` and the CSR mode is restored to Machine Mode.
- **mcause Mapping**: Sets `mcause` to `11` for `ECALL`, `25` (Usage Fault) for `EBREAK`, or passes through the `mcause` from the `FaultManager` or `interrupt_cause` from the CSR.
- **mtval Mapping**: Captures the faulting PC (`pcEx`) or passes through the `mtval` from the `FaultManager`.



---

> **Traceability:** Generated by Gemini. Derived from upstream commit 7d6e35cff17732cad8f99b866333423ea1f4d750.
