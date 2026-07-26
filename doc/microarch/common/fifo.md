# Fifo Hardware Primitive

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

> **Intended Audience:** HW Devs, SW/Compiler Devs

## Overview

The `Fifo` primitive (defined in `hdl/chisel/src/common/Fifo.scala`) is the fundamental decoupled queue abstraction utilized throughout the CoralNPU pipeline. It implements an N-1 internal queue coupled with a registered output stage.

## Architecture

The FIFO employs a dual-stage architecture:

- **Internal Memory Array:** An `m` depth memory array where `m = n - 1`.
- **Output Stage:** A registered output stage that holds the head element, ensuring that the critical path for valid data output is isolated from the memory read path.

### Pass-Through Behavior

- **`passReady = false` (Default):** The FIFO operates with a strict 1-cycle latency. The input `ready` signal is exclusively dependent on the internal buffer capacity (`wready`).
- **`passReady = true`:** The FIFO allows a combinatorial pass-through path. The input `ready` signal becomes a logical OR of the internal capacity (`wready`) and the downstream consumer's `ready` signal (`io.out.ready`).

## Interfaces

| Signal     | Direction | Type                    | Description                                                |
| :--------- | :-------- | :---------------------- | :

--------------------------------------------------------------------------------

**Provenance & Traceability**
- **Verified As Of:** 2026-07-25
- **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7)
- **Primary Source(s):** `hdl/chisel/src/common/Fifo.scala:L27-117` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
