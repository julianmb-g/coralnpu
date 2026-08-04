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
# Hardware interrupt routing map

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

> **Intended Audience:** HW Integrators, SW/Compiler Devs

CoralNPU implements standard RISC-V interrupt routing using a Core Local Interruptor (CLINT) for timer/software interrupts, and a Platform-Level Interrupt Controller (PLIC) for external interrupts.

## Interrupt controllers

### Core local interruptor (CLINT)
CLINT generates standard RISC-V Machine Timer (`MTIP`) and Software (`MSIP`) Interrupts.
- **`mtip` routing:** Wired directly to the vector core's `io.timer_irq` port.
- **`msip` routing:** Wired directly to the vector core's `io.software_irq` port.

### Platform-level interrupt controller (PLIC)
PLIC aggregates and prioritizes external interrupts.
- **Input Sources:** Up to 31 external interrupts (`ext_intrs` to `io.srcs`), mapped to the top-level boundary for direct peripheral wiring (DMA, UART, SPI).
- **Priority:** Supports 8 priority levels (3-bit `priorityWidth`).
- **Output Routing:** Aggregated PLIC interrupt (`irq`) wires to vector core's `io.irq`, triggering a Machine External Interrupt (`MEIP`).

## Core trap resolution (`score` -> `CSR`)

The vector core processes interrupts via its Control and Status Unit (`csr`).
- When asserted with enable bits set in `mstatus` and `mie` CSRs, the core traps.
- `FaultManager` flushes the pipeline and redirects PC to the `mtvec` trap vector address.
- `mcause` CSR updates to reflect interrupt type (`11` External, `7` Timer, `3` Software).

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/soc/CoralNPUChiselSubsystem.scala`, `hdl/chisel/src/soc/SoCChiselConfig.scala`, `hdl/chisel/src/coralnpu/scalar/SCore.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
