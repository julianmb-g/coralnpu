# Control and Status Registers (CSR)

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

> While every effort is made to ensure technical accuracy, the underlying source
> code and hardware RTL implementation remain the absolute source of truth. Use
> at your own risk.

> **Intended Audience:** HW Devs

The Control and Status Registers (CSR) module manages the architectural and microarchitectural state of the Coral NPU.

## `mstatush` Register Behavior

The `mstatush` CSR (address `0x310`) is explicitly implemented in the CoralNPU RTL to maintain compatibility with upstream execution expectations.

- **Access Behavior**: It explicitly returns `0.U(32.W)` on all reads and safely ignores all writes. It does not trigger an illegal instruction exception when accessed.

[Source: hdl/chisel/src/coralnpu/scalar/Csr.scala | As of: 2026-06-06 | Commit: be666b5187c4c2ae2c1e5a28a47c3b231108c6d2]

## DbgReqOp Register Offset

The `DbgReqOp` (Debug Request Operation) register is part of the external memory-mapped debug interface.

- **Offset**: `0x808.U`

[Source: hdl/chisel/src/coralnpu/CoreAxiCSR.scala | As of: 2026-06-21]

## CoreAxiCSR Offset Constraints

The `CoreAxiCSR` module provides the external AXI4 interface to the NPU's memory-mapped control registers. It enforces strict address alignment and grouping constraints:

1. **Address Alignment**: Incoming read addresses are dynamically masked to align with the AXI4 data bus width (`readAddr & ~((p.axi2DataBytes - 1).U)`).
2. **Lane Multiplexing**: 32-bit registers (`kRegWidthBits = 32`) are packed into the wider AXI4 data bus based on their offset (`(offset % p.axi2DataBytes) / kRegWidthBytes`).
3. **Write Protection**: Registers are grouped by their aligned base address to prevent multiple writers in a single cycle.
4. **Valid Range Checking**: A read or write operation is only acknowledged (`writeResp` or `readDataValid`) if the requested address matches a defined offset in the register map.

[Source: `hdl/chisel/src/coralnpu/CoreAxiCSR.scala`]
[Citation: The memory-mapped CSR alignment and access control are explicitly enforced via Chisel `RegMap` and `RegField` definitions within `CoreAxiCSR.scala`.]

## Core Control & Memory-Mapped Registers (CoreAxiCSR)

The external memory-mapped registers of the Coral NPU are mapped over AXI4. They are grouped and aligned via `CoreCSR` and `CoreAxiCSR` modules, allowing an external host to configure, boot, and monitor the NPU.

| Offset  | Register Name  | Access | Width   | Description                                                                                                                                                               | Chisel Source Reference           |
| :------ | :------------- | :----- | :------ | :

--------------------------------------------------------------------------------

**Provenance & Traceability**
- **Verified As Of:** 2026-07-25
- **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7)
- **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Csr.scala`, `hdl/chisel/src/coralnpu/CoreAxiCSR.scala`, `hdl/chisel/src/coralnpu/scalar/FaultManager.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
