# Load Store Unit

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

> **Intended Audience:** HW Devs


![image](../../images/lsu.svg)

The Load Store Unit handles memory operations issued by the core. Functionally,
it's purpose is to translate memory instructions into transactions on the
appropriate subsystem.

## Slots

### Starvation and Blocking Behavior

The LSU currently implements a single-slot architecture without advanced starvation prevention mechanisms or fair queuing. A stalled transaction on any bus interface (particularly the external `ebus`) will physically block the **Transfer Memory** state. This head-of-line blocking prevents any subsequent memory operations from issuing until the stalled transaction completes, which may induce pipeline starvation if the external memory system experiences high latency.

### Memory Serialization and Consistency Model (QA-013)

The CoralNPU core implements a strict in-order, single-slot memory consistency model with the following guarantees:
- **Strict In-Core Serialization:** Due to the single-slot execution pipeline and the explicit lack of Load-after-Store or Store-to-Load data forwarding paths (no store buffer or bypass network), memory transactions are processed sequentially. A subsequent load instruction targeting the same address as an active preceding store must stall until the store has fully committed its writeback to the target memory.
- **AXI4 Ordering Boundaries:** External memory requests dispatched to the system interconnect via the AXI4 master interface are issued in strict program order. Because only a single transaction can be outstanding, out-of-order response hazard risks are mitigated entirely at the boundary.
- **System-Level Consistency:** The hardware does not natively enforce multicore consistency (e.g., TSO or RVWMO) with external processors or DMA masters. Explicit software synchronization (such as RISC-V `FENCE` or fence-like instructions) must be issued to establish memory barriers and synchronize state across distinct system masters.
- **Non-Speculative Execution:** Memory transactions are never issued speculatively; every transaction mapped to AXI or TCM represents a committed, non-speculative instruction.

### Unaligned Access Handling

The LSU handles unaligned scalar memory accesses by widening them and aligning them to a 16-byte cache line boundary. Specifically, unaligned half-word (`LH`, `LHU`, `SH`) and word (`LW`, `SW`, `FLOAT`) accesses have their size set to 16 bytes and their address aligned to the line boundary (`lineAlignedAddress`). Note that the internal `dbus` always operates on 16-byte aligned `targetLineAddr`, while the external `ebus` accepts a logically aligned address corresponding to the original instruction. For unaligned stores, **write masks** (`wmask`) are dynamically computed to protect adjacent data, ensuring that only the intended bytes within the 16-byte line are modified in the backing memory.

For unaligned loads, the hardware extracts the relevant bytes from the returned 16-byte cache line and shifts them to the least significant byte positions. This extraction is performed using the `Gather` hardware primitive (`ScatterGather.scala`), which populates an internal slot data buffer by selecting bytes from the returned line using their element address offsets. During the writeback stage, the `scalarLoadResult()` method (`Lsu.scala`) reconstructs the final value by concatenating the lowest entries of this buffer (e.g., `data(0)` to `data(3)`). This sequence implicitly shifts the unaligned data to the bottom of the register and applies the necessary sign-extension logic based on the instruction type. Aligned accesses proceed with their native size and alignment.

### Vector Load/Store Alignment Constraints (QA-014)

Unlike scalar implementations on some platforms that trap on misaligned accesses, the CoralNPU LSU provides transparent hardware support for unaligned vector memory accesses:
- **No Alignment Traps:** Unaligned vector loads and stores do **NOT** generate address-misaligned traps.
- **16-byte Cache Line Alignment:** Under the hood, the LSU aligns all unaligned memory transactions to 16-byte cache line boundaries (`lineAlignedAddress`).
- **Byte-Granular Masking and Gathering:**
  - For unaligned vector stores, the hardware dynamically computes byte-level write masks (`wmask`) using the `Gather` primitive in `ScatterGather.scala` to modify only the targeted elements, protecting adjacent data on the 16-byte line.
  - For unaligned vector loads, the LSU retrieves the 16-byte cache line, and the byte extraction logic selects and gathers the active elements, shifting and sign-extending them based on the Selected Element Width (`sew`) register.
- **Performance Impact:** Unaligned vector transactions that cross 16-byte cache line boundaries will be split by the LSU into multiple physical memory transactions, resulting in a proportional throughput penalty. For optimal performance, vector base addresses should be aligned to 16-byte boundaries.

The CoralNPU LSU uses a concept called _slots_ to handle memory transactions. A
slot is a data structure which manages the state of a single dispatched LSU
operation and determines what memory transaction should be performed. At its
core, there exists a table in each slot which tracks which part of the memory
operation has been completed.

For example, below is the slot table for a word-store into address 0xDEADBEEF:

| Index | Active | Address    | Data |
| ----- | ------ | ---------- | ---- |
| 0     | 1      | 0xDEADBEEF | 0x01 |
| 1     | 1      | 0xDEADBEF0 | 0x23 |
| 2     | 1      | 0xDEADBEF1 | 0x45 |
| 3     | 1      | 0xDEADBEF2 | 0x67 |
| 4     | 0      | 0xDEADBEF3 | 0x00 |

...
| n | 0 | 0xDEADBEF3 | 0x00 |

Memory transactions over TCM or AXI4 busses will read/write data in the slot
table and flip the active bits to 0 as they are made.

The typical lifetime of a slot is as follows:

1. **Idle**: An idle slot will dequeue a LsuOperation from the command queue.
   Scalar operations will move directly into **Transfer Memory** while vector
   operations will go to the **Vector Update** state.
2. **Vector Update**: For vector operations masks, addresses (for indexed ops)
   and data (for store ops) need to be received from the RvvCore. This stage is
   bypassed for scalar operations.
3. **Transfer Memory**: While there are still active entries in the slot table,
   the active entry with the lowest "index" will be selected for a memory
   transaction. A scatter/gather unit will then select all other active entries
   (although not necessarily contiguous) that will be bundled with the transaction.
   The appropriate memory bus (ibus, dbus, ebus) will then be selected and a memory
   transaction will be conducted. When there are no active entries, the slot moves
   into the next state (**Writeback**).
4. **Writeback**: Once all memory transactions are completed, the result must
   be written back to register files. Additionally, vector stores are
   "acknowledged" back to the RvvCore. Scalar and floating point scores bypass this
   stage. Once writeback is completed, the slot moves back into the
   **Vector Update** state for LMUL > 1, or the **Idle** state.

CoralNPU currently uses one "slot" in the LSU. In the future, multiple slots maybe
added to allow multiple operations to partake in the same transaction.

## Interfaces

### LSU Command Interface

The LSU has a command interface coming from the dispatch unit and register file.

| Signal Name   | Type          | Description                                                   |
| ------------- | ------------- |

--------------------------------------------------------------------------------

**Provenance & Traceability**
- **Verified As Of:** 2026-07-25
- **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7)
- **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Lsu.scala`, `hdl/chisel/src/coralnpu/scalar/ScatterGather.scala`, `hdl/chisel/src/coralnpu/Interfaces.scala`
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

> **Traceability:** Generated by Gemini. Derived from upstream commit 6a8cc54a67fb4ca7ecda116453fbdc4a97994ebf.
