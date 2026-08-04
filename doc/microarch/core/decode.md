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

# Core decode and dispatch unit

> **Intended Audience:** HW Devs

The Decode and Dispatch unit (`DispatchV2` in RTL) is responsible for interpreting raw instruction data (`FetchInstruction`) and routing valid decoded commands (`AluCmd`, `BruCmd`, `LsuCmd`, etc.) to the appropriate downstream execution pipelines. It actively manages serialization, structural hazards, and architectural interlocks (such as Scoreboarding) across parameterizable instruction lanes.

## Core interfaces

The unit connects directly to the instruction fetch queue and fans out to the execution backends.

| Interface Name | Direction | Type | Description |
| :--- | :--- | :--- | :--- |
| `io.halted` | Input | `Bool` | Core halted control signal. |
| `io.mactive` | Input | `Bool` | Memory active status. |
| `io.lsuActive` | Input | `Bool` | LSU active status. |
| `io.scoreboard` | Input | `Bundle` | Scoreboard status (regd, comb). |
| `io.fscoreboard` | Input | `Option[UInt(32.W)]` | Floating-point scoreboard status. |
| `io.branchTaken` | Input | `Bool` | Branch taken status. |
| `io.csrFault` | Output | `Vec[Bool]` | CSR fault status per lane. |
| `io.jalFault` | Output | `Vec[Bool]` | JAL fault status per lane. |
| `io.jalrFault` | Output | `Vec[Bool]` | JALR fault status per lane. |
| `io.bxxFault` | Output | `Vec[Bool]` | Branch fault status per lane. |
| `io.undefFault` | Output | `Vec[Bool]` | Undefined instruction fault status per lane. |
| `io.rvvFault` | Output | `Option[Vec[Bool]]` | RVV fault status per lane. |
| `io.bruTarget` | Output | `Vec[UInt]` | BRU target address per lane. |
| `io.jalrTarget` | Input | `Vec[RegfileBranchTargetIO]` | JALR target address per lane. |
| `io.interlock` | Input | `Bool` | Interlock status. |
| `io.inst` | Input | `Vec[Decoupled[FetchInstruction]]` | Decoded instruction input stream. |
| `io.rs1Read` | Output | `Vec[RegfileReadAddrIO]` | Register file read address 1. |
| `io.rs1Set` | Output | `Vec[RegfileReadSetIO]` | Register file read set 1. |
| `io.rs2Read` | Output | `Vec[RegfileReadAddrIO]` | Register file read address 2. |
| `io.rs2Set` | Output | `Vec[RegfileReadSetIO]` | Register file read set 2. |
| `io.rdMark` | Output | `Vec[RegfileWriteAddrIO]` | Register file write address. |
| `io.busRead` | Output | `Vec[RegfileBusAddrIO]` | Register file bus address. |
| `io.alu` | Output | `Vec[Valid[AluCmd]]` | ALU command output. |
| `io.bru` | Output | `Vec[Valid[BruCmd]]` | BRU command output. |
| `io.csr` | Output | `Valid[CsrCmd]` | CSR command output. |
| `io.lsu` | Output | `Vec[Decoupled[LsuCmd]]` | LSU command output. |
| `io.lsuQueueCapacity` | Input | `UInt(3.W)` | LSU queue capacity. |
| `io.mlu` | Output | `Vec[Decoupled[MluCmd]]` | MLU command output. |
| `io.dvu` | Output | `Vec[Decoupled[DvuCmd]]` | DVU command output. |
| `io.rvv` | Output | `Option[Vec[Decoupled[RvvCompressedInstruction]]]` | RVV command output. |
| `io.float` | Output | `Option[Decoupled[FloatInstruction]]` | Float command output. |
| `io.branch` | Output | `Vec[Bool]` | Branch instruction detected per lane. |
| `io.jump` | Output | `Vec[Bool]` | Jump instruction detected per lane. |

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Decode.scala:L301`, `hdl/chisel/src/coralnpu/scalar/Decode.scala:L837`, `hdl/chisel/src/common/Library.scala:L251` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
