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

# DBus to AXI bridge (dbus2axi)

> **Intended Audience:** HW Integrators, HW Devs

The `DBus2Axi` (implemented as `DBus2AxiV2`) module serves as the translation bridge between the internal CoralNPU Data Bus (`DBusIO`) and the external AXI Master interface (`AxiMasterIO`). It explicitly manages the decoupled translation of load/store requests from the core's Load-Store Unit (LSU) to the system memory interconnect.

## Interface translation & state machines

The bridge operates by explicitly decoupling the address, data, and response phases of the AXI protocol.

### Read path (load instructions)

- The read path translates a `dbus.valid && !dbus.write` request into an AXI `AR` channel transaction.

- The AXI protection level (`ARPROT`) is hardcoded to `2` (`0b010`, unprivileged non-secure data access).

- A state latch (`rdataReceived`) captures the incoming read data.

- A 1-cycle delay register (`readNext`) buffers the read data before presenting it to `dbus.rdata` upon the assertion of `dbus.ready`.

### Write path (store instructions)

- The write path translates a `dbus.valid && dbus.write` request into concurrent AXI `AW` and `W` channel transactions.

- Similar to the read path, `AWPROT` is hardcoded to `2`.

- Write data (`wdata`) and strobes (`wmask`) are buffered through a 2-entry `Queue`.

- The module waits for the AXI `B` (Response) channel transaction before signaling `dbus.ready` back to the core.

## AXI4 burst capabilities & constraints

The `DBus2Axi` bridge translates single-beat data bus (`DBusIO`) requests into AXI4 transactions.

- **Burst Length**: All translated transactions are issued as single-beat bursts (`AWLEN = 0`, `ARLEN = 0`).
- **Burst Type**: The bridge defaults to `INCR` (incrementing) burst types, though the length is strictly 1. Multi-beat burst transactions are not supported directly by the bridge as the internal `DBusIO` operates on single-beat semantics.

## Fault handling

The bridge monitors the response signals (`RRESP` and `BRESP`). If a response from the interconnect indicates a failure (i.e., `resp != AxiResponseType.OKAY`), the bridge asserts the `fault.valid` interface.

The `FaultInfo` payload propagated back to the core contains:

- `write`: Boolean indicating if the fault occurred on a store.

- `addr`: The memory address that caused the fault.

- `epc`: The architectural Program Counter (`dbus.pc`) associated with the failing transaction.

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/coralnpu/DBus2Axi.scala`, `hdl/chisel/src/coralnpu/DBus2AxiTest.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
