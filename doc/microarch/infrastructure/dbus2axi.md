# DBus to AXI Bridge (DBus2Axi)

<!--
 Copyright 2024 Google LLC

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

> **Intended Audience:** Hardware Integrators, Microarchitecture Developers

The `DBus2Axi` (implemented as `DBus2AxiV2`) module serves as the translation bridge between the internal CoralNPU Data Bus (`DBusIO`) and the external AXI Master interface (`AxiMasterIO`). It explicitly manages the decoupled translation of load/store requests from the core's Load-Store Unit (LSU) to the system memory interconnect.

## Interface Translation & State Machines

The bridge operates by explicitly decoupling the address, data, and response phases of the AXI protocol.

### Read Path (Load Instructions)

- The read path translates a `dbus.valid && !dbus.write` request into an AXI `AR` channel transaction.
- The AXI protection level (`ARPROT`) is hardcoded to `2` (`0b010`, unprivileged non-secure data access).
- A state latch (`rdataReceived`) captures the incoming read data.
- A 1-cycle delay register (`readNext`) buffers the read data before presenting it to `dbus.rdata` upon the assertion of `dbus.ready`.

### Write Path (Store Instructions)

- The write path translates a `dbus.valid && dbus.write` request into concurrent AXI `AW` and `W` channel transactions.
- Similar to the read path, `AWPROT` is hardcoded to `2`.
- Write data (`wdata`) and strobes (`wmask`) are buffered through a 2-entry `Queue`.
- The module waits for the AXI `B` (Response) channel transaction before signaling `dbus.ready` back to the core.

## Fault Handling

The bridge monitors the response signals (`RRESP` and `BRESP`). If a response from the interconnect indicates a failure (i.e., `resp != AxiResponseType.OKAY`), the bridge asserts the `fault.valid` interface.

The `FaultInfo` payload propagated back to the core contains:

- `write`: Boolean indicating if the fault occurred on a store.
- `addr`: The memory address that caused the fault.
- `epc`: The architectural Program Counter (`dbus.pc`) associated with the failing transaction.

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-06 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/chisel/src/coralnpu/DBus2Axi.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
