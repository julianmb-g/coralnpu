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
# Global memory map

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

> **Intended Audience:** SW/Compiler Devs, HW Integrators

The global memory map supports a default layout and an optional "highmem" configuration for expanded SRAM. It defines addresses for internal TCMs, system memory, and peripheral CSRs.

## Core memory regions (TCM & CSR)

The architecture defines three primary internal address spaces. Base addresses depend on the `default` or `highmem` profile.

### Default configuration
- **ITCM (Instruction TCM):** Base: `0x00000000`, Size: 8 KB
- **DTCM (Data TCM):** Base: `0x00010000`, Size: 32 KB
- **Core CSRs (Peripheral Space):** Base: `0x00030000`, Size: 4 KB

### Highmem configuration
- **ITCM (Instruction TCM):** Base: `0x00000000`, Size: 1 MB (configurable up to)
- **DTCM (Data TCM):** Base: `0x00100000`, Size: 1 MB (configurable up to)
- **Core CSRs (Peripheral Space):** Base: `0x00200000`, Size: 4 KB

[Source: `hdl/chisel/src/coralnpu/Parameters.scala` | Object: `MemoryRegions`]

## Crossbar & peripheral memory map

External memories and peripheral controllers map to the system crossbar at the following addresses.

| Device Name | Base Address | Size | Description |
| :--- | :--- | :--- | :--- |
| **rom** | `0x10000000` | 32 KB | Boot ROM |
| **sram** | `0x20000000` | 4 MB | Main System SRAM |
| **clint** | `0x02000000` | 64 KB | Core Local Interruptor |
| **plic** | `0x0c000000` | 64 MB | Platform-Level Interrupt Controller |
| **uart0** | `0x40000000` | 4 KB | UART Controller 0 |
| **clk_table** | `0x40001000` | 4 KB | Clock Configuration Table |
| **uart1** | `0x40010000` | 4 KB | UART Controller 1 |
| **spi_master** | `0x40020000` | 4 KB | SPI Master Controller |
| **gpio** | `0x40030000` | 4 KB | General Purpose I/O |
| **i2c_master** | `0x40040000` | 4 KB | I2C Master Controller |
| **dma** | `0x40050000` | 4 KB | Direct Memory Access Engine |
| **spi_master_flash** | `0x40070000` | 4 KB | SPI Flash Controller |
| **ispyocto_ctrl** | `0x50000000` | 1 MB | ISP Control Interface |
| **ddr_ctrl** | `0x70000000` | 4 KB | DDR Memory Controller |
| **ddr_mem** | `0x80000000` | 2 GB | DDR Memory Range |

[Source: `hdl/chisel/src/soc/CrossbarConfig.scala` | Object: `CrossbarConfig`]

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/soc/CrossbarConfig.scala`, `hdl/chisel/src/coralnpu/Parameters.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
