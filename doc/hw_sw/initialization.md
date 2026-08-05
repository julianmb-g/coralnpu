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

# System initialization and resets

> **Intended Audience:** HW Integrators, SW/Compiler Devs

This document describes system initialization, reset synchronization, clock gating, and command completion for the CoralNPU IP block.

## Boot sequence

The CoralNPU boots from a cold power-on state using a coordinated host sequence:

1. **Power-on reset**: Host asserts active-low asynchronous system reset (`aresetn` low).

2. **Clock stabilization**: System clock (`aclk`) stabilizes while reset remains active.

3. **Reset de-assertion**: Host releases `aresetn` high. Top-level wrappers and the host interface (AXI4 slave) exit reset, allowing access to CSRs.

4. **Boot address latching**: On the first clock after `aresetn` de-assertion, the hardware captures the external boot address (`io.bootAddr`) into the `pcStart` path.

5. **Start PC override (optional)**: Host can write a new target address to `pcStartReg` (offset `0x4`).

6. **Core release**: Host clears the reset bit (Bit 0) and releases clock gating (Bit 1) in `resetReg` (offset `0x0`). This starts instruction fetches from the start PC.

## Core control registers

The `CoreCSR` block manages power-on defaults and runtime control.

### Core reset register (`resetreg`)

* **Offset**: `0x0`

* **Default**: `0x3` (core reset and clock gate active)

* **Access**: Read/Write

| Bit(s) | Name | Access | Default | Description |
| :--- | :--- | :--- | :--- | :--- |
| 0 | `reset` | R/W | `1` | Core Reset (Active High). Holds internal core in reset. |
| 1 | `cg` | R/W | `1` | Clock Gate Override (Active High). Forces core clock gated. |
| 31:2 | - | RO | `0` | Reserved. |

### Start PC register (`pcstartreg`)

* **Offset**: `0x4`

* **Default**: Latches `io.bootAddr` post-reset.

* **Access**: Read/Write (software boot vector override).

## DDR controller reset synchronization

The DDR controller reset (`ddr_rst_n`) synchronizes with `aresetn` via a reset synchronizer, de-asserting 10 cycles after the main NPU core reset to protect memory bus integrity during voltage rail stabilization.

## Reset synchronization

### Reset synchronizer (`rstsync`)

The external active-low asynchronous reset (`aresetn`) is synchronized internally to prevent race conditions:

* **Assertion**: Asserting `aresetn` low asynchronously clears `rst_delay_reg`, immediately driving `rstn_o` low and disabling the clock gate enable (`clk_en_int`) to halt `clk_o`.

* **De-assertion**: Releasing `aresetn` high shifts ones into `rst_delay_reg`. After 2 cycles (`RST_DELAY`), `rstn_o` goes high. After 4 cycles (`CLK_DELAY + RST_DELAY`), the clock gate enables to restore `clk_o`. This ensures the core exits reset while the clock is disabled, avoiding metastabilities.

### Global vs. core reset

* **Global reset (`global_reset`)**: Resets the entire IP, including host registers, Debug Module (`dm`), and Control CSRs.

* **Core-specific reset (`core_reset`)**: Sourced as the logical-OR of `csr.io.reset` and `dm.io.ndmreset`. Resets only the NPU core pipeline, leaving debug sessions active.

## Clock gating and low-power wake-up

Dynamic clock gating disables `cg.io.clk_o` during idle states to minimize dynamic power.

* **Gating criteria**: Clock gates off when the core executes a Wait-For-Interrupt (`WFI`) instruction (asserting `core.io.wfi`) or when `csr.io.cg` is set.

* **Wake-up conditions**: Clock enables if any wake-up signal is pending: external interrupt (`irq_reg`), timer interrupt (`timer_irq_reg`), software interrupt (`software_irq_reg`), or debug halt request (`dm.io.haltreq(0)`).

## Manufacturing test enable bypass

During manufacturing tests (ATPG):

* **Reset bypass**: Asserting `io.te` high routes the raw asynchronous `io.aresetn` directly to `global_reset` and `core_reset`, bypassing synchronizers.

* **Clock gate bypass**: Asserting `io.te` high forces all core clocks to run continuously, bypassing dynamic gating.

## Command completion and interrupt model

The CoralNPU does not feature a physical, dedicated hardware completion interrupt output pin on the top-level `CoreAxi` wrapper. Instead, software-hardware coordination and completion signaling are managed through memory-mapped state updates and core-driven status signals:

* **Tail pointer polling**: The hardware updates the tail pointer in tightly-coupled memory (TCM) as it consumes commands from the ring buffer. Software polls this memory-mapped tail pointer to detect command queue completions. Because TCM is dual-ported and single-cycle, polling incurs no bus arbitration latency or memory-hierarchy overhead.

* **Core-driven `wfi` signaling**: To signal task completion without CPU polling, NPU software can execute a `WFI` instruction. This asserts the top-level active-high `wfi` output pin (`io.wfi`) on the NPU IP boundary. The SoC integrator can route this `wfi` pin directly to the host interrupt controller as a completion interrupt.

* **Latency characteristics**:

  * **WFI assertion latency**: Asserting the `wfi` output pin takes exactly 1 clock cycle (`aclk`) following `WFI` instruction retirement.
  * **Clock-wake latency**: Restoring the core clock upon an interrupt wake-up event takes exactly **1 clock cycle** (`aclk`) following interrupt assertion, due to the single-stage synchronization of external interrupts before the clock gate enable.

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/coralnpu/CoreAxiCSR.scala`, `hdl/chisel/src/coralnpu/CoreAxi.scala`, `hdl/verilog/RstSync.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.


> **Traceability:** Generated by Gemini. Derived from upstream commit d9622642c63f7eba6e0c9baa7fea2188d32e28e3.