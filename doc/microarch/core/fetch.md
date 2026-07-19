# Uncached Fetch Unit

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

The `UncachedFetch` module is the primary instruction fetch mechanism for the CoralNPU core. It initiates instruction fetches via the `IBus`, handles pre-decoding of branches, manages multi-cycle fetch transactions using a Fetch Reorder Buffer, and buffers instructions for downstream decoders.

## Submodule Architecture

| Component           | Description                                                                                                                                                                                                  |
| ------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `FetchControl`      | Tracks the Program Counter (PC), detects branch/jump/flush conditions, and manages speculative fetch issuance. Performs lightweight pre-decoding to predict branches (`PredictJump`) within the fetch group. |
| `Fetcher`           | Manages the `IBus` interface. Contains a `FetchReorderBuffer` that tracks up to 2 concurrent in-flight memory transactions (`maxConcurrentTx = 2`), matching responses to transaction IDs.                   |
| `InstructionBuffer` | A generic FIFO-like buffer that stores fetched instructions (up to `fetchInstrSlots * 2` elements) to absorb variable latency from the `IBus` and smooth the delivery to the decoders.                       |

## Interfaces

| Interface    | Type                 | Direction | Description                                                                                     |
| ------------ | -------------------- | --------- | ----------------------------------------------------------------------------------------------- |
| `ibus`       | `IBusIO`             | Out       | Interface to the instruction memory system for requesting and receiving instruction data.       |
| `inst.lanes` | `DecoupledVectorIO`  | Out       | 4-lane decoupled output delivering up to 4 instructions per cycle to the decoders.              |
| `branch`     | `Vec[BranchTakenIO]` | In        | Branch resolution signals from the execution pipeline indicating a change in control flow.      |
| `iflush`     | `Valid(UInt)`        | In        | Pipeline flush request, redirecting fetch to the provided PC.                                   |
| `csr`        | `CsrInIO`            | In        | Core CSR values. `csr.value(0)` provides the boot address upon exiting reset.                   |
| `fault`      | `Valid(UInt)`        | Out       | Asserted when the `IBus` returns a fault during a fetch transaction, capturing the faulting PC. |

## Fetch Reorder Buffer (FRB)

The `Fetcher` utilizes a `FetchReorderBuffer` to track outstanding transactions over the `IBus`.

- **Capacity:** 2 concurrent transactions (`maxConcurrentTx = 2`).
- **Backpressure:** When the buffer reaches its maximum capacity of 2 concurrent transactions, it de-asserts `newTx.ready`. This directly backpressures the `IBus` transaction issuance by holding `io.ibus.valid` low (`canStartFetch = io.ctrl.valid && reorderBuffer.io.newTx.ready && txidAllocator.io.alloc.valid`) and stalls the upstream `FetchControl` stage.
- **Function:** Associates `IBus` requests with a `txid` and matches responses.
- **Flushing:** On a pipeline flush or branch mispredict, the buffer marks in-flight transactions as cancelled (`nCancelled`). The responses are subsequently dropped upon arrival instead of being buffered.

## PC Generation and Pre-Decoding

The `FetchControl` block maintains the speculative PC. The PC is determined by:

1. **Reset:** Initialized via `io.csr.value(0)` (from the system boot vector).
2. **Branch/Flush:** Overridden by `io.branch` or `io.iflush`.
3. **Pre-decode:** As a fetch line is received, `FetchControl` performs a pre-decode (`PredictJump`). If an unconditional jump (JAL) or static backward branch (BXX) is detected within the fetched slots, the next PC is speculatively updated to the branch target.

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------


> **Traceability:** Generated by Gemini. Derived from upstream commit 28fdd2f4b80b1db06a4025b828807fcdc0e76f88. AI-generated/assisted; RTL is the source of truth.
