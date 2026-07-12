# CircularBufferMulti

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

`CircularBufferMulti` is a hardware primitive implementing a generic FIFO-style circular buffer capable of accepting and providing multiple elements concurrently per clock cycle.

## Parameters

| Parameter  | Type   | Constraints        | Description                                                               |
| :--------- | :----- | :----------------- | :------------------------------------------------------------------------ |
| `t`        | `Data` | None               | Data type of the elements stored in the buffer.                           |
| `n`        | `Int`  | Must be power of 2 | Maximum number of elements that can be enqueued or dequeued concurrently. |
| `capacity` | `Int`  | Must be power of 2 | Total element capacity of the circular buffer.                            |

## Interfaces & Ports

| Port Name   | Direction | Type/Width                     | Description                                                             |
| :---------- | :-------- | :----------------------------- | :---------------------------------------------------------------------- |
| `enqValid`  | Input     | `UInt(log2Ceil(n + 1))`        | Number of valid elements being enqueued in the current cycle (max `n`). |
| `enqData`   | Input     | `Vec(n, t)`                    | Array of `n` elements to enqueue.                                       |
| `nEnqueued` | Output    | `UInt(log2Ceil(capacity + 1))` | Current number of elements stored in the buffer.                        |
| `nSpace`    | Output    | `UInt(log2Ceil(capacity + 1))` | Available space remaining in the buffer.                                |
| `dataOut`   | Output    | `Vec(n, t)`                    | The next `n` elements available for dequeue.                            |
| `deqReady`  | Input     | `UInt(log2Ceil(n + 1))`        | Number of elements consumed/dequeued in the current cycle (max `n`).    |
| `flush`     | Input     | `Bool`                         | Synchronous clear. Resets pointers and sets `nEnqueued` to 0.           |

## Behaviors & Constraints

- **Concurrent Access**: Supports enqueueing and dequeueing on the same clock cycle, resolving the next state pointer accurately.
- **Capacity Bounds**: It is a hardware assertion violation to enqueue more items than available space (`io.enqValid <= capacity - io.nEnqueued`), or to dequeue more items than are present (`io.deqReady <= io.nEnqueued`).
- **Pointer Wrapping**: Internal read and write pointers rely on power-of-2 constraints to correctly map elements into the `buffer` backing store via left and right vector rotation.
- **Flush Semantics**: Asserting the `flush` signal resets `enqPtr`, `deqPtr`, and `nEnqueued` to `0` synchronously, effectively discarding all contents.

<!-- mdformat off -->
<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

> **Provenance & Traceability** - **Verified As Of:** 2026-07-06 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/chisel/src/common/CircularBufferMulti.scala:L17-78` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- prettier-ignore-end -->
<!-- mdformat on -->
