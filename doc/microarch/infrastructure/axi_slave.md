# AXI4 Slave Interface

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

> **Intended Audience:** Hardware Developers, HW Integrators

The `AxiSlave` module acts as the primary host-facing bridge, explicitly translating host transactions into the internal `FabricIO` protocol used by the CoralNPU. The interface implements the AXI4 protocol as the exclusive host interface. It manages read/write arbitration and backpressure handling.

## Interface Specifications

| Interface Group    | Description                                                                                            |
| ------------------ | ------------------------------------------------------------------------------------------------------ |
| `io.axi4`          | Flipped `Axi4MasterIO` representing the external AXI4 interface.                                       |
| `io.fabric`        | Internal `FabricIO` interface connecting to the on-chip memory fabric.                                 |
| `io.txnInProgress` | Output flag asserted while any AXI4 command is actively being processed.                               |
| `io.periBusy`      | Input flag from the fabric. When asserted, the slave stalls and does not accept new AXI4 transactions. |

## Operational Behavior

The `AxiSlave` implements the following mechanisms:

### Arbitration and Dispatch

- **Read/Write Fairness**: Employs a 2-way Round-Robin Arbiter (`CoralNPURRArbiter`) to select between pending Read Address (`AR`) and Write Address (`AW`) requests.
- **Backpressure Handling**: The slave evaluates `io.periBusy`. If the internal fabric asserts busy, AXI4 transfers are paused, and no read/write commands are issued internally until the fabric is ready.

### Response Codes

AXI4 response codes (`BRESP` and `RRESP`) are mapped from the internal fabric validity signals:

- `OKAY` (0x0): Asserted when the fabric successfully acknowledges a write or returns valid read data.
- `SLVERR` (0x2): Asserted if the fabric indicates an error condition during the transaction.

<!-- mdformat off -->

<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

<!-- prettier-ignore-end -->

**Provenance & Traceability** - **Verified As Of:** 2026-07-22 - **Upstream Commit:** 5c2647afd951f70d6244ea06b5a8b7fa1fdf2918 - **Primary Source(s):** `hdl/chisel/src/coralnpu/AxiSlave.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
