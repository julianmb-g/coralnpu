# Core Decode and Dispatch Unit

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

> **Intended Audience:** Hardware Developers

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

The Decode and Dispatch unit (`DispatchV2` in RTL) is responsible for interpreting raw instruction data (`FetchInstruction`) and routing valid decoded commands (`AluCmd`, `BruCmd`, `LsuCmd`, etc.) to the appropriate downstream execution pipelines. It actively manages serialization, structural hazards, and architectural interlocks (such as Scoreboarding) across parameterizable instruction lanes.

## Core Interfaces

The unit connects directly to the instruction fetch queue and fans out to the execution backends.

| Interface Name             | Direction | Type                       | Description                                                                |
| :------------------------- | :-------- | :------------------------- | :------------------------------------------------------------------------- |
| `inst`                     | Input     | Decoupled                  | Raw instructions from the Fetch queue/IBus.                                |
| `rs1Read`, `rs2Read`       | Output    | Flipped RegfileReadAddrIO  | Scalar register file read requests.                                        |
| `rdMark`                   | Output    | Flipped RegfileWriteAddrIO | Scalar register file write target reservation.                             |
| `scoreboard`               | Input     | Bundle                     | Feed-forward scoreboarding status (`regd`, `comb`) from the Register File. |
| `alu`, `bru`, `mlu`, `dvu` | Output    | Valid                      | Dispatch signals for ALU, Branch, Multiplier, and Divider units.           |
| `lsu`                      | Output    | Decoupled                  | Dispatch queue for the Load/Store Unit.                                    |
| `csr`                      | Output    | Valid                      | Dispatch signal for Control and Status Register operations.                |
| `rvv` (Optional)           | Output    | Decoupled                  | Dispatch queue for Vector backend (`RvvCompressedInstruction`).            |
| `float` (Optional)         | Output    | Decoupled                  | Dispatch queue for Float backend (`FloatInstruction`).                     |

## Instruction Decoding

The internal `DecodeInstruction` object performs combinatorial extraction of the instruction fields and immediate types (`imm12`, `imm20`, `immjal`, `immbr`, `immcsr`, `immst`). It supports:

- **RV32IM**: Standard integer ALUs, branching, multiplying, and dividing.
- **ZBB**: Bit manipulation extensions (e.g., `clz`, `ctz`, `cpop`, `rev8`, `rol`/`ror`).

### Register Field Extraction

| Register | Field                        | Bits        |
| :------- | :--------------------------- | :---------- |
| `rd`     | Destination Register         | `op(11,7)`  |
| `rs1`    | Source Register 1            | `op(19,15)` |
| `rs2`    | Source Register 2            | `op(24,20)` |
| `rs3`    | Source Register 3 (Optional) | `op(31,27)` |

[Source: hdl/chisel/src/coralnpu/scalar/Decode.scala:L331, L345, L346]

### Immediate Field Extraction

| Immediate | Type   | Logic (Sign/Zero Extension)                                     |
| :-------- | :----- | :-------------------------------------------------------------- |
| `imm12`   | I-type | `Cat(Fill(20, op(31)), op(31,20))`                              |
| `imm20`   | U-type | `Cat(op(31,12), 0.U(12.W))`                                     |
| `immjal`  | J-type | `Cat(Fill(12, op(31)), op(19,12), op(20), op(30,21), 0.U(1.W))` |
| `immbr`   | B-type | `Cat(Fill(20, op(31)), op(7), op(30,25), op(11,8), 0.U(1.W))`   |
| `immcsr`  | Zicsr  | `op(19,15)`                                                     |
| `immst`   | S-type | `Cat(Fill(20, op(31)), op(31,25), op(11,7))`                    |

[Source: hdl/chisel/src/coralnpu/scalar/Decode.scala:L829-L834]

### Instruction Bit Masks (BitPat)

The following table details the exact bit patterns (`BitPat`) used by the decoder to identify instructions. `?` denotes "don't care" bits (register addresses or immediates).

| Instruction | Bit Pattern                               |
| :---------- | :---------------------------------------- |
| **LUI**     | `b????????????????????_?????_0110111`     |
| **AUIPC**   | `b????????????????????_?????_0010111`     |
| **JAL**     | `b????????????????????_?????_1101111`     |
| **JALR**    | `b????????????_?????_000_?????_1100111`   |
| **BEQ**     | `b???????_?????_?????_000_?????_1100011`  |
| **BNE**     | `b???????_?????_?????_001_?????_1100011`  |
| **BLT**     | `b???????_?????_?????_100_?????_1100011`  |
| **BGE**     | `b???????_?????_?????_101_?????_1100011`  |
| **BLTU**    | `b???????_?????_?????_110_?????_1100011`  |
| **BGEU**    | `b???????_?????_?????_111_?????_1100011`  |
| **CSRRW**   | `b????????????_?????_?01_?????_1110011`   |
| **CSRRS**   | `b????????????_?????_?10_?????_1110011`   |
| **CSRRC**   | `b????????????_?????_?11_?????_1110011`   |
| **LB**      | `b????????????_?????_000_?????_0000011`   |
| **LH**      | `b????????????_?????_001_?????_0000011`   |
| **LW**      | `b????????????_?????_010_?????_0000011`   |
| **LBU**     | `b????????????_?????_100_?????_0000011`   |
| **LHU**     | `b????????????_?????_101_?????_0000011`   |
| **SB**      | `b????????????_?????_000_?????_0100011`   |
| **SH**      | `b????????????_?????_001_?????_0100011`   |
| **SW**      | `b????????????_?????_010_?????_0100011`   |
| **FENCE**   | `b0000_????_????_00000_000_00000_0001111` |
| **ADDI**    | `b????????????_?????_000_?????_0010011`   |
| **SLTI**    | `b????????????_?????_010_?????_0010011`   |
| **SLTIU**   | `b????????????_?????_011_?????_0010011`   |
| **XORI**    | `b????????????_?????_100_?????_0010011`   |
| **ORI**     | `b????????????_?????_110_?????_0010011`   |
| **ANDI**    | `b????????????_?????_111_?????_0010011`   |
| **SLLI**    | `b0000000_?????_?????_001_?????_0010011`  |
| **SRLI**    | `b0000000_?????_?????_101_?????_0010011`  |
| **SRAI**    | `b0100000_?????_?????_101_?????_0010011`  |
| **ADD**     | `b0000000_?????_?????_000_?????_0110011`  |
| **SUB**     | `b0100000_?????_?????_000_?????_0110011`  |
| **SLT**     | `b0000000_?????_?????_010_?????_0110011`  |
| **SLTU**    | `b0000000_?????_?????_011_?????_0110011`  |
| **XOR**     | `b0000000_?????_?????_100_?????_0110011`  |
| **OR**      | `b0000000_?????_?????_110_?????_0110011`  |
| **AND**     | `b0000000_?????_?????_111_?????_0110011`  |
| **SLL**     | `b0000000_?????_?????_001_?????_0110011`  |
| **SRL**     | `b0000000_?????_?????_101_?????_0110011`  |
| **SRA**     | `b0100000_?????_?????_101_?????_0110011`  |
| **MUL**     | `b0000_001_?????_?????_000_?????_0110011` |
| **MULH**    | `b0000_001_?????_?????_001_?????_0110011` |
| **MULHSU**  | `b0000_001_?????_?????_010_?????_0110011` |
| **MULHU**   | `b0000_001_?????_?????_011_?????_0110011` |
| **DIV**     | `b0000_001_?????_?????_100_?????_0110011` |
| **DIVU**    | `b0000_001_?????_?????_101_?????_0110011` |
| **REM**     | `b0000_001_?????_?????_110_?????_0110011` |
| **REMU**    | `b0000_001_?????_?????_111_?????_0110011` |
| **ANDN**    | `b0100000_?????_?????_111_?????_0110011`  |
| **ORN**     | `b0100000_?????_?????_110_?????_0110011`  |
| **XNOR**    | `b0100000_?????_?????_100_?????_0110011`  |
| **CLZ**     | `b0110000_00000_?????_001_?????_0010011`  |
| **CTZ**     | `b0110000_00001_?????_001_?????_0010011`  |
| **CPOP**    | `b0110000_00010_?????_001_?????_0010011`  |
| **MAX**     | `b0000101_?????_?????_110_?????_0110011`  |
| **MAXU**    | `b0000101_?????_?????_111_?????_0110011`  |
| **MIN**     | `b0000101_?????_?????_100_?????_0110011`  |
| **MINU**    | `b0000101_?????_?????_101_?????_0110011`  |
| **SEXTB**   | `b0110000_00100_?????_001_?????_0010011`  |
| **SEXTH**   | `b0110000_00101_?????_001_?????_0010011`  |
| **ROL**     | `b0110000_?????_?????_001_?????_0110011`  |
| **ROR**     | `b0110000_?????_?????_101_?????_0110011`  |
| **ORCB**    | `b0010100_00111_?????_101_?????_0010011`  |
| **REV8**    | `b0110100_11000_?????_101_?????_0010011`  |
| **ZEXTH**   | `b0000100_00000_?????_100_?????_0110011`  |
| **RORI**    | `b0110000_?????_?????_101_?????_0010011`  |

## Integrated Partial Decoder Behavior

The CoralNPU employs asymmetric or "partial" decoding across its multiple instruction lanes. While Lane 0 (Pipeline 0) is a fully-featured decoder, subsequent lanes (Pipeline > 0) are stubbed out for complex or serialized instructions.

If the pipeline index is greater than 0, the following instruction classes are structurally stubbed out (forced to invalid) by the decoder:

- **System & CSRs:** `csrrw`, `csrrs`, `csrrc`, `ebreak`, `ecall`, `mpause`, `mret`, `wfi`
- **Complex Arithmetic:** `div`, `divu`, `rem`, `remu`
- **Fences:** `fence`, `fencei`
- **Floating-Point:** All FPU instructions (`enableFloat` must be true)

These operations can only dispatch out of Slot 0.

## Hazards, Serialization, and Interlocks

The dispatch logic prevents structural, control, and data hazards by stalling instructions across the parameterizable lanes if an interlock condition is triggered.

### Scoreboarding (RAW/WAW Hazards)

The dispatch engine computes `readAfterWrite` and `writeAfterWrite` vectors. It scans all instructions dynamically against the current `scoreboard.regd` (register status) and `scoreboard.comb` (write-forwarding status) to prevent collisions. For systems with the FPU enabled, a parallel `floatReadAfterWrite` and `floatWriteAfterWrite` check is performed.

### Control-Flow and Context Switching

- **Jump/Branch Interlocks**: Instructions after an unconditional jump, context switch (`mret`, `ecall`), or a fence are blocked in the current cycle. Only pure ALU or Branch operations can be dynamically scheduled in lanes following a conditional branch in the same cycle.

### The "Slot 0" Restriction

Specific instructions are architecturally required to be dispatched out of "Slot 0" exclusively (with no other instructions dispatched on the same cycle in wider scalar cores). This includes:

- Fences and Core Idle transitions (`wfi`, `mpause`).
- CSR instructions.
- RVV and Floating Point instructions that cross architectural boundaries.
- Undefined opcodes.

### RVV & LSU Interlocks

- **Vector Configuration**: RVV Memory operations (`vload`, `vstore`) stall if the vector configuration state (`vtype`/`vl`) has changed and is invalid.
- **Vstart Constraints**: RVV instructions demanding a zero `vstart` state trigger an explicit hardware fault (`rvvFault`) if dispatched when `vstart != 0`.
- **Queue Backpressure**: RVV and LSU instructions maintain counters against their respective buffer capacities (`rvvQueueCapacity`, `lsuQueueCapacity`).

---

## Safe Enum Casting and `SafeMuxUpTo1H` Logic

The instruction decoder enforces safe `ChiselEnum` casting during dynamic operation dispatch. The decoders utilize the `SafeMuxUpTo1H` utility (defined in `Library.scala`).

`SafeMuxUpTo1H` explicitly falls back to a default valid-false state when no instruction selectors match (e.g., `MakeValid(false.B, AluOp.ADD)` for the ALU). It employs the `.safe()` method on the underlying `ChiselEnum` to guarantee that the resulting bitfield maps to a defined enumeration value. Undefined or out-of-bounds instruction encodings default to an invalid operation.

## Verification

This hardware block is validated as part of the top-level simulation and UVM testbench environment.

- [CoralNPU Top-Level Testbench](tests/uvm/tb/coralnpu_tb_top.sv)

--------------------------------------------------------------------------------

<!-- prettier-ignore -->
**Provenance & Traceability** - **Verified As Of:** 2026-07-07 - **Upstream Commit:** 8ba6f4108901602e14e28345b4bd009e6f3b6897 - **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Decode.scala:L301`, `hdl/chisel/src/coralnpu/scalar/Decode.scala:L837`, `hdl/chisel/src/common/Library.scala:L251` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
> **Traceability:** Generated by Gemini. Derived from upstream commit 7d6e35cff17732cad8f99b866333423ea1f4d750.
