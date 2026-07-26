# Memory Hierarchy

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

> **Intended Audience:** HW Devs, HW Integrators

The CoralNPU memory subsystem is explicitly designed to ensure deterministic execution, prioritizing Tightly Coupled Memory (TCM) and direct AXI4 memory access over traditional caching structures.

## Overview

The memory hierarchy consists of the following primary components:

1.  **Tightly Coupled Memory (TCM)**: On-chip SRAM dedicated to the NPU core for low-latency instruction and data access. It is split into ITCM (Instruction) and DTCM (Data).
2.  **Shared SRAM**: An additional on-chip SRAM block accessible via the AXI4 crossbar.
3.  **External Memory (AXI4)**: Access to main system memory or external peripherals is provided through the `CoreAxi` host interfaces (`IBus2Axi` and `DBus2Axi`).

## Tightly Coupled Memory (TCM)

The TCM provides single-cycle access for the NPU core.

- **ITCM**: Mapped to base address `0x00000000` (default 8KB). Used for instruction storage.
- **DTCM**: Mapped to base address `0x00010000` (default 32KB). Used for data storage.
- **Arbitration**: Access to the TCM is arbitrated by the internal `Fabric` module, handling requests from the NPU core and external debug/host interfaces.

[Source: `hdl/chisel/src/coralnpu/TCM.scala`]

## External Memory Interfaces

When an address falls outside the TCM ranges, the request is forwarded to the external AXI4 interfaces:

- **IBus2Axi**: Bridges the internal instruction fetch bus to an AXI4 read-only host interface.
- **DBus2Axi**: Bridges the internal data bus (LSU) to an AXI4 read/write host interface.

These interfaces connect to the top-level `CoralNPUXbar` crossbar, which routes the transactions to the Shared SRAM or the external SoC memory system.

[Source: `hdl/chisel/src/coralnpu/CoreAxi.scala`]

## Architectural Exclusions

Per architectural decisions (ADR-033), the L1 Instruction Cache (`L1ICache`) and L1 Data Cache (`L1DCache`) are explicitly excluded from the standard memory hierarchy documentation.

<!-- mdformat off -->

<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

**Provenance & Traceability**
- **Verified As Of:** 2026-07-25
- **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7)
- **Primary Source(s):** `hdl/chisel/src/coralnpu/TCM.scala`, `hdl/chisel/src/coralnpu/CoreAxi.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
