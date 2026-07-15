# Instruction Details

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

> **Intended Audience:** Software Developers, Compiler Engineers


This document describes the encoding, scheduling constraints, execution side effects, and RTL decoder paths for core controls, fences, and custom instruction extensions implemented on the CoralNPU.

---

## 1. Core Controls & Fences Overview

The CoralNPU implements standard RISC-V control flow and fence instructions alongside custom hardware acceleration primitives. The centralized instruction decoder (`Decode.scala`) processes these instructions and dispatches them to their respective execution pipelines.

| Instruction | Type     | Opcode (Hex) | Encoding Bit Pattern (`BitPat` / Hex) | Decoder Logic (`Decode.scala`) | Side Effects / Behavior                                         |
| :---------- | :------- | :----------- | :------------------------------------ | :----------------------------- | :-------------------------------------------------------------- |
| `ebreak`    | Standard | `0x73`       | `0x00100073`                          | Line 909                       | Pauses execution and triggers a hardware Debug Trap.            |
| `ecall`     | Standard | `0x73`       | `0x00000073`                          | Line 910                       | Triggers a software Environment Call Trap to Machine Mode.      |
| `mpause`    | Custom   | `0x73`       | `0x08000073`                          | Line 911                       | Halts core pipeline execution and gates the core clock.         |
| `mret`      | Standard | `0x73`       | `0x30200073`                          | Line 912                       | Restores Machine Mode context and jumps to `mepc`.              |
| `wfi`       | Standard | `0x73`       | `0x10500073`                          | Line 913                       | Suspends instruction execution until an asynchronous interrupt. |
| `fencei`    | Standard | `0x0F`       | `0x0000100F`                          | Line 916                       | Pipeline fence: flushes pipeline and invalidates ITCM fetches.  |
| `flushat`   | Custom   | `0x77`       | `0x10000077 \| (rs1 << 15)`           | Line 917                       | Invalidates a single specific cache line at address in `rs1`.   |
| `flushall`  | Custom   | `0x77`       | `0x10000077`                          | Line 918                       | Invalidates all cache lines in the data cache.                  |

---

## 2. Custom Instruction Deep Dive

### 2.1. `mpause` (Core Pause/Idle)

The custom `mpause` instruction halts execution to optimize power consumption when the NPU is idle.

- **Encoding**:
  - **Opcode**: `1110011` (`0x73` - SYSTEM)
  - **funct12**: `000010000000` (`0x080`)
  - **funct3**: `000`
  - **rs1**: `00000` (`x0`)
  - **rd**: `00000` (`x0`)
  - **Raw Hexadecimal**: `0x08000073`
- **Scheduling Constraints**:
  - **Slot 0 Only**: `mpause` is restricted to Lane 0 (Slot 0). It enforces a strict single-issue slot 0 interlock, preventing any other instructions from being dispatched in the same cycle.
  - **Core Idle Guard**: `mpause` can only be dispatched when the internal signal `coreIdle` is asserted.
- **Execution Side Effects**:
  - Upon execution in the Branch Unit (`Bru.scala`), `mpause` asserts `io.csr.get.in.halt` to signal the CSR block.
  - This transitions the clock management block to gate the main core clock (`clk`), placing the NPU in a low-power, quiescent state until a hardware interrupt or external debug request wakes the core.

---

### 2.2. Custom Cache Flush Instructions (`flushat`, `flushall`)

The CoralNPU includes custom hardware commands to manage the data cache footprint and guarantee coherency during host execution handoffs. Both instructions operate under the Custom-3 opcode space (`0x77`).

```
Custom Cache Flush Instruction Format:
+---------+-------+-------+-----+-------+---------+
| funct7  |  rs2  |  rs1  | f3  |  rd   | opcode  |
| 31   25 | 24 20 | 19 15 | 14  | 11  7 | 6     0 |
+---------+-------+-------+-----+-------+---------+
| 0010??? | 00000 |  base | 000 | 00000 | 1110111 |  (flushat: rs1 != x0)
| 0010??? | 00000 | 00000 | 000 | 00000 | 1110111 |  (flushall: rs1 == x0)
+---------+-------+-------+-----+-------+---------+
```

#### `flushat` (Flush Cache Line at Address)

Invalidates a specific line inside the data cache, utilizing the virtual or physical address loaded in the register specified by `rs1`.

- **Encoding**:
  - **Opcode**: `1110111` (`0x77` - Custom-3)
  - **rd**: `00000` (`x0`)
  - **funct3**: `000`
  - **rs1**: Non-zero source register (`rs1 != 0.U`) holding the base address.
  - **rs2**: `00000`
  - **funct7**: `0010???` (Matches `0010???` bit pattern)
- **Scheduling Constraints**:
  - Enforces a Slot 0 restriction and is classified as `isFency()`.
  - Instruction fetch and issue are stalled until the LSU is entirely inactive (`!lsuActive` and `!io.mactive`) before dispatching.
- **Side Effects**:
  - Dispatches an LSU command with opcode `LsuOp.FLUSHAT`.
  - Bypasses standard register writeback.
  - Triggers the LSU's cache invalidation logic, causing the specific cache line mapped to the target address to be invalidated.

#### `flushall` (Flush All Cache Lines)

Performs a global invalidation across all data cache lines.

- **Encoding**:
  - Identical to `flushat`, except that `rs1` is explicitly set to `00000` (`x0`).
- **Scheduling Constraints**:
  - Identical to `flushat`. Strict Slot 0 restriction, stalls dispatch until the pipeline and LSU are idle.
- **Side Effects**:
  - Dispatches an LSU command with opcode `LsuOp.FLUSHALL`.
  - Instructs the LSU's flush state machine to sweep through all cache ways and lines, invalidating them globally and ensuring clean state handoffs.

---

## 3. RTL Implementation & Traceability

The functional behavior and orchestration of these control, fence, and custom instructions can be traced to the following Chisel modules:

1.  **Instruction Decoder (`Decode.scala`)**:
    - Combinatorial instruction decoding and `BitPat` matching take place under `object DecodeInstruction` starting around Line 821.
    - Asymmetric stubbing rules (disallowing controls beyond Pipeline 0) are handled under Line 937.
    - Interlocks, Scoreboard tracking, and Single-Issue Slot 0 dispatch rules are enforced on Lines 490-530.
2.  **Load/Store Unit (`Lsu.scala`)**:
    - `LsuOp` enumeration defines the internal operation identifiers (`FLUSHAT`, `FLUSHALL`) on Lines 81-82.
    - The LSU's flush state machine tracks and issues cache control commands asynchronously on Lines 854-884.
3.  **Scalar Core (`SCore.scala`)**:
    - The top-level core arbitrates flush handshakes between the LSU's flush request and the fetch controller's instruction flush (`iflush`) / data flush (`dflush`) channels on Lines 103-114.

<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-10 - **Upstream Commit:** c9d3cd8816886ced4a935722205fd47aeb72eed9 - **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Decode.scala:909-918`, `hdl/chisel/src/coralnpu/scalar/Lsu.scala:81-82`, `hdl/chisel/src/coralnpu/scalar/SCore.scala:103-114` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- prettier-ignore-end -->

> **Traceability:** Generated by Gemini. Derived from upstream commit 9a1e82634c2b0f3d42310f89cd1484d8f3302ec9.
