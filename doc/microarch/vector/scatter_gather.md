# Vector Scatter/Gather Hardware (ScatterGather.scala)

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

> **Intended Audience:** Hardware Developers, SW/Compiler Developers

The `Scatter` and `Gather` hardware blocks are combinatorial generator primitives defined in Chisel that facilitate non-contiguous memory access. They are integrated directly into the CoralNPU Load-Store Unit (`Lsu.scala`), which coordinates the multi-cycle execution loop across memory lines.


---

## 1. Gather Hardware Primitive

The `Gather` block maps non-contiguous memory locations (from a single cache or SRAM line) back into a contiguous set of vector registers.

### 1.1 API and Port Interfaces

The `Gather` primitive is a combinatorial generator. It maps an array of indices to an array of fetched data elements.

| Port/Parameter | Direction | Type        | Description                                                           |
| :------------- | :-------- | :---------- | :-------------------------------------------------------------------- |
| `indices`      | Input     | `Vec[UInt]` | The target memory offsets for each element.                           |
| `data`         | Input     | `Vec[T]`    | Raw memory cache line data bytes (`Vec[T]`, where `T <: Data`).       |
| **Returns**    | Output    | `Vec[T]`    | Contiguous vector elements.                                           |

---

## 2. Scatter Hardware Primitive

The `Scatter` block takes a contiguous vector register of data and scatters it to non-contiguous memory locations. Its primary task is collision detection and overlap resolution within a single memory transaction.

### 2.1 API and Port Interfaces

The `Scatter` primitive takes active valid flags, target indices, and the data to write, producing the write payload, byte-enable masks, and selection tracking.

| Port/Parameter                   | Direction | Type        | Description                                                                 |
| :------------------------------- | :-------- | :---------- | :-------------------------------------------------------------------------- |
| `valid`                          | Input     | `Vec[Bool]` | Active mask indicating which vector element lanes are participating.        |
| `indices`                        | Input     | `Vec[UInt]` | Dest addresses of each element within the target memory line.               |
| `data`                           | Input     | `Vec[T]`    | Input vector register elements to scatter.                                  |
| **Return 1** (`result`)          | Output    | `Vec[T]`    | Assembled memory write payload.                                             |
| **Return 2** (`resultMask`)      | Output    | `Vec[Bool]` | Byte write-enable mask for the destination memory line.                     |
| **Return 3** (`indicesSelected`) | Output    | `Vec[Bool]` | Selection mask indicating which input elements were successfully scattered. |
---


## 3. Microarchitectural State Sequencing (LSU Interfaces)

The `Scatter` and `Gather` blocks are combinatorial, but they are driven by the sequential state machine of the Load-Store Unit (`Lsu.scala`). Because a scatter/gather instruction can span multiple cache lines, the LSU executes them incrementally over multiple clock cycles.

### 3.1 Gather Sequencing (`loadUpdate`)

When performing a vector load, the LSU tracks the pending elements using a bitmask of `active` lanes inside `LsuSlot`:

1. The LSU calculates the target physical addresses of all active elements in the slot.
2. It selects a target cache line address (`lineAddr`) and triggers a memory read.
3. Upon receiving the line data, the LSU calculates which active lanes target this line (`lineActive` mask).
4. The raw line bytes are converted into a vector and passed to `Gather(elemAddresses(), lineDataVec)`.
5. The gathered data is written into the LSU slot, and those lanes' `active` bits are cleared.
6. This cycle repeats until the slot's `active` mask is fully cleared (`0`).

### 3.2 Scatter Sequencing (`scatter` / `storeUpdate`)

For a vector store, the LSU sequences the writes line-by-line:

1. The LSU determines the target line address and invokes `slot.scatter(lineAddr)`.
2. This invokes the combinatorial `Scatter` block, passing the `lineActive` mask, address indices, and vector data.
3. `Scatter` returns the assembled `result` (write payload), the `resultMask` (write byte-enable), and `indicesSelected` (which lanes successfully wrote without collision).
4. The LSU issues a physical DBus store using the generated payload and write-enable masks.
5. The LSU then calls `storeUpdate(indicesSelected)` to clear the active status of the written lanes.
6. If any active lanes remain, the LSU moves to the next line address and repeats.

---

## 4. Physical Layout Constraints & Bounds

The hardware enforces a strict boundary limit:

- **Index Width Boundary**: Address indices passed to the Scatter/Gather blocks are restricted to a maximum width of **16 bits** (`indexWidth <= 16`).
- This constraints the maximum scatter/gather lookup space to **65,536 elements** per micro-op block.

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-22 - **Upstream Commit:** 5c2647afd951f70d6244ea06b5a8b7fa1fdf2918 - **Primary Source(s):** `hdl/chisel/src/common/ScatterGather.scala`, `hdl/chisel/src/coralnpu/scalar/Lsu.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->

> **Traceability:** Generated by Gemini. Derived from upstream commit f05a63aa421b1c7880e6fb2309e5e2c0e35607c3.
