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

# Scalar Register File (Regfile)

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

> **Intended Audience:** HW Devs, SW/Compiler Devs

The `Regfile` module implements the primary 32-entry integer general-purpose register file (GPR) for the CoralNPU scalar core. Beyond providing standard read/write access to scalar registers, it houses the global scoreboard logic responsible for tracking Read-After-Write (RAW) dependencies and interlocking the instruction decoders.

## Architectural Purpose

The register file is parameterized by `p.scalarRegCount` (typically 32 general-purpose registers) of width `p.xlen`. To support superscalar execution and dispatch without stalls, it features a highly multi-ported memory array, combinational write-forwarding, a `readSet` injection interface, and a dual-nature scoreboard.

## Read and Write Ports (QA-RF-001)

The register file provides a highly multi-ported interface parameterized by the number of instruction lanes (`p.instructionLanes`).

### Port Count and Configuration
- **Read Ports**: `p.instructionLanes * 2` ports (e.g., 8 ports for 4 lanes). Each port consists of an input read address (`io.readAddr(i)`) and a sequential read output (`io.readData(i)`).
- **Write Ports**: `p.instructionLanes + extraWritePorts` (where `extraWritePorts = 2`, total 6 write ports for 4 lanes).
- **Port Allocation**:
  - `p.instructionLanes` write ports are dedicated to standard core pipelines (one per instruction lane) for same-cycle results.
  - `extraWritePorts` (2 ports) are dedicated to late-writeback units: one for the Matrix Lookup Unit / Data Vector Unit (MLU/DVU) and one for the Load-Store Unit (LSU) to support longer/variable latency operations.

### Cycle Separation
Write operations split their handshake across pipeline stages:
- Write addresses are declared during the **Decode cycle** via `io.writeAddr` (to set the global scoreboard early).
- Write data is supplied during the **Execute/Writeback cycle** via `io.writeData`.

### Port Interface Table

| Port / Interface | Group | Type | Width | Description |
| :--- | :--- | :--- | :--- | :--- |
| `io.readAddr` | Decode | Input Vec | `log2Ceil(p.scalarRegCount)` | Read addresses for operand fetch. |
| `io.readSet` | Decode | Input Vec | `p.xlen` | Special override injection. |
| `io.writeAddr` | Decode | Input Vec | `log2Ceil(p.scalarRegCount)` | Speculated destination register addresses. |
| `io.readData` | Execute | Output Vec | `p.xlen` | Read data responses. |
| `io.writeData` | Execute | Input Vec | `p.xlen` + addr | Writeback data and destination address. |
| `io.writeMask` | Execute | Input Vec | Bool | Mask indicating write skip (e.g. branch shadow). |

[Primary Source: `hdl/chisel/src/coralnpu/scalar/Regfile.scala` - Read/Write Ports definition]

## Write Forwarding and Latency (QA-RF-002)

Register read responses operate with a **1-cycle sequential latency**. Both ready state and data bits are registered:
- `readDataReady(i)` tracks whether a read request was active.
- `readDataBits(i)` stores the data retrieved.

```scala
readDataReady(i) := io.readAddr(i).valid || io.readSet(i).valid
```

### Write Forwarding (Bypass) Logic
Because read responses take 1 cycle, same-cycle writebacks would normally introduce a pipeline stall. To prevent this, the register file contains internal **combinational write forwarding bypass paths**:
- If an active writeback (with `valid` high and `writeMask` low) targets a register index currently being read, the updated data (`writeData`) is combinationally forwarded directly to the read output of the same cycle (`rwdata`).
- This bypasses the memory array's internal 1-cycle delay, making the updated value immediately available to the executing instruction in the next stage.

```scala
val write = VecAt(writeValid, idx)
rdata(i)  := VecAt(regfile, idx)
wdata(i)  := VecAt(writeData, idx)
rwdata(i) := Mux(write, wdata(i), rdata(i))
```

[Primary Source: `hdl/chisel/src/coralnpu/scalar/Regfile.scala` - Read port and write forwarding bypass]

## Write Hazard and Collision Detection (QA-RF-003)

Hardware prohibits multiple write ports from targeting the same non-zero register index in the same clock cycle. 

To prevent physical write collisions and structural hazards, the hardware implements the following mechanisms:
- **Index Priority Masking**: For each register `i > 0`, the write valid signals of all write ports are verified. Chisel asserts that at most one port succeeds:
  ```scala
  assert(PopCount(valid) <= 1.U)
  ```
- **Hazard Diagnostic Assertion**: A dedicated register `write_fail` tracks overlapping writes. If two distinct write ports both present valid data for the same non-zero destination index, an assertion fires on the subsequent cycle to aid simulation debugging:
  ```scala
  write_fail := io.writeData(i).valid && io.writeData(j).valid &&
                io.writeData(i).bits.addr === io.writeData(j).bits.addr &&
                io.writeData(i).bits.addr =/= 0.U
  assert(!write_fail)
  ```

[Primary Source: `hdl/chisel/src/coralnpu/scalar/Regfile.scala` - Multi-port collision assertions]

## Global Scoreboard Dependency Tracking (QA-RF-004)

The `Regfile` houses a global dependency tracking register (`scoreboard`) of width `p.scalarRegCount`. The scoreboard manages RAW interlocks in the decoders by tracking speculated outstanding destination registers.

### Scoreboard Mechanics
- **Bit-Setting**: Bits are set on the scoreboard during the decode cycle based on speculated destination addresses declared via `io.writeAddr` (`scoreboard_set`).
- **Bit-Clearing**: Scoreboard bits are cleared during writeback cycles (`scoreboard_clr` mapped from `io.writeData`). Bit 0 (corresponding to register `x0`) is always masked out of the clear mask to ensure it is never scoreboard-tracked.
- **Consistency Guard**: Chisel asserts that cleared scoreboard bits were previously set:
  ```scala
  scoreboard_error := ((scoreboard & scoreboard_clr) =/= scoreboard_clr) && !dm_write_valid
  assert(!scoreboard_error)
  ```

### Dual Scoreboard Fields: `regd` vs. `comb`
To allow the instruction decoders maximum efficiency in scheduling, the scoreboard outputs two distinct fields:
- **`io.scoreboard.regd`**: Outputs the registered scoreboard state directly (`scoreboard`).
- **`io.scoreboard.comb`**: Outputs the scoreboard state combinationally subtracted by the same-cycle writeback clear mask (`scoreboard & ~scoreboard_clr`). This allows decoders to combinationally resolve and clear RAW interlocks on the exact same cycle the writeback data arrives!

[Primary Source: `hdl/chisel/src/coralnpu/scalar/Regfile.scala` - Scoreboard and lock tracking fields]

## Register x0 Special Handling (QA-RF-005)

The integer register 0 (`x0`) is hardwired to zero, as mandated by the RISC-V ISA. The hardware employs the following optimizations to bypass `x0` tracking and save power:
- **Memory Array Bypassing**: `writeValid(0)` is permanently hardwired to `true.B`, and `writeData(0)` is permanently tied to `0.U`. The physical register `regfile(0)` is optimized away entirely by Chisel synthesis.
- **Scoreboard Exclusion**: Scoreboard setting and clearing logic specifically filters out bit 0. Writes targeting register 0 do not register a dependency interlock.
- **Debug Collection**: A dedicated signal `x0` tracks writes attempting to target index 0 (e.g., standard `nop` instructions) strictly for debugging and performance counters without affecting state.

[Primary Source: `hdl/chisel/src/coralnpu/scalar/Regfile.scala` - Zero register hardwiring optimizations]

## ReadSet Injection Interface (QA-RF-006)

The register file provides a dedicated sideband injection interface `io.readSet` for each read port.

### Interface Behavior and Override
- If `io.readSet(i).valid` is asserted, the register file ignores the output of the physical register file (`rwdata`) and combinationally overrides `nxtReadDataBits` with `io.readSet(i).value`.
- This injection takes absolute precedence, allowing external sources (such as debug rings, bypass loops, or special execution units) to dynamically override operand fetches combinationally.

```scala
nxtReadDataBits(i) := Mux(io.readSet(i).valid, io.readSet(i).value, rwdata(i))
```

[Primary Source: `hdl/chisel/src/coralnpu/scalar/Regfile.scala` - readSet override multiplexing logic]

--------------------------------------------------------------------------------

**Provenance & Traceability**
- **Verified As Of:** 2026-07-25
- **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7)
- **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Regfile.scala`
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
