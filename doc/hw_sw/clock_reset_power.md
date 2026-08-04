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

# Clocking, reset, and power management

> **Intended Audience:** HW Integrators, HW Devs

Details clock distribution, reset synchronization, and power management (clock gating).

## Clocking infrastructure

The IP operates on a single synchronous clock domain (`aclk`).

### Clock gate (`clockgate`)

`ClockGate` provides fine-grained clock distribution control. Implemented as a technology-aware blackbox for ASIC targets.

| Port | Direction | Description |
| :--- | :--- | :--- |
| `clk_i` | Input | Primary source clock (`aclk`). |
| `enable` | Input | Clock enable ('1' passthrough, '0' gated). |
| `te` | Input | Test Enable. When high, bypasses gating for ATPG/DFT. |
| `clk_o` | Output | Gated clock output. |

**Implementation Nuances:**
- **ASIC**: Maps to technology-specific Integrated Clock Gating (ICG) cells (e.g., TSMC12FFC, GF12, GF22).
- **Simulation**: Implemented as a transparent latch to prevent glitches and maintain Verilator compatibility.

[Source: `hdl/verilog/ClockGate.sv`]

## Reset management

Employs an asynchronous-assertion, synchronous-deassertion reset strategy.

### Reset synchronizer (`rstsync`)

`RstSync` synchronizes the external active-low asynchronous reset (`aresetn`) to the local clock domain.

**Operational Sequence:**
1. **Assertion**: `aresetn` goes low. `rstn_o` drives low asynchronously. Internal clock gate disables to prevent race conditions.
2. **De-assertion**: `aresetn` goes high. Internal shift registers track.
   - **Reset Release**: `rstn_o` high after 2 cycles (`RST_DELAY`).
   - **Clock Resume**: `clk_o` re-enables after 4 cycles (`CLK_DELAY + RST_DELAY`).

[Source: `hdl/verilog/RstSync.sv`]

### Reset domains

Two reset domains allow debug persistence during core resets.

| Domain | Reset Source | Scope |
| :--- | :--- | :--- |
| **Global** | `aresetn` (synchronized) | Entire IP (Host I/F, CSRs, Debug Module, Core). |
| **Core** | `csr.io.reset \| dm.io.ndmreset` | NPU Pipeline only. Preserves Debug Module state. |

[Source: `hdl/chisel/src/coralnpu/CoreAxi.scala`]

## Power management (clock gating)

Power management uses dynamic and static core clock gating.

### Gating triggers

Core clock gates when:
- **Software Forced**: The `cg` bit (Bit 1) of the `resetReg` (offset `0x0`) is set to 1.
- **Wait For Interrupt**: The core executes a `WFI` instruction, signaling it is idle.

### Wake-up logic

Clock resumes if:
- **External Interrupts**: `irq`, `timer_irq`, or `software_irq`.
- **Debug Requests**: An external debugger asserts a halt request (`dm.io.haltreq`).

**Wake-up Latency:** 1 clock cycle (`aclk`) due to single-stage external interrupt synchronization.

## Manufacturing test mode

When test enable `io.te` is high:
- **Reset Bypass**: The raw `aresetn` signal bypasses the synchronizer and is routed directly to all domains.
- **Clock Bypass**: All clock gates are forced open, ensuring full observability and controllability for ATPG.

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/verilog/ClockGate.sv`, `hdl/verilog/RstSync.sv`, `hdl/chisel/src/coralnpu/CoreAxi.scala`, `hdl/chisel/src/coralnpu/CoreAxiCSR.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
