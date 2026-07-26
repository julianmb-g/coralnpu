# Generic Skid Buffer

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

The Generic Skid Buffer concept is applied in the `CoreAxi` wrapper (defined in `hdl/chisel/src/coralnpu/CoreAxi.scala`) to manage protocol conversion between the internal `FabricIO` and external AXI4 host interface. Specifically, a 2-entry buffer is used on the AXI Read Data channel (R) to decouple handshakes and prevent backpressure deadlocks.

## Architecture

In the implemented reality of the RTL, this "Skid Buffer" function is realized using a 2-entry `Queue` instance (`readDataSkid`) from the standard Chisel utility library.

- **Decoupling:** It ensures that up to two outstanding read responses can be buffered if the internal fabric or the host interface is stalled.
- **Handshake Separation:** It allows the AXI `RVALID`/`RREADY` handshake to operate independently of internal fabric response timing, preventing protocol violations or stalls in the internal fabric.
- **Routing:** Read data is routed back to either the Instruction Bus (`ibus`) or Execution Bus (`ebus`) based on the transaction ID stored in the buffer.

## Interfaces

The buffer operates on the AXI4 Read Data channel interface within `CoreAxi`.

| Signal                    | Direction             | Description                                                         |
| :------------------------ | :-------------------- | :

--------------------------------------------------------------------------------

**Provenance & Traceability**
- **Verified As Of:** 2026-07-25
- **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7)
- **Primary Source(s):** `hdl/chisel/src/coralnpu/CoreAxi.scala:L254-260` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
