# Index Allocator (IndexAllocator)

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

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

The `IndexAllocator` is a parameterized hardware block designed to dynamically allocate and free unique transaction/resource indices (IDs). Within the CoralNPU pipeline, transaction tracking requires a unique index. `IndexAllocator` manages these indices in a shifting vector configuration.

> **Intended Audience:** Hardware Developers, Software & Compiler Engineers

## Architectural Function & Behavior

The `IndexAllocator` maintains an active pool of unique indices from `0` to `capacity - 1`.

- **Initialization:** Upon reset, the allocator registers are pre-loaded sequentially with values `0` through `capacity - 1`. The count of available elements (`nAvail`) is set to `capacity`, and the allocator is marked as non-empty.
- **Allocation (`alloc`):** When an allocation is requested (`alloc.valid` and `alloc.ready` are asserted), the allocator pops the index at the head of the list (`regs(0)`) and shifts the remaining valid indices down by one position.
- **Deallocation (`free`):** When an index is freed (`free.valid` is asserted), the returned index is appended to the tail of the active list (`regs(nAvail)`).
- **Simultaneous Alloc & Free:** If an allocation and a deallocation occur in the same cycle:
  - If the allocator is non-empty, the index at `regs(0)` is allocated, and the newly freed index is written directly to `regs(0)`, bypassing the shift operation.
  - If the allocator is completely empty (`nAvail == 0`), the incoming free index is bypassed directly to the allocation interface.

---

## Hardware Interfaces

The module interfaces are parameterized by the maximum transaction `capacity`.

| Port Name        | Direction | Type  | Width                | Description                                                                 |
| :--------------- | :-------- | :---- | :------------------- | :-------------------------------------------------------------------------- |
| `clock`          | Input     | Clock | 1                    | Global system clock.                                                        |
| `reset`          | Input     | Reset | 1                    | Synchronous active-high reset.                                              |
| `io.alloc.valid` | Output    | Bool  | 1                    | Indicates that an index is available for allocation.                        |
| `io.alloc.ready` | Input     | Ready | 1                    | Downstream ready signal to accept the allocated index.                      |
| `io.alloc.bits`  | Output    | UInt  | `log2Ceil(capacity)` | The unique allocated transaction ID.                                        |
| `io.free.valid`  | Input     | Bool  | 1                    | Indicates that a transaction has completed and its index is being returned. |
| `io.free.ready`  | Output    | Bool  | 1                    | Ready signal to accept the deallocation request.                            |
| `io.free.bits`   | Input     | UInt  | `log2Ceil(capacity)` | The transaction ID being returned to the pool.                              |

---

## State Machine and Internal Shifting Logic

The allocator state is represented by an internal shifting list of registers (`regs`) containing the pool of available unique indices, a counter tracking available indices (`nAvail`), and an active-high non-empty flag (`isNonEmpty`).

### Active Register Transition Table

For an allocator with `capacity` indices, the internal state shifts according to the firing of `alloc` and `free` handshakes:

| `io.alloc.fire` | `io.free.fire` | `isNonEmpty` | Transition Behavior                                                                                                                      |
| :-------------: | :------------: | :----------: | :--------------------------------------------------------------------------------------------------------------------------------------- |
|       `1`       |      `0`       |     `1`      | **Shift Left / Pop:** Yields `regs(0)`. All valid registers are shifted down by one position. Decrements `nAvail` by 1.                  |
|       `0`       |      `1`       |     `-`      | **Append / Push:** Freed index is written to `regs(nAvail)`. Increments `nAvail` by 1.                                                   |
|       `1`       |      `1`       |     `1`      | **Direct Overwrite:** Yields `regs(0)` and overwrites `regs(0)` with `io.free.bits`. Shifting is bypassed. `nAvail` remains unchanged.   |
|       `1`       |      `1`       |     `0`      | **Bypass Mode:** No indices are available in registers. `io.free.bits` is bypassed directly to `io.alloc.bits`. Registers are unchanged. |

---

## Edge Cases and Backpressure Handling

### Deallocation Backpressure (Full Allocator)

The allocator asserts backpressure on the `free` interface when it is full (`nAvail == capacity`). Under this condition, no indices are outstanding, and any incoming free request represents an invalid/illegal transaction state.

- **Behavior:** `io.free.ready` is driven by `!state.valids(capacity - 1)`. When `nAvail == capacity`, `valids(capacity - 1)` is true, driving `io.free.ready` to low.
- **Hardware Assertion:** An assertion ensures that no deallocation can be valid unless the ready line is high: `assert(!io.free.valid | io.free.ready)`.

### Uniqueness and Duplicate Free Assertion

The `IndexAllocator` enforces that every element in the active pool remains unique.

- **Uniqueness Check:** The hardware evaluates every slot in parallel to detect if the incoming `free.bits` matches an already available index.
- **Assertion:** If any match is detected, a hardware simulation assertion is triggered, stopping execution.

<!-- mdformat off -->

<!-- prettier-ignore -->
--------------------------------------------------------------------------------

<!-- prettier-ignore -->
**Provenance & Traceability** - **Verified As Of:** 2026-07-06 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/chisel/src/common/IndexAllocator.scala:L36-120` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
> **Traceability:** Generated by Gemini. Derived from upstream commit 7d6e35cff17732cad8f99b866333423ea1f4d750.
