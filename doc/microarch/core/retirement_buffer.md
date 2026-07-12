# Retirement Buffer

<!--
 Copyright 2024 Google LLC

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

The Retirement Buffer manages the lifecycle of instructions from dispatch to retirement in the CoralNPU scalar core. It ensures that instructions are committed to the architectural state in-order, despite out-of-order execution or varying latencies of different functional units. Retirement Buffer tracks all scalar, float, and vector write-backs.

## Instruction Lifecycle

Instructions transition through three main states within the buffer:

1.  **Dispatched:** The instruction is enqueued into the buffer upon dispatch from the fetch unit.
2.  **Completed:** All side effects—such as register writes, store completions, or faults—are committed to the architectural state. The buffer tracks completion via the `resultBuffer`.
3.  **Retired:** The final state. Instructions are dequeued from the buffer in-order only when they and all preceding instructions are marked as completed.

### Dispatch Contiguity Constraint

The `RetirementBuffer` requires that instructions firing within a single dispatch group must be strictly contiguous. In the `io.inst` vector, all firing instructions (those with `valid` asserted) must appear in a continuous sequence starting from the first slot. Non-firing or invalid instructions are not permitted to be interspersed between valid instructions in a single dispatch cycle. This structural constraint is explicitly enforced by hardware assertions in `RetirementBuffer.scala` and ensures deterministic in-order allocation within the buffer.

## Architecture and Data Path

The core of the Retirement Buffer is a circular buffer (`CircularBufferMulti`) that can enqueue and dequeue multiple elements per cycle. It stores an `Instruction` bundle tracking the program counter (`addr`), instruction bits (`inst`), destination register index (`idx`), and various flags (e.g., `trap`, `isControlFlow`, `isBranch`, `isVector`).

### Dependency Tracking and Completion

The module maintains a `resultBuffer` to track the completion status of each enqueued instruction.

- **Register Writes:** The buffer monitors incoming write ports (`writeDataScalar`, `writeDataFloat`, `writeDataVector`) and matches their destination addresses against the `idx` of buffered instructions. When a match occurs, the instruction's data dependency is satisfied.
- **Stores:** For store instructions, the buffer waits for the `storeComplete` signal.
- **Control Flow:** For jumps and branches, the buffer verifies control flow continuity (`linkOk`) by checking if the target address matches the program counter of the subsequent instruction. Mismatches trigger a trap.

### Control Flow Continuity and Mini-mode Optimization

The Retirement Buffer tracks the expected program counter of the next instruction across dispatch cycles. This "mini-mode" relies on state registers (`regLastTarget`, `regLastAddr`, and `regLastIsBranch`) that latch the target and PC of the last fired instruction in a dispatch group.

For the first instruction of a subsequent dispatch group, the buffer verifies control flow continuity (`linkOk`) by comparing the incoming PC against the expected `regLastTarget` (or `regLastAddr + 4` for a fall-through branch). If the fetcher provides an instruction that does not match this expected target (e.g., due to an unresolved misprediction), the `linkOk` check fails, and the buffer flags the instruction to eventually trigger a trap upon retirement.

### Fault and Trap Handling

Faulting instructions (e.g., illegal instructions or misaligned accesses) are enqueued and tracked. When an instruction marked with a trap reaches the head of the buffer and is ready to retire:

- The `trapRetired` signal is asserted.
- The `instBuffer.io.flush` is triggered, flushing all subsequent speculative instructions.
- The `io.trapPending` signal is raised to notify the broader pipeline.

## Interfaces

| Signal Name       | Direction | Type                                                     | Description                                                           |
| :---------------- | :-------- | :------------------------------------------------------- | :-------------------------------------------------------------------- |
| `inst`            | Input     | `Vec(p.instructionLanes, Decoupled(FetchInstruction))`   | Vector of decoupled fetch instructions to enqueue.                    |
| `targets`         | Input     | `Vec(p.instructionLanes, UInt)`                          | Target addresses for control flow instructions.                       |
| `jalrTargets`     | Input     | `Vec(p.instructionLanes, UInt)`                          | JALR target addresses.                                                |
| `jump`            | Input     | `Vec(p.instructionLanes, Bool)`                          | Flags indicating if an instruction is a jump.                         |
| `branch`          | Input     | `Vec(p.instructionLanes, Bool)`                          | Flags indicating if an instruction is a branch.                       |
| `storeComplete`   | Input     | `Valid(UInt)`                                            | Valid signal indicating a store operation has completed.              |
| `writeAddrScalar` | Input     | `Vec(p.instructionLanes, RegfileWriteAddrIO)`            | Destination register addresses for scalar writes.                     |
| `writeDataScalar` | Input     | `Vec(p.instructionLanes + 2, Valid(RegfileWriteDataIO))` | Valid write-back data for scalar registers.                           |
| `writeAddrFloat`  | Input     | `RegfileWriteAddrIO`                                     | Destination register addresses for float writes (Optional).           |
| `writeDataFloat`  | Input     | `Vec(2, Valid(RegfileWriteDataIO))`                      | Valid write-back data for float registers (Optional).                 |
| `writeAddrVector` | Input     | `Vec(p.instructionLanes, RegfileWriteAddrIO)`            | Destination register addresses for vector writes (Optional).          |
| `writeDataVector` | Input     | `Vec(p.instructionLanes, Valid(VectorWriteDataIO))`      | Valid write-back data for vector registers (Optional).                |
| `fault`           | Input     | `Valid(FaultManagerOutput)`                              | Valid fault information from the Fault Manager.                       |
| `nSpace`          | Output    | `UInt`                                                   | The number of available slots in the buffer.                          |
| `nRetired`        | Output    | `UInt`                                                   | The number of instructions successfully retired in the current cycle. |
| `empty`           | Output    | `Bool`                                                   | High if the retirement buffer is empty.                               |
| `trapPending`     | Output    | `Bool`                                                   | High if a trap is pending retirement.                                 |
| `debug`           | Output    | `RetirementBufferDebugIO`                                | Debug interface (Optional).                                           |

## Verification

This hardware block is validated as part of the top-level simulation and verification environment.

- [CoralNPU Top-Level Testbench](../../../tests/uvm/tb/coralnpu_tb_top.sv)
- [Core Mini AXI Verilator Testbench](../../../tests/verilator_sim/coralnpu/core_mini_axi_tb.cc)

<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

> **Provenance & Traceability** - **Verified As Of:** 2026-07-07 - **Upstream Commit:** 8ba6f4108901602e14e28345b4bd009e6f3b6897 - **Primary Source(s):** `hdl/chisel/src/coralnpu/RetirementBuffer.scala:L52` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- prettier-ignore-end -->
