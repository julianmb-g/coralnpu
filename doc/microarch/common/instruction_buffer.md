# Instruction buffer

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
>
> **Intended Audience:** HW Devs

## Decoupled IO interface (`feedin`)

The `InstructionBuffer` module accepts incoming instructions through a specialized `DecoupledVectorIO` interface named `feedIn`. This interface encapsulates up to `n` `DecoupledIO` streams. The interface follows the convention that the first `nValid` elements are valid for enqueueing.

- **Interface:** `DecoupledVectorIO`

- **Signals:**
  - `nReady`: Output from the buffer indicating how many elements it can currently accept (minimum of `n` or the available `nSpace` in the underlying `CircularBufferMulti`).

  - `nValid`: Input to the buffer specifying the number of valid instructions being provided.
  - `bits`: A vector of `n` instructions (`gen`).

## N-element visibility logic

The buffer exposes `n` elements at its dequeue port (`io.out`). The validity of these elements is dynamically managed:

- An element at `nIndex` is marked as `valid` if and only if `nIndex < nEnqueued` (meaning there are enough buffered instructions) AND the buffer is not currently being flushed (`!io.flush`).

- The dequeue interface does not exert backpressure. Enqueued instructions become available for dequeueing one full cycle after enqueueing.

## Verification

This hardware block is validated using standalone tests.

- [Instruction Buffer Chisel Testbench](../../../hdl/chisel/src/common/InstructionBufferTest.scala)

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/common/InstructionBuffer.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
