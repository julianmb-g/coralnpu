# Clock Gate

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

The `ClockGate` module provides synchronous clock gating capabilities for the CoralNPU IP block. It allows upstream logic to dynamically disable the clock signal to downstream functional blocks.

The module is implemented as a Chisel `BlackBox`, relying on the underlying SystemVerilog implementation (`ClockGate.sv`) to provide the physical clock gating cell (ICG) mapped to the target technology library.

## Interface

The `ClockGate` module exposes the following ports:

| Port Name | Direction | Type    | Description                                                                                                              |
| :-------- | :-------- | :------ | :----------------------------------------------------------------------------------------------------------------------- |
| `clk_i`   | Input     | `Clock` | The incoming source clock signal.                                                                                        |
| `enable`  | Input     | `Bool`  | Clock enable signal. When `1` (High), `clk_i` passes through to `clk_o`. When `0` (Low), the clock is disabled.          |
| `te`      | Input     | `Bool`  | Test Enable. Bypasses the `enable` signal during Design-for-Test (DFT) scan modes, forcing the clock on for testability. |
| `clk_o`   | Output    | `Clock` | The gated output clock signal, distributed to the targeted downstream block.                                             |

## Fine-Grained Clock Gating (Negative Space)

The `ClockGate` module provides block-level (coarse-grained) clock gating capabilities. The CoralNPU IP explicitly **lacks** support for fine-grained clock gating (e.g., at the individual flip-flop or register level) within the functional modules themselves. All clock gating is managed at the macro-block interface boundaries.

## System Integration & Clock Management

The `ClockGate` module is instantiated in top-level wrappers (e.g., `CoreAxi.scala`) to implement functional clock gating for the core pipeline.

### Clock Distribution Hierarchy

1. **Source Clock**: External `aclk` is processed by `RstSync` to generate a stable internal clock (`clk_o`).
2. **Gated Clock**: A `ClockGate` instance (`cg`) gates this internal clock to yield the Core clock.

### Functional Gating Conditions

The Core clock is enabled (`ClockGate.enable = 1`) only when active work is pending or debugging is active. The enable logic evaluates the following conditions:

- Registered interrupts (`irq`, `timer_irq`, `software_irq`).
- Debug Module requests (`haltreq`).
- Functional execution (`!core.io.wfi`) when clock gating is not disabled by CSR (`!csr.io.cg`).

### Reset Sequence Interlocking

The reset sequence is interlocked with clock gating via `RstSync`. The clock is actively held disabled during reset assertion and for a fixed delay post-deassertion to ensure clean startup transitions. See [Reset Synchronization](./rstsync.md) for sequence details.

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------

> **Provenance & Traceability** - **Verified As Of:** 2026-07-10 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/chisel/src/coralnpu/ClockGate.scala:16`, `hdl/chisel/src/coralnpu/CoreAxi.scala:55-85` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
