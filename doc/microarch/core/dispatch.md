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

# CoralNPU dispatch rules

> **Intended Audience:** HW Devs

The following describes the general rules used to determine which instructions can
be dispatched in the CoralNPU core. The Dispatch unit operates based on the scoreboard status.

## Interfaces

### Port definitions

| Signal Name | Direction | Type | Description |
| :--- | :--- | :--- | :--- |
| `halted` | Input | `Bool` | Core halted state indication. |
| `mactive` | Input | `Bool` | Memory active status. |
| `lsuActive` | Input | `Bool` | LSU active status. |
| `scoreboard.regd` | Input | `UInt(32.W)` | Integer register destination scoreboard (pending writebacks). |
| `scoreboard.comb` | Input | `UInt(32.W)` | Combinatorial register scoreboard (pending writebacks from active pipeline). |
| `fscoreboard` | Input | `UInt(32.W)` | Floating-point register scoreboard (optional, conditional on float support). |
| `branchTaken` | Input | `Bool` | Branch taken feedback signal. |
| `csrFault` | Output | `Vec(p.instructionLanes, Bool)` | Signal indicating an exception in a CSR operation for each instruction lane. |
| `jalFault` | Output | `Vec(p.instructionLanes, Bool)` | Exception signal indicating a jump-and-link fault. |
| `jalrFault` | Output | `Vec(p.instructionLanes, Bool)` | Exception signal indicating a jump-and-link-register fault. |
| `bxxFault` | Output | `Vec(p.instructionLanes, Bool)` | Exception signal indicating a branch fault. |
| `undefFault` | Output | `Vec(p.instructionLanes, Bool)` | Exception signal for undefined instructions. |
| `rvvFault` | Output | `Vec(p.instructionLanes, Bool)` | Exception signal for vector instructions (optional, conditional on RVV support). |
| `bruTarget` | Output | `Vec(p.instructionLanes, UInt(p.programCounterBits.W))` | Branch Resolution Unit target PCs. |
| `jalrTarget` | Input | `Vec(p.instructionLanes, new RegfileBranchTargetIO(p))` | Regfile branch target outputs for JALR instruction. |
| `interlock` | Input | `Bool` | Hazard interlocking signal. |
| `inst` | Input | `Vec(p.instructionLanes, Flipped(Decoupled(new FetchInstruction(p))))` | Decoupled instruction streams from Fetch unit. |
| `rs1Read` | Input | `Vec(p.instructionLanes, Flipped(new RegfileReadAddrIO(p)))` | Regfile read address source 1. |
| `rs1Set` | Input | `Vec(p.instructionLanes, Flipped(new RegfileReadSetIO(p)))` | Regfile read data valid indicator for rs1. |
| `rs2Read` | Input | `Vec(p.instructionLanes, Flipped(new RegfileReadAddrIO(p)))` | Regfile read address source 2. |
| `rs2Set` | Input | `Vec(p.instructionLanes, Flipped(new RegfileReadSetIO(p)))` | Regfile read data valid indicator for rs2. |
| `rdMark` | Input | `Vec(p.instructionLanes, Flipped(new RegfileWriteAddrIO(p)))` | Destination register writeback scoreboard marking. |
| `busRead` | Input | `Vec(p.instructionLanes, Flipped(new RegfileBusAddrIO(p)))` | Read bus address interface. |
| `rdMark_flt` | Input | `Flipped(new RegfileWriteAddrIO(p))` | Floating-point register writeback scoreboard marking (optional). |
| `rvvRdMark` | Input | `Vec(p.instructionLanes, Flipped(new RegfileWriteAddrIO(p)))` | Vector register writeback scoreboard marking (optional). |
| `frs1Read` | Input | `Vec(p.instructionLanes, Flipped(new RegfileReadAddrIO(p)))` | Floating-point register read address source 1 (optional). |
| `alu` | Output | `Vec(p.instructionLanes, Valid(new AluCmd(p)))` | Command vectors sent to integer ALU units. |
| `bru` | Output | `Vec(p.instructionLanes, Valid(new BruCmd(p)))` | Command vectors sent to BRU. |
| `csr` | Output | `Valid(new CsrCmd(p))` | Command interface sent to CSR. |
| `lsu` | Output | `Vec(p.instructionLanes, Decoupled(new LsuCmd(p)))` | Decoupled LSU commands sent to Load Store Unit. |
| `lsuQueueCapacity` | Input | `UInt(3.W)` | LSU instruction queue remaining capacity. |
| `mlu` | Output | `Vec(p.instructionLanes, Decoupled(new MluCmd(p)))` | Decoupled MLU commands sent to Multiplier unit. |
| `dvu` | Output | `Vec(p.instructionLanes, Decoupled(new DvuCmd(p)))` | Decoupled DVU commands sent to Divider unit. |
| `rvv` | Output | `Vec(p.instructionLanes, Decoupled(new RvvCompressedInstruction(p)))` | Decoupled vector instructions sent to Vector Backend (optional). |
| `rvvState` | Input | `Valid(new RvvConfigState(p))` | Current Vector Backend configuration state (optional). |
| `rvvIdle` | Input | `Bool` | Idle status returned from Vector Backend (optional). |
| `rvvQueueCapacity` | Input | `UInt(4.W)` | Vector Backend instruction queue remaining capacity (optional). |
| `float` | Output | `Decoupled(new FloatInstruction(p))` | Decoupled float instructions sent to Floating-Point Core (optional). |
| `csrFrm` | Input | `UInt(3.W)` | Floating-point rounding mode from CSR (optional). |
| `fbusPortAddr` | Output | `UInt(5.W)` | Floating-point register address bus output (optional). |
| `retirement_buffer_nSpace` | Input | `UInt` | Remaining free space inside the retirement buffer. |
| `retirement_buffer_empty` | Input | `Bool` | Status indicating if the retirement buffer is empty. |
| `retirement_buffer_trap_pending` | Input | `Bool` | Status indicating if a trapped instruction is pending. |
| `single_step` | Input | `Bool` | Debug single-step mode enable status. |
| `debug_mode` | Input | `Bool` | Status indicating if the core is currently in debug mode. |
| `branch` | Output | `Vec(p.instructionLanes, Bool)` | Asserted if the dispatched instruction is a conditional branch. |
| `jump` | Output | `Vec(p.instructionLanes, Bool)` | Asserted if the dispatched instruction is an unconditional jump. |

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Decode.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
