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

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model.
> While every effort is made to ensure technical accuracy, the underlying source
> code and hardware RTL implementation remain the absolute source of truth. Use
> at your own risk.

> **Intended Audience:** Hardware Developers

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
| :------ | :------------- | :----- | :------ | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | :-------------------------------- |
| `0x000` | `resetReg`     | R/W    | 32 bits | NPU Reset and Clock Gate Control.<br>• **Bit 0**: `reset` (Active High reset signal to the NPU core)<br>• **Bit 1**: `cg` (Clock Gate Control - Active High clock gating) | `CoreAxiCSR.scala` (`resetReg`)   |
| `0x004` | `pcStartReg`   | R/W    | 32 bits | Program Counter Start Address. Configures the fetch starting address. captures `bootAddr` on reset.                                                                       | `CoreAxiCSR.scala` (`pcStartReg`) |
| `0x008` | `statusReg`    | R      | 32 bits | Read-only external status register.<br>• **Bit 0**: `halted` (Core halted status)<br>• **Bit 1**: `fault` (Core fault/exception status)                                   | `CoreAxiCSR.scala` (`statusReg`)  |
| `0x100` | `mscratch_mm`  | R      | 32 bits | Memory-mapped access to the internal `mscratch` CSR (via `coralnpu_csr.value(0)` / `io.csr.in.value(12)`).                                                                | `CoreAxiCSR.scala`                |
| `0x104` | `mepc_mm`      | R      | 32 bits | Memory-mapped read-only mirror of the internal Machine Exception Program Counter (`mepc`).                                                                                | `CoreAxiCSR.scala`                |
| `0x108` | `mtval_mm`     | R      | 32 bits | Memory-mapped read-only mirror of the internal Machine Trap Value (`mtval`).                                                                                              | `CoreAxiCSR.scala`                |
| `0x10C` | `mcause_mm`    | R      | 32 bits | Memory-mapped read-only mirror of the internal Machine Cause (`mcause`).                                                                                                  | `CoreAxiCSR.scala`                |
| `0x110` | `mcycle_l`     | R      | 32 bits | Memory-mapped read-only mirror of the lower 32 bits of the CPU cycle counter (`mcycle(31,0)`).                                                                            | `CoreAxiCSR.scala`                |
| `0x114` | `mcycle_h`     | R      | 32 bits | Memory-mapped read-only mirror of the upper 32 bits of the CPU cycle counter (`mcycle(63,32)`).                                                                           | `CoreAxiCSR.scala`                |
| `0x118` | `minstret_l`   | R      | 32 bits | Memory-mapped read-only mirror of the lower 32 bits of the instructions-retired counter (`minstret(31,0)`).                                                               | `CoreAxiCSR.scala`                |
| `0x11C` | `minstret_h`   | R      | 32 bits | Memory-mapped read-only mirror of the upper 32 bits of the instructions-retired counter (`minstret(63,32)`).                                                              | `CoreAxiCSR.scala`                |
| `0x120` | `mcontext0_mm` | R      | 32 bits | Memory-mapped read-only mirror of Machine Context 0 (`mcontext0`).                                                                                                        | `CoreAxiCSR.scala`                |

---

## CSR Field Definitions and Write Side Effects

The architectural state within the NPU's `Csr` module is modified during program execution or under trap handling conditions. Specific registers have explicit bit breakdowns and associated write side effects:

### `mstatus` (Machine Status - Address `0x300`)

| Bits    | Field  | Type | Reset   | Description & Write Side Effects                                                                                                                                                                                                    |
| :------ | :----- | :--- | :------ | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `3`     | `MIE`  | R/W  | `false` | **Machine Interrupt Enable**.<br>• **Write Effect**: Modifies interrupt eligibility globally.<br>• **Trap Entry Effect**: Cleared to `false.B` to disable nested interrupts.<br>• **MRET Effect**: Restored to the value of `MPIE`. |
| `7`     | `MPIE` | R/W  | `false` | **Machine Previous Interrupt Enable**.<br>• **Trap Entry Effect**: Captured from `MIE`.<br>• **MRET Effect**: Restored to `true.B` upon returning.                                                                                  |
| `10:9`  | `VS`   | R    | `1.U`   | **Vector Extension State**. Hardwired to `1.U(2.W)` (Initial/Active) if Vector extensions are enabled (`p.enableRvv` is true), else `0.U(2.W)`. Read-only; writes are ignored.                                                      |
| `14:13` | `FS`   | R    | `1.U`   | **Floating-Point State**. Hardwired to `1.U(2.W)` (Initial/Active) if Floating-Point extensions are enabled (`p.enableFloat` is true), else `0.U(2.W)`. Read-only; writes are ignored.                                              |

### `mie` (Machine Interrupt Enable - Address `0x304`)

- **Field Layout**:
  - **Bit 3**: `MSIE` (Machine Software Interrupt Enable)
  - **Bit 7**: `MTIE` (Machine Timer Interrupt Enable)
  - **Bit 11**: `MEIE` (Machine External Interrupt Enable)
- **Write Side Effect**: Writes are explicitly masked with `0x888` (`mie := wdata & "h888".U`). All other bits are ignored and hardwired to zero.

### `mip` (Machine Interrupt Pending - Address `0x344`)

- **Field Layout**:
  - **Bit 3**: `MSIP` (Machine Software Interrupt Pending) - Reflects input pin `io.software_irq`
  - **Bit 7**: `MTIP` (Machine Timer Interrupt Pending) - Reflects input pin `io.timer_irq`
  - **Bit 11**: `MEIP` (Machine External Interrupt Pending) - Reflects input pin `io.irq`
- **Access Behavior**: Read-only concatenation of the interrupt pins. Writes are completely ignored.

### `fcsr` (Floating-Point Control & Status - Address `0x003`)

- **Write Side Effect**: Writing to the unified `fcsr` register simultaneously updates the internal `fflags` register with `wdata(4,0)` and the rounding mode `frm` with `wdata(7,5)`.

---

## Fault Management Pipeline Integration & Mapping

Exceptions and hardware faults are processed in real-time by the `FaultManager` module and latched into the core CSR bank (`Csr.scala`).

### Fault Integration Flow

1. Pipeline logic monitors various execution stages and asserts raw fault signals on `io.in` of `FaultManager.scala` (e.g., undefined instructions, misaligned memory addresses, fetch failures).
2. The `FaultManager` prioritizes and aggregates these faults, asserting a unified output `io.out.valid`.
3. Upon a valid fault, `Csr.scala` latches the fault-specific outputs from `FaultManager` into the standard machine CSRs:
   - **`mepc`** captures the exception program counter.
   - **`mcause`** captures the machine cause code.
   - **`mtval`** captures auxiliary fault data (such as address or instruction bits).
4. Concurrently, the core enters a trap state, raising the top-level `io.fault` signal, which is reflected in Bit 1 of the external memory-mapped `statusReg` (`0x8`) for external tracking.

### Exception Causes (`mcause`) & Trap Values (`mtval`) Mapping

The following table documents how the `FaultManager` maps active pipeline exceptions to their corresponding architectural `mcause` and `mtval` values:

| Exception Source Condition   | `mcause` Code | `mtval` Latched Contents                                   | Description / Triggering Logic Citation                                                                                  |
| :--------------------------- | :------------ | :--------------------------------------------------------- | :----------------------------------------------------------------------------------------------------------------------- |
| **Instruction Access Fault** | `1.U`         | `0.U(p.xlen.W)`                                            | Fired when `fetchFault.valid` is high due to an out-of-bounds or protected fetch transaction.                            |
| **Illegal Instruction**      | `2.U`         | Raw instruction word (`io.in.undef(undef_fault_idx).inst`) | Fired when an undefined instruction opcode, unauthorized CSR operation, or unsupported Vector instruction is dispatched. |
| **Load Access Fault**        | `5.U`         | Offending memory address (`io.in.memory_fault.bits.addr`)  | Fired when a load memory request fails due to address alignment issues, bus errors, or unauthorized access.              |
| **Store/AMO Access Fault**   | `7.U`         | Offending memory address (`io.in.memory_fault.bits.addr`)  | Fired when a store memory request fails due to address alignment issues, bus errors, or unauthorized access.             |

[Source: hdl/chisel/src/coralnpu/scalar/FaultManager.scala | As of: 2026-07-10]

<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

> **Provenance & Traceability** - **Verified As Of:** 2026-07-10 - **Upstream Commit:** c9d3cd8816886ced4a935722205fd47aeb72eed9 - **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Csr.scala`, `hdl/chisel/src/coralnpu/CoreAxiCSR.scala`, `hdl/chisel/src/coralnpu/scalar/FaultManager.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- prettier-ignore-end -->
