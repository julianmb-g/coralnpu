# Scalar Register File (Regfile)

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

> **Intended Audience:** Hardware Developers, Compiler Engineers

The `Regfile` module (`Regfile.scala`) implements the primary 32-entry integer register file for the CoralNPU scalar core. Beyond providing standard read/write access to the general-purpose registers (GPRs), it houses the global scoreboard logic responsible for tracking Read-After-Write (RAW) dependencies and interlocking the instruction decoders.

## Architectural Purpose

The register file is parameterized by the number of instruction lanes (`p.instructionLanes`). It is highly multi-ported to support superscalar dispatch and features internal write-forwarding to minimize pipeline stalls.

## Read and Write Ports

The register file provides the following structural port configuration:

| Port Type            | Quantity               | Purpose                                                                                                                                                                                                                  |
| :------------------- | :--------------------- | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Read Ports**       | `instructionLanes * 2` | Supports reading two operands (`rs1`, `rs2`) per instruction lane during the decode cycle.                                                                                                                               |
| **Write Ports**      | `instructionLanes + 2` | `instructionLanes` ports are dedicated to the standard ALUs. Two extra write ports (`extraWritePorts`) are provisioned for longer-latency units (e.g., the MLU/DVU and the LSU) to prevent structural writeback hazards. |
| **Bus/Target Ports** | `instructionLanes`     | Priority-encoded address ports for LSU address generation and branch target calculation.                                                                                                                                 |

### Write Forwarding

Read ports are equipped with write forwarding logic. If a read index matches an active write index in the same cycle, the data is forwarded directly from the write port to the read port (`rwdata(i) := Mux(write, wdata(i), rdata(i))`), ensuring back-to-back dependent instructions do not stall unnecessarily.

## Global Scoreboard

To manage RAW dependencies across the pipeline, the `Regfile` module maintains a 32-bit global scoreboard.

- **Set Condition (`scoreboard_set`)**: During the decode cycle, speculated opcodes set the scoreboard bits corresponding to their destination registers (`rd`). If an opcode is in the shadow of a taken branch, it still sets the scoreboard, but the actual writeback will be masked.
- **Clear Condition (`scoreboard_clr`)**: During the execute/writeback cycle, valid write data arrivals clear the respective scoreboard bits.

The scoreboard value (`io.scoreboard.regd` and combinatorial `io.scoreboard.comb`) is routed back to the decoders. If an instruction attempts to read a register whose scoreboard bit is set, the decoder will interlock (stall) until the bit is cleared.

> [!NOTE] > `x0` (register 0) is hardwired to zero and optimized away. It is not tracked by the scoreboard.

## Assertions and Faults

The `Regfile` module includes hardware assertions to detect critical structural errors:

- **Write Collisions**: Asserts that no two write ports attempt to write to the same register index (other than `x0`) in the same cycle.
- **Scoreboard Errors**: Asserts that the scoreboard state remains consistent and does not experience spurious clears (`scoreboard_error`).

<!-- mdformat off -->

<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

<!-- prettier-ignore-end -->

**Provenance & Traceability** - **Verified As Of:** 2026-07-06 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Regfile.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
