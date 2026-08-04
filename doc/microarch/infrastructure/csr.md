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

# Control and status registers (CSR)

> **Intended Audience:** HW Devs

The Control and Status Registers (CSR) module manages the architectural and microarchitectural state of the Coral NPU.

## `mstatush` register behavior

The `mstatush` CSR (address `0x310`) is explicitly implemented in the CoralNPU RTL to maintain compatibility with upstream execution expectations.

- **Access Behavior**: It explicitly returns `0` (32-bit zero) on all reads and safely ignores all writes. It does not trigger an illegal instruction exception when accessed.

[Source: `hdl/chisel/src/coralnpu/scalar/Csr.scala`]

## Dbgreqop register offset

The `DbgReqOp` (Debug Request Operation) register is part of the external memory-mapped debug interface.

- **Offset**: `0x808.U`

[Source: `hdl/chisel/src/coralnpu/CoreAxiCSR.scala`]

## Coreaxicsr offset constraints

The `CoreAxiCSR` module provides the external AXI4 interface to the NPU's memory-mapped control registers. It enforces strict address alignment and grouping constraints:

1. **Address Alignment**: Incoming read addresses are dynamically masked to align with the AXI4 data bus width (`readAddr & ~(p.axi2DataBytes - 1)`).

2. **Lane Multiplexing**: 32-bit registers (`kRegWidthBits = 32`) are packed into the wider AXI4 data bus based on their offset (`(offset % p.axi2DataBytes) / kRegWidthBytes`).

3. **Write Protection**: Registers are grouped by their aligned base address to prevent multiple writers in a single cycle.

4. **Valid Range Checking**: A read or write operation is only acknowledged (`writeResp` or `readDataValid`) if the requested address matches a defined offset in the register map.

[Source: `hdl/chisel/src/coralnpu/CoreAxiCSR.scala`]
[Citation: The memory-mapped CSR alignment and access control are explicitly enforced via Chisel `RegMap` and `RegField` definitions within `CoreAxiCSR.scala`.]

## Core control & memory-mapped registers (coreaxicsr)

The external memory-mapped registers of the Coral NPU are mapped over AXI4. They are grouped and aligned via `CoreCSR` and `CoreAxiCSR` modules, allowing an external host to configure, boot, and monitor the NPU.

| Offset  | Register Name  | Access | Reset   | Width   | Description                                                                                                                                                               | Chisel Source Reference           |
| :------ | :------------- | :----- | :------ | :------ | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | :-------------------------------- |
| `0x000` | `reset`        | R/W    | `0x003` | 32-bit  | Reset register. Bit 0: Core Reset (Active High). Bit 1: Clock Gate (Active High).                                                                                         | `CoreAxiCSR.scala: coreRegMap`    |
| `0x004` | `pcStart`      | R/W    | `0x000` | 32-bit  | Boot address / Program Counter Start address.                                                                                                                             | `CoreAxiCSR.scala: coreRegMap`    |
| `0x008` | `status`       | R/O    | `0x000` | 32-bit  | Core status register. Includes `fault` and `halted` bits.                                                                                                                 | `CoreAxiCSR.scala: coreRegMap`    |
| `0x100` | `coralnpu_csr` | R/O    | `0x000` | 32-bit  | Base address for CoralNPU's internal CSRs mapped via AXI.                                                                                                                 | `CoreAxiCSR.scala: csrRegMap`     |
| `0x800` | `DbgReqAddr`   | R/W    | `0x000` | 32-bit  | Debug Request Address.                                                                                                                                                    | `CoreAxiCSR.scala: debugReadMap`  |
| `0x804` | `DbgReqData`   | R/W    | `0x000` | 32-bit  | Debug Request Data.                                                                                                                                                       | `CoreAxiCSR.scala: debugReadMap`  |
| `0x808` | `DbgReqOp`     | R/W    | `0x000` | 32-bit  | Debug Request Operation.                                                                                                                                                  | `CoreAxiCSR.scala: debugReadMap`  |
| `0x80C` | `DbgRspData`   | R/O    | `0x000` | 32-bit  | Debug Response Data.                                                                                                                                                      | `CoreAxiCSR.scala: debugReadMap`  |
| `0x810` | `DbgRspOp`     | R/O    | `0x000` | 32-bit  | Debug Response Operation.                                                                                                                                                 | `CoreAxiCSR.scala: debugReadMap`  |
| `0x814` | `DbgStatus`    | R/O    | `0x000` | 32-bit  | Debug Status (queue valid, req ready).                                                                                                                                    | `CoreAxiCSR.scala: debugReadMap`  |

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Csr.scala`, `hdl/chisel/src/coralnpu/CoreAxiCSR.scala`, `hdl/chisel/src/coralnpu/scalar/FaultManager.scala`, `hdl/chisel/src/coralnpu/CoreAxiCSRTest.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
