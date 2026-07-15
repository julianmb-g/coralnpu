# Tightly Coupled Memory (TCM128)

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

The CoralNPU utilizes a Tightly Coupled Memory (TCM) architecture for high-bandwidth, deterministic on-chip storage. The base implementation is the `TCM128` module.

## Overview

Per ADR-071, the CoralNPU features Tightly Coupled Memories (TCMs) to provide low-latency, deterministic access for instructions (ITCM) and data (DTCM). The TCMs are designed for single-cycle read timing, providing high-bandwidth 128-bit native paths directly to the core's instruction and data buses without incurring the latency penalties of standard caches or AXI4 bus arbitration.

## TCM Implementation and Structure

The core instantiates two distinct TCM modules within the `CoreAxi` wrapper:

- **ITCM**: Connected to the Instruction Bus (`ibus`), acting as the fast-path for instruction fetch.
- **DTCM**: Connected to the Data Bus (`dbus`), acting as the fast-path for memory access.

Both TCMs are implemented using the `TCM128` wrapper, which aggregates individual `Sram_Nx128` and underlying `SramBlock` black-box primitives.

- **Data Width**: 128 bits per bank.
- **Single-Cycle Read Timing**: The `SRAM` wrapper implements strict single-cycle read timing. When an un-conflicted read request is issued (`issueRead`), the data is returned on the next clock cycle (`readIssued := issueRead`). This strict timing contract is critical for compiler engineers modeling instruction issue rates and load-use delays.
- **Dual 128-bit Paths**: By maintaining separate ITCM and DTCM modules, the core guarantees dual 128-bit parallel access paths for instructions and data, bypassing the shared AXI4 interconnect bottleneck.

## Architecture

The `TCM128` module is constructed around a 128-bit wide data path and acts as a wrapper around the underlying SRAM macro (`Sram_Nx128`).

### Parameterization

The module accepts the following configuration parameters at instantiation:

| Parameter          | Type  | Description                                         |
| :----------------- | :---- | :-------------------------------------------------- |
| `tcmSizeBytes`     | `Int` | Total size of the TCM instance in bytes.            |
| `tcmSubEntryWidth` | `Int` | Width of the sub-entries mapped to the I/O vectors. |
| `globalBaseAddr`   | `Int` | Base address offset for the memory region.          |

### Data Path Width

The internal data path width is hardcoded to **128 bits** (`val tcmWidth = 128`). The number of entries in the underlying SRAM is dynamically calculated based on the requested byte size:
`tcmEntries = tcmSizeBytes / 16` (where 16 is `tcmWidth / 8`).

### Interface Signals

The interface utilizes structured Chisel `Vec` bundles for sub-entry routing.

| Signal   | Direction | Width                              | Description                       |
| :------- | :-------- | :--------------------------------- | :-------------------------------- |
| `addr`   | Input     | `log2Ceil(tcmEntries)`             | Row address for SRAM access.      |
| `enable` | Input     | 1                                  | Memory enable signal.             |
| `write`  | Input     | 1                                  | Write enable signal.              |
| `wdata`  | Input     | `tcmSubEntries * tcmSubEntryWidth` | Write data vector.                |
| `wmask`  | Input     | `tcmSubEntries`                    | Write byte/sub-entry mask vector. |
| `rdata`  | Output    | `tcmSubEntries * tcmSubEntryWidth` | Read data vector.                 |

### Bit Reversal & Alignment

The `TCM128` module performs explicit vector reversal (`io.wdata.reverse`, `io.wmask.reverse`) before passing the signals down to the `Sram_Nx128` macro. On reads, `sram.io.rdata` is cast back to a `Vec` and reversed again to maintain lane alignment with the architectural interface.

## Arbitration and Access

TCMs are arbitrated using the `FabricArbiter` (`Fabric.scala`) within the `CoreAxi` wrapper. For both ITCM and DTCM, the arbitration hierarchy is fixed-priority (lower-indexed ports take precedence):

1. **Port 0 (Core Native Path)**: Receives highest priority. For ITCM, this is the `ibus`; for DTCM, this is the `dbus`.
2. **Port 1 (AXI4 Slave / System)**: Allows external hosts (via AXI4) to read and write to the TCMs for program loading or DMA.
3. **Port 2 (Debug Module)**: Allows the Debug Module to inspect or alter memory state.

This arbitration scheme ensures that the core is never starved by background DMA operations, though high AXI4 traffic may be delayed.

### LSU Internal Bus Arbitration

Accesses to the TCM via the LSU (Load/Store Unit) are subject to internal bus selection. The LSU can issue requests to the I-Bus (for ITCM loads), D-Bus (for DTCM accesses), or E-Bus (for external accesses). These accesses are mutually exclusive per cycle from the LSU's perspective. The RTL enforces this constraint mathematically via `assert(PopCount(Seq(ibusFired, dbusFired, ebusFired)) <= 1.U)`.

## Sram_Nx128 Generation

The `Sram_Nx128` generator optimizes physical RAM footprint by cascading the largest available SRAM primitives (`2048x128`, `512x128`, or `128x128`) based on the total required capacity (`tcmEntries`). It manages read multiplexing across sub-modules using `MuxLookup` based on the upper address bits.

<!-- mdformat off -->

<!-- prettier-ignore-start -->

## Note on VRGATHER Execution

`VRGATHER` is a vector permutation instruction implemented in the Permutation/RDT unit (`hdl/verilog/rvv/design/rvv_backend_pmtrdt_unit_permutation.sv:310-336`) and not a memory instruction accessing TCM. It performs element-wise vector permutation based on an index vector. Permutation design and details for `VRGATHER` can be found in `doc/microarch/vector/rvvalu.md`.

--------------------------------------------------------------------------------

<!-- prettier-ignore-end -->

**Provenance & Traceability** - **Verified As Of:** 2026-07-10 - **Upstream Commit:** c9d3cd8816886ced4a935722205fd47aeb72eed9 - **Primary Source(s):** `hdl/chisel/src/coralnpu/TCM.scala`, `hdl/chisel/src/coralnpu/scalar/Lsu.scala`, `hdl/chisel/src/coralnpu/CoreAxi.scala`, `hdl/chisel/src/coralnpu/Fabric.scala`, `hdl/chisel/src/coralnpu/SramNx128.scala`, `hdl/verilog/rvv/design/rvv_backend_pmtrdt_unit_permutation.sv:310-336` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->

> **Traceability:** Generated by Gemini. Derived from upstream commit 9a1e82634c2b0f3d42310f89cd1484d8f3302ec9.
