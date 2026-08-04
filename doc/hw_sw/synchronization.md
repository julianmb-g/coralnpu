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

# Synchronization (hardware fences)

> **Intended Audience:** SW/Compiler Devs

## Overview

CoralNPU implements standard RISC-V memory synchronization (`FENCE`) and instruction cache invalidation (`FENCE.I`).

## Hardware fence implementation

Decoder logic identifies synchronization primitives and routes them to the pipeline.

| Instruction | Function | Pipeline Interaction |
| :--- | :--- | :--- |
| `FENCE` | Memory ordering fence | LSU / Fetch synchronization |
| `FENCE.I` | Instruction cache synchronization | LSU / Fetch flush (`iflush`) |

### Pipeline interaction
Synchronization stalls the pipeline or triggers flushes. `FENCE.I` interacts with LSU and Fetch to invalidate/flush instruction cache, ensuring memory consistency.

[Source: `hdl/chisel/src/coralnpu/scalar/Decode.scala` (Instruction identification), `hdl/chisel/src/coralnpu/scalar/SCore.scala` (Pipeline flush interaction)]

## Atomic operations

CoralNPU has no support for AMO (Atomic Memory Operations) instructions. Manage synchronization via memory fences and software protocols.

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Decode.scala`, `hdl/chisel/src/coralnpu/scalar/SCore.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
