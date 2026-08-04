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

# SRAM wrappers

> **Intended Audience:** HW Devs

The CoralNPU IP encapsulates standard memory macros within generic Chisel wrappers. These wrappers provide unified interfaces and handle the translation of internal interconnect protocols (like the Internal Memory Fabric) into physical SRAM signals (read/write enables, addresses, and byte masks).

## Core wrappers overview

The SRAM abstraction is partitioned into three hierarchical layers to separate the interface protocol logic from the physical macro instantiations:

1. **`SRAM` (Protocol Adapter)**: Translates the internal `FabricIO` request/response protocol into a raw memory interface (`SRAMIO`).

2. **`Sram_Nx128` (Parameterizable Macro Array)**: Stitches together multiple physical SRAM blocks to achieve an arbitrary depth.

3. **`SramBlock` (Physical Blackbox Wrapper)**: The lowest-level wrapper instantiating the simulated or synthesized blackbox Verilog model (`Sram.v`).

## Interfaces

### SRAMIO (raw memory interface)

The `SRAMIO` bundle defines the fundamental signals required by the physical memory macros.

| Signal      | Direction | Width              | Description                                               |
| :---------- | :-------- | :----------------- | :-------------------------------------------------------- |
| `address`   | Output    | `sramAddressWidth` | Memory access address                                     |
| `enable`    | Output    | `1`                | Memory enable / chip select                               |
| `isWrite`   | Output    | `1`                | Write enable signal (1 = Write, 0 = Read)                 |
| `readData`  | Input     | `axi2DataBits`     | Data read from the physical memory                        |
| `writeData` | Output    | `axi2DataBits`     | Data to be written to the physical memory                 |
| `mask`      | Output    | `axi2DataBits/8`   | Byte-level write enable mask                              |

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/coralnpu/SRAM.scala:L31`, `hdl/chisel/src/coralnpu/Sram.scala:L16`, `hdl/chisel/src/coralnpu/SramNx128.scala:L16` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
