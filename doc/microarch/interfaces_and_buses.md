# Interfaces and Buses Overview

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

> **Intended Audience:** HW Devs


## System Architecture Overview

For a visual representation of the top-level CoralNPU SoC Subsystem architecture, please refer to the [Top-Level Subsystem](../top_level_subsystem.md) documentation.

This document aggregates the high-level API/ABI contracts and centralizes the architectural blueprint for the CoralNPU's memory buses and interfaces, ensuring a clean "Ship-to-SoC" boundary.

## API and ABI Contracts

### Host-Facing AXI4 (`CoreAxiCSR`)

- `0x000 - 0x008`: Core Control & Status (`RESET`, `PC_START`, `STATUS`).
- `0x100 - 0x120`: 9 Specific Internal Metrics.

### Internal RISC-V CSRs (`Csr.scala`)

- Standard RISC-V 12-bit addresses, custom registers (`MCONTEXT0-7` at `0x7C0`, `KISA` at `0xFC0`).
- **`mstatush` Contract**: Mapped at `0x310`. The `mstatush` CSR is implemented to maintain compatibility. It explicitly returns `0.U(32.W)` on all reads and safely ignores all writes without triggering an exception.

### Instruction Decode & Pipeline Scheduling (`Decode.scala`)

- **Float Intra-Cycle RAW/WAW Hazards**: The ABI assumes strict superscalar dispatch fencing. The hardware guarantees that a floating-point load and a dependent floating-point operation (or dual float writes) decoded simultaneously will be structurally fenced via `floatReadAfterWrite` and `floatWriteAfterWrite` interlocks, forcing the dependent instruction to the next cycle.
- **FMA `rs2` Dependency Contract**: The decoder's `uses_rs2` mask explicitly tracks `MADD`, `MSUB`, `NMADD`, and `NMSUB`. Compilers must assume 3-source operand dependencies are accurately tracked for RAW hazard mitigation without software `NOP` padding.

## Memory Buses

The core execution units communicate via a set of decoupled internal buses, defined in `Interfaces.scala`:

### `IBusIO` (Instruction Bus)

Used by the `FetchUnit` to retrieve instructions from the memory subsystem (ITCM or external host memory via IBus2Axi bridge).

- **Control Phase**: `valid`, `ready`, `addr` (Address bits).
- **Read Phase**: `rdata` (Instruction data), `fault` (Fault information).
- **Handshake Constraint**: When `valid` is asserted, `addr` must remain constant until `ready` is fired.

### `DBusIO` (Data Bus)

Used by the Scalar and Vector Load/Store units to perform read/write operations against the DTCM or external memory.

- **Control Phase**: `valid`, `ready`, `write`, `pc`, `addr`, `adrx`, `size`, `wdata`, `wmask`.
- **Read Phase**: `rdata` (Read data returned from memory).

### `EBusIO` (External Bus)

Wraps the `DBusIO` to provide routing information, indicating whether an access is `internal` (TCM) or requires an external translation, along with associated `fault` signals.

### `FabricIO` (Low-Latency Internal Fabric)

A simplified, low-latency protocol used by the `AxiSlave` to translate host AXI4 accesses to internal components like the TCM and CSRs.

- **Timing Contract**: `FabricIO` drops the standard AXI4 `valid`/`ready` handshake in favor of deterministic latency. In-band backpressure is not supported; instead, an out-of-band `periBusy` signal must be used to stall the master prior to transaction issuance.
- **Read Channel**: The master asserts `readDataAddr` (Valid + Addr). The slave must return the `readData` (Valid + Data) exactly **one cycle later**.
- **Write Channel**: The master asserts `writeDataAddr` (Valid + Addr), `writeDataBits`, and `writeDataStrb` simultaneously. The slave returns `writeResp` to indicate success (translated to `OKAY` or `SLVERR`).

## AXI4 Bus Signals and Defaults

The CoralNPU utilizes a standardized AXI4 implementation for external memory and high-performance internal interconnects. The `AxiAddress` bundle defines the signals for both Read and Write Address channels.

### `AxiAddress` Default Values

To ensure deterministic behavior when optional AXI4 signals are not explicitly driven, the hardware enforces the following default values (as defined in `Axi.scala`):

| Signal       | Width  | Default Value | Description                                                               |
| :----------- | :----- | :------------ | :

--------------------------------------------------------------------------------

**Provenance & Traceability**
- **Verified As Of:** 2026-07-25
- **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7)
- **Primary Source(s):** `hdl/chisel/src/coralnpu/Interfaces.scala`, `hdl/chisel/src/coralnpu/CoreAxi.scala`, `hdl/chisel/src/bus/Axi.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
