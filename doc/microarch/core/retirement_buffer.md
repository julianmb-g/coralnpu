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

# Retirement buffer

> **Intended Audience:** HW Devs

The Retirement Buffer manages the lifecycle of instructions from dispatch to retirement in the CoralNPU scalar core. It ensures that instructions are committed to the architectural state in-order, despite out-of-order execution or varying latencies of different functional units. Retirement Buffer tracks all scalar, float, and vector write-backs.

## Instruction lifecycle

Instructions transition through three main states within the buffer:

1. **Dispatched:** The instruction is enqueued into the buffer upon dispatch from the fetch unit.

2. **Completed:** All side effects—such as register writes, store completions, or faults—are committed to the architectural state. The buffer tracks completion via the `resultBuffer`.

3. **Retired:** The final state. Instructions are dequeued from the buffer in-order only when they and all preceding instructions are marked as completed.

### Dispatch contiguity constraint

The `RetirementBuffer` requires that instructions firing within a single dispatch group must be strictly contiguous. In the `io.inst` vector, all firing instructions (those with `valid` asserted) must appear in a continuous sequence starting from the first slot. Non-firing or invalid instructions are not permitted to be interspersed between valid instructions in a single dispatch cycle. This structural constraint is explicitly enforced by hardware assertions in `RetirementBuffer.scala` and ensures deterministic in-order allocation within the buffer.

## Architecture and data path

The core of the Retirement Buffer is a circular buffer (`CircularBufferMulti`) that can enqueue and dequeue multiple elements per cycle. It stores an `Instruction` bundle tracking the program counter (`addr`), instruction bits (`inst`), destination register index (`idx`), and various flags (e.g., `trap`, `isControlFlow`, `isBranch`, `isVector`).

### Dependency tracking and completion

The module maintains a `resultBuffer` to track the completion status of each enqueued instruction.

- **Register Writes:** The buffer monitors incoming write ports (`writeDataScalar`, `writeDataFloat`, `writeDataVector`) and matches their destination addresses against the `idx` of buffered instructions. When a match occurs, the instruction's data dependency is satisfied.

- **Stores:** For store instructions, the buffer waits for the `storeComplete` signal.

- **Control Flow:** For jumps and branches, the buffer verifies control flow continuity (`linkOk`) by checking if the target address matches the program counter of the subsequent instruction. Mismatches trigger a trap.

### Control flow continuity and mini-mode optimization

The Retirement Buffer tracks the expected program counter of the next instruction across dispatch cycles. This "mini-mode" relies on state registers (`regLastTarget`, `regLastAddr`, and `regLastIsBranch`) that latch the target and PC of the last fired instruction in a dispatch group.

For the first instruction of a subsequent dispatch group, the buffer verifies control flow continuity (`linkOk`) by comparing the incoming PC against the expected `regLastTarget` (or `regLastAddr + 4` for a fall-through branch). If the fetcher provides an instruction that does not match this expected target (e.g., due to an unresolved misprediction), the `linkOk` check fails, and the buffer flags the instruction to eventually trigger a trap upon retirement.

### Fault and trap handling

Faulting instructions (e.g., illegal instructions or misaligned accesses) are enqueued and tracked. When an instruction marked with a trap reaches the head of the buffer and is ready to retire:

- The `trapRetired` signal is asserted.

- The `instBuffer.io.flush` is triggered, flushing all subsequent speculative instructions.

- The `io.trapPending` signal is raised to notify the broader pipeline.

## Interfaces

| Signal Name       | Direction | Type                                                     | Description                                                           |
| :---------------- | :-------- | :------------------------------------------------------- | :-------------------------------------------------------------------- |
| `inst`            | Input     | Vec(Lanes, Decoupled(FetchInstruction))                  | Dispatched instructions.                                              |
| `targets`         | Input     | Vec(Lanes, UInt)                                         | Branch targets.                                                       |
| `jalrTargets`     | Input     | Vec(Lanes, UInt)                                         | JALR targets.                                                         |
| `jump`            | Input     | Vec(Lanes, Bool)                                         | Jump indicator.                                                       |
| `branch`          | Input     | Vec(Lanes, Bool)                                         | Branch indicator.                                                     |
| `storeComplete`   | Input     | Valid(UInt)                                              | Address of completed store instruction.                               |
| `writeAddrScalar` | Input     | Vec(Lanes, RegfileWriteAddrIO)                           | Scalar register write addresses.                                      |
| `writeDataScalar` | Input     | Vec(Lanes+2, Valid(RegfileWriteDataIO))                  | Scalar register write data.                                           |
| `writeAddrFloat`  | Input     | RegfileWriteAddrIO                                       | Float register write addresses.                                       |
| `writeDataFloat`  | Input     | Vec(2, Valid(RegfileWriteDataIO))                        | Float register write data.                                            |
| `writeAddrVector` | Input     | Vec(Lanes, RegfileWriteAddrIO)                           | Vector register write addresses.                                      |
| `writeDataVector` | Input     | Vec(Lanes, Valid(VectorWriteDataIO))                     | Vector register write data.                                           |
| `fault`           | Input     | Valid(FaultManagerOutput)                                | Incoming faults.                                                      |
| `nSpace`          | Output    | UInt                                                     | Available space in buffer.                                            |
| `nRetired`        | Output    | UInt                                                     | Number of retired instructions.                                       |
| `empty`           | Output    | Bool                                                     | Buffer is empty.                                                      |
| `trapPending`     | Output    | Bool                                                     | A trap is pending.                                                    |

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/coralnpu/RetirementBuffer.scala:L52` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
