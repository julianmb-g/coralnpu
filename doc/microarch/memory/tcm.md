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

# Tightly coupled memory (TCM128)

> **Intended Audience:** HW Devs

The CoralNPU utilizes a Tightly Coupled Memory (TCM) architecture for high-bandwidth, deterministic on-chip storage. The base implementation is the `TCM128` module.

## Overview

Per ADR-071, the CoralNPU features Tightly Coupled Memories (TCMs) to provide low-latency, deterministic access for instructions (ITCM) and data (DTCM). The TCMs are designed for single-cycle read timing, providing high-bandwidth 128-bit native paths directly to the core's instruction and data buses without incurring the latency penalties of standard caches or AXI4 bus arbitration.

## Tcm implementation and structure

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

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `tcmSizeBytes` | Int | Total size of the TCM in bytes. |
| `tcmSubEntryWidth` | Int | Width of each sub-entry in bits. |
| `globalBaseAddr` | Int | Global base address for the TCM instance. |

### Ports

| Port Name | Direction | Type | Description |
| :--- | :--- | :--- | :--- |
| `addr` | Input | Integer | Memory address (word-aligned). |
| `enable` | Input | Boolean | Memory enable signal. |
| `write` | Input | Boolean | Write enable signal. |
| `wdata` | Input | Vector | Data to be written (128-bit split into sub-entries). |
| `wmask` | Input | Vector | Write mask per sub-entry. |
| `rdata` | Output | Vector | Read data output (128-bit split into sub-entries). |

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/coralnpu/TCM.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
