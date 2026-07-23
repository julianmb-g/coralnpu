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

> **Intended Audience:** Hardware Developers


![image](../../images/lsu.svg)

The Load Store Unit handles memory operations issued by the core. Functionally,
it's purpose is to translate memory instructions into transactions on the
appropriate subsystem.

## Slots

### Starvation and Blocking Behavior

The LSU currently implements a single-slot architecture without advanced starvation prevention mechanisms or fair queuing. A stalled transaction on any bus interface (particularly the external `ebus`) will physically block the **Transfer Memory** state. This head-of-line blocking prevents any subsequent memory operations from issuing until the stalled transaction completes, which may induce pipeline starvation if the external memory system experiences high latency.

### Memory Serialization and Forwarding (Negative Space)

The CoralNPU LSU explicitly **lacks** Load-after-Store or Store-to-Load data forwarding paths. There is no store buffer or bypass network within the LSU architecture. As a result, all memory operations are strictly serialized. A load that depends on a preceding store to the same memory address must wait for the store to completely execute and write to the backing memory before the load can be safely issued.

### Unaligned Access Handling

The LSU handles unaligned scalar memory accesses by widening them and aligning them to a 16-byte cache line boundary. Specifically, unaligned half-word (`LH`, `LHU`, `SH`) and word (`LW`, `SW`, `FLOAT`) accesses have their size set to 16 bytes and their address aligned to the line boundary (`lineAlignedAddress`). Note that the internal `dbus` always operates on 16-byte aligned `targetLineAddr`, while the external `ebus` accepts a logically aligned address corresponding to the original instruction. For unaligned stores, **write masks** (`wmask`) are dynamically computed to protect adjacent data, ensuring that only the intended bytes within the 16-byte line are modified in the backing memory.

For unaligned loads, the hardware extracts the relevant bytes from the returned 16-byte cache line and shifts them to the least significant byte positions. This extraction is performed using the `Gather` hardware primitive (`ScatterGather.scala`), which populates an internal slot data buffer by selecting bytes from the returned line using their element address offsets. During the writeback stage, the `scalarLoadResult()` method (`Lsu.scala`) reconstructs the final value by concatenating the lowest entries of this buffer (e.g., `data(0)` to `data(3)`). This sequence implicitly shifts the unaligned data to the bottom of the register and applies the necessary sign-extension logic based on the instruction type. Aligned accesses proceed with their native size and alignment.

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
| ------------- | ------------- | ------------------------------------------------------------- |
| req.valid     | Bool          | If the LSU command is valid.                                  |
| req.op        |               | The LSU operation to execute.                                 |
| req.addr      | UInt(5)       | The RegFile address to write the result to for loads.         |
| req.pc        | UInt(32)      | The PC of the LSU instruction. Use for fault reporting.       |
| req.elemWidth | UInt(32)      | Used in the RVV only. The EEW for strided loads.              |
| req.ready     | Bool (output) | If the command is accepted. Used in a ready-valid hand-shake. |

### Bus Interfaces

| Signal Name | Type              | Description                                                       |
| ----------- | ----------------- | ----------------------------------------------------------------- |
| ibus.valid  | Bool              | If the ibus transaction is valid.                                 |
| ibus.addr   | UInt(32)          | The address of the ibus transaction.                              |
| ibus.rdata  | UInt(128) (input) | The data read from the ibus. Arrives one cycle after hand-shake.  |
| ibus.ready  | Bool (input)      | If the transaction is accepted. Used in a ready-valid hand-shake. |

| Signal Name | Type              | Description                                                       |
| ----------- | ----------------- | ----------------------------------------------------------------- |
| dbus.valid  | Bool              | If the dbus transaction is valid.                                 |
| dbus.addr   | UInt(32)          | The address of the dbus transaction.                              |
| dbus.adrx   | UInt(32)          | The 16-byte aligned line address.                                 |
| dbus.size   | UInt(5)           | The size of this transaction in bytes.                            |
| dbus.pc     | UInt(32)          | The PC of the LSU instruction. Use for fault reporting.           |
| dbus.rdata  | UInt(128) (input) | The data read from the dbus. Arrives one cycle after hand-shake.  |
| dbus.wdata  | UInt(128)         | The data to write from the dbus.                                  |
| dbus.wmask  | UInt(16)          | A byte write mask for this transaction.                           |
| dbus.ready  | Bool (input)      | If the transaction is accepted. Used in a ready-valid hand-shake. |

| Signal Name      | Type              | Description                                                       |
| ---------------- | ----------------- | ----------------------------------------------------------------- |
| ebus.valid       | Bool              | If the ebus transaction is valid.                                 |
| ebus.addr        | UInt(32)          | The address of the ebus transaction.                              |
| ebus.adrx        | UInt(32)          | The 16-byte aligned line address.                                 |
| ebus.size        | UInt(5)           | The size of this transaction in bytes.                            |
| ebus.pc          | UInt(32)          | The PC of the LSU instruction. Use for fault reporting.           |
| ebus.rdata       | UInt(128) (input) | The data read from the ebus. Arrives one cycle after hand-shake.  |
| ebus.wdata       | UInt(128)         | The data to write from the ebus.                                  |
| ebus.wmask       | UInt(16)          | A byte write mask for this transaction.                           |
| ebus.ready       | Bool (input)      | If the transaction is accepted. Used in a ready-valid hand-shake. |
| ebus.fault.valid | Bool (input)      | Raised if a fault occurs on the external bus.                     |
| ebus.fault.write | Bool (input)      | If the fault occured on a write operation.                        |
| ebus.fault.addr  | Bool (input)      | The address of the memory transaction when the fault occurred.    |
| ebus.fault.epc   | Bool (input)      | The PC of the instruction that triggered the fault.               |
| ebus.internal    | Bool              | Not used.                                                         |

### Writeback Interfaces

| Signal Name | Type     | Description                                              |
| ----------- | -------- | -------------------------------------------------------- |
| rd.valid    | Bool     | If the writeback to the scalar regfile is valid.         |
| rd.addr     | UInt(5)  | The address of the scalar register file to writeback to. |
| rd.data     | UInt(32) | The data to write to the scalar register file.           |

| Signal Name  | Type     | Description                                                      |
| ------------ | -------- | ---------------------------------------------------------------- |
| rd_flt.valid | Bool     | If the writeback to the floating point regfile is valid.         |
| rd_flt.addr  | UInt(5)  | The address of the floating point register file to writeback to. |
| rd_flt.data  | UInt(32) | The data to write to the floating point register file.           |

### RVV Interfaces

For the RVVCore, the LSU contains the following interfaces:

| Signal Name            | Type         | Description                                                       |
| ---------------------- | ------------ | ----------------------------------------------------------------- |
| rvv2lsu.valid          | Bool         | If the rvv2lsu transaction is valid.                              |
| rvv2lsu.idx.valid      | Bool         | If there is valid index data from the vector register file.       |
| rvv2lsu.idx.addr       | UInt(5)      | The address of the indices from the vector register file.         |
| rvv2lsu.idx.data       | UInt(128)    | The indices from the vector register file index.                  |
| rvv2lsu.vregfile.valid | UInt(128)    | If there is valid data from the vector register file.             |
| rvv2lsu.vregfile.addr  | UInt(5)      | The address of the data from the vector register file.            |
| rvv2lsu.vregfile.data  | UInt(128)    | The vector data to write back for this operation.                 |
| rvv2lsu.mask           | UInt(16)     | A byte activity mask for this transaction.                        |
| rvv2lsu.ready          | Bool (input) | If the transaction is accepted. Used in a ready-valid hand-shake. |

| Signal Name   | Type         | Description                                                       |
| ------------- | ------------ | ----------------------------------------------------------------- |
| lsu2rvv.valid | Bool         | If the lsu2rvv transaction is valid.                              |
| lsu2rvv.addr  | UInt(5)      | The destination vector register file index.                       |
| lsu2rvv.data  | UInt(128)    | The vector data to write back for load operations.                |
| lsu2rvv.last  | Bool         | If this transaction is a store or not.                            |
| lsu2rvv.ready | Bool (input) | If the transaction is accepted. Used in a ready-valid hand-shake. |

### Auxiliary Interfaces

| Signal Name   | Type    | Description                                                        |
| ------------- | ------- | ------------------------------------------------------------------ |
| busPort       | Flipped | Read interface to the scalar register file for store data.         |
| busPort_flt   | Flipped | Read interface to the floating-point register file for store data. |
| flush         | Output  | Issues flush/fence instructions to the cache and pipeline.         |
| fault         | Valid   | Outputs fault information (address, EPC) on memory errors.         |
| vldst         | Output  | Indicates if a vector load/store is active.                        |
| rvvState      | Input   | Vector configuration state (vtype, vl) for execution context.      |
| queueCapacity | Output  | Remaining command queue capacity.                                  |
| active        | Output  | Indicates if the LSU is currently processing any transaction.      |
| storeComplete | Output  | Signals when a store operation successfully completes.             |

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-22 - **Upstream Commit:** 8ba6f4108901602e14e28345b4bd009e6f3b6897 - **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Lsu.scala`, `hdl/chisel/src/coralnpu/Interfaces.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
