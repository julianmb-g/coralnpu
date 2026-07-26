<!-- Copyright 2026 Google LLC -->
<!-- Licensed under the Apache License, Version 2.0 (the "License"); -->
<!-- you may not use this file except in compliance with the License. -->
<!-- You may obtain a copy of the License at -->
<!-- http://www.apache.org/licenses/LICENSE-2.0 -->
<!-- Unless required by applicable law or agreed to in writing, software -->
<!-- distributed under the License is distributed on an "AS IS" BASIS, -->
<!-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. -->
<!-- See the License for the specific language governing permissions and -->
<!-- limitations under the License. -->

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

> **Intended Audience:** HW Devs

# System overview

The CoralNPU is a standalone IP block designed for high-performance tensor processing. It utilizes a scalar control core paired with a high-throughput vector processing engine.

## Architecture overview

The CoralNPU IP consists of several interconnected modules providing scalar and vector processing capabilities, coupled with dedicated local memory.

- **Core**: Scalar control logic (`hdl/chisel/src/coralnpu/scalar/SCore.scala`) and Vector processing (`hdl/chisel/src/coralnpu/rvv/RvvCore.scala`).
- **Memory Hierarchy**: ITCM/DTCM local SRAMs.
- **Interfaces**: AXI4 host bus interface and TLUL bridges for peripheral integration.

## Memory hierarchy

CoralNPU utilizes Tightly Coupled Memory (TCM) for low-latency access:
- **ITCM**: `0x00000000` (Instruction Tightly Coupled Memory).
- **DTCM**: `0x00010000` (Data Tightly Coupled Memory).
- **Shared SRAM**: Accessed via the `CoralNPUXbar` crossbar interconnect.

## Interfaces

The CoralNPU IP interfaces with the SoC fabric:

| Interface | Protocol | Description |
| :--- | :--- | :--- |
| Host Bus | AXI4 | Full AXI4 host master/slave with multi-beat burst support. |
| Peripheral | TLUL | TileLink-UL interface for peripheral access. |

Note: The IP natively supports full AXI4 burst capabilities. TLUL ports are exposed via `CoreTlul.scala` bridges to maintain "Implementation Reality."

## Clocking and reset (QA-017)

The CoralNPU IP is designed with a robust clocking and reset architecture to manage power consumption and ensure synchronous reset de-assertion:

### Primary clock domain
The entire NPU IP operates on a single primary clock domain (`clock` / `clk_i`). All internal registers, the scalar `SCore`, and the vector `RvvCore` execute synchronously within this domain.

### Integrated clock gating and reset synchronization
The NPU integrates the `RstSync` blackbox module (`hdl/chisel/src/coralnpu/RstSync.scala`) at critical clock boundaries:
- **Asynchronous Reset Synchronization:** The `RstSync` module synchronizes the de-assertion of the primary active-low asynchronous reset (`rstn_i`) to the rising edge of the clock (`clk_i`), outputting a safe, synchronized active-low reset (`rstn_o`). This mitigates register metastability issues upon exiting the reset state.
- **Dynamic Clock Gating:** `RstSync` incorporates an integrated clock enable (`clk_en`) input to dynamically gate/enable the clock output (`clk_o`). This enables fine-grained power management, allowing the system to gate the clock to idle modules (such as the `RvvCore` vector backend) when no operations are pending.

### Synchronization constraints
When integrating the CoralNPU IP into a larger SoC, if the host fabric (e.g., AXI host or TileLink peripheral bus) operates asynchronously to the NPU's primary clock domain, appropriate Clock Domain Crossing (CDC) synchronizers (such as standard multi-stage register synchronizer chains) must be instantiated at the interface boundaries.

## Interrupt architecture (QA-018)

The CoralNPU IP exposes three standard, dedicated active-high interrupt inputs at the top-level boundary interface (defined in `hdl/chisel/src/coralnpu/Core.scala`) to interface directly with the SoC's PLIC (Platform-Level Interrupt Controller) or CLINT (Core-Local Interruptor):

| Port Name | Type | Description | Destination |
| :--- | :---: | :--- | :--- |
| `irq` | Input (Bool) | External/Peripheral Interrupt Line. Used by external peripherals or system controllers to signal events to the NPU. | Mapped to scalar `SCore` and routed directly to the `mext` (Machine External Interrupt) bit in the standard RISC-V `mip` CSR. |
| `timer_irq` | Input (Bool) | Core-Local Timer Interrupt. Driven by the SoC system timer. | Mapped to standard Machine Timer Interrupt (`mtip` bit in `mip` CSR). |
| `software_irq` | Input (Bool) | Core-Local Software Interrupt. Used for inter-processor interrupts (IPI). | Mapped to standard Machine Software Interrupt (`msip` bit in `mip` CSR). |

These ports allow the CoralNPU's scalar core to take precise exception/interrupt traps, orchestrate task scheduling, and coordinate execution flows with the host processor.

--------------------------------------------------------------------------------

**Provenance & Traceability**
- **Verified As Of:** 2026-07-25
- **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7)
- **Primary Source(s):** `hdl/chisel/src/coralnpu/Core.scala`, `hdl/chisel/src/coralnpu/RstSync.scala`, `hdl/chisel/src/coralnpu/scalar/SCore.scala`
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
