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

# System control and status registers (csrs)

> **Intended Audience:** HW Devs

This document details the internal Control and Status Register (CSR) architecture implemented within the CoralNPU scalar core.

## Internal RISC-V CSR table (ADR-015)

The hardware implements the following memory-mapped CSRs. Unmapped registers will trigger an illegal instruction fault via `CsrAddress.safe`.

| Address    | Name          | Description                                  |
| :--------- | :------------ | :------------------------------------------- |
| `0x001`    | `FFLAGS`      | Floating-Point Accrued Exceptions            |
| `0x002`    | `FRM`         | Floating-Point Dynamic Rounding Mode         |
| `0x003`    | `FCSR`        | Floating-Point Control and Status            |
| `0x008`    | `VSTART`      | Vector Start Index                           |
| `0x009`    | `VXSAT`       | Vector Fixed-Point Saturation Flag           |
| `0x00a`    | `VXRM`        | Vector Fixed-Point Rounding Mode             |
| `0x300`    | `MSTATUS`     | Machine Status Register                      |
| `0x301`    | `MISA`        | Machine ISA Register                         |
| `0x304`    | `MIE`         | Machine Interrupt Enable                     |
| `0x305`    | `MTVEC`       | Machine Trap-Vector Base-Address             |
| `0x340`    | `MSCRATCH`    | Machine Scratch Register                     |
| `0x341`    | `MEPC`        | Machine Exception Program Counter            |
| `0x342`    | `MCAUSE`      | Machine Cause Register                       |
| `0x343`    | `MTVAL`       | Machine Trap Value                           |
| `0x344`    | `MIP`         | Machine Interrupt Pending                    |
| `0xc20`    | `VL`          | Vector Length                                |
| `0xc21`    | `VTYPE`       | Vector Type                                  |
| `0xc22`    | `VLENB`       | Vector Length in Bytes (VLEN = 128 bits)     |
| `0xc23`    | `MTYPE`       | Machine Type (Custom/VME)                    |
| `0xf11`    | `MVENDORID`   | Machine Vendor ID                            |
| `0xf12`    | `MARCHID`     | Machine Architecture ID                      |
| `0xf13`    | `MIMPID`      | Machine Implementation ID                    |
| `0xf14`    | `MHARTID`     | Hardware Thread ID                           |

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Csr.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
