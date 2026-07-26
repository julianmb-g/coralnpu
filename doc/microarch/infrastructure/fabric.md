# Internal Memory Fabric

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

> **Intended Audience:** HW Devs, HW Integrators

## Overview

The Internal Memory Fabric (`Fabric`) serves as the low-latency interconnect within the CoralNPU core, arbitrating access from various bus masters to shared memory resources, primarily the Tightly Coupled Memory (TCM). It consists of arbitration (`FabricArbiter`) and routing (`FabricMux`) components.

## Arbitration Scheme (`FabricArbiter`)

The `FabricArbiter` implements a strict static/fixed-priority arbitration scheme where lower-indexed ports take precedence over higher-indexed ones.

For TCM access, the fabric maps to 3 functional ports in decreasing order of priority:

- **Port 0 (Highest Priority)**: Core fast-path.
- **Port 1**: External `AxiSlave`.
- **Port 2 (Lowest Priority)**: Debug Module.

Backpressure is provided to lower-priority buses via the `fabricBusy` signals.

## Routing and Multiplexing (FabricMux)

The `FabricMux` routes fabric commands from the source to the appropriate port based on the target address and memory region configuration.

### 1-Cycle Read Delay Logic

The `FabricMux` delays the **selection** of the read data response path by one clock cycle.

This is managed via the `lastReadSelected` register. When a read command is issued and a port is successfully selected (`portSelected(i) && io.source.readDataAddr.valid`), the index of the selected port is registered in `lastReadSelected`.

In the following cycle (Cycle N+1), the `FabricMux` uses this registered state (`lastReadSelected`) to multiplex the read data (`io.ports(i).readData`) back to the source (`io.source.readData`). Note that there is no pipeline register on the `readData` path itself; the delay is introduced purely via the registered selection logic, assuming the peripheral drives valid data in the cycle following the request.

Write responses (`writeResp`) are multiplexed combinationally based on the current `portSelected` state.

## Interfaces

The Fabric uses `FabricIO` which contains request and response channels for both read and write operations.

### Simultaneous Read/Write Constraint

A single `FabricIO` initiator is strictly prohibited from asserting both `readDataAddr.valid` and `writeDataAddr.valid` in the same clock cycle. This mutual exclusivity constraint is explicitly enforced via hardware assertions in both `FabricMux` and `FabricArbiter`. Violating this constraint will cause the hardware assertions to fail and halt execution.

### Backpressure & Busy Signal Propagation

The `FabricMux` implements a combinational flow control and backpressure mechanism to manage transaction propagation between the initiator source and downstream peripherals:

1. **Transaction Blocking**: When a downstream peripheral is busy (indicated by its `io.periBusy` signal), the multiplexer blocks command forwarding to that specific port. New requests are held and not forwarded to the busy module until it becomes ready.
2. **Initiator Backpressure (`io.fabricBusy`)**: Simultaneously, the multiplexer routes the busy state of the targeted peripheral directly back to the initiator via the `io.fabricBusy` signal.
3. **Combinational Stall**: This feedback loop stalls the transaction at the fabric boundary, requiring the initiator to hold its request valid and unchanged on the `io.source` port until `io.fabricBusy` is deasserted.

## Bus Access Mutual Exclusivity (ADR-045)

Accesses from the Data Bus (D-Bus), Instruction Bus (I-Bus), and External Bus (E-Bus) are mutually exclusive per cycle. The implementation guarantees that only one of these buses can successfully fire an access to a single TCM port in any given clock cycle.

As explicitly enforced in the Load-Store Unit (`Lsu.scala`), the condition `assert(PopCount(Seq(ibusFired, dbusFired, ebusFired)) <= 1.U)` guarantees that D-Bus, I-Bus, and E-Bus accesses to the TCM cannot occur in the same cycle.

<!-- mdformat off -->

<!-- prettier-ignore -->

--------------------------------------------------------------------------------

**Provenance & Traceability**
- **Verified As Of:** 2026-07-25
- **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7)
- **Primary Source(s):** `hdl/chisel/src/coralnpu/scalar/Lsu.scala`, `hdl/chisel/src/coralnpu/Fabric.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
