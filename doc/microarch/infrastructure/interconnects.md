# Interconnects & Interfaces

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

> **Intended Audience:** Hardware Developers, System Integrators

## Seamless Facade (AXI4 Boundary)

The CoralNPU IP enforces a strict "Seamless Facade" by exposing only standard AXI4 interfaces (`axi_slave` and `axi_master`) at the top-level `CoreAxi` wrapper. All internal bridging, protocol translation, and subsystem-specific interconnects are strictly encapsulated within the core wrapper. SoC integrators must interface exclusively with the provided AXI4 ports, ensuring that the IP can be integrated into standard AMBA fabrics without exposing internal, platform-specific protocols.

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

The CoralNPU relies on standard AXI4 interfaces for external communication, providing a decoupled, high-bandwidth bridge to the internal NPU core which utilizes a custom, low-latency `FabricIO` protocol.

## CoreAxi Wrapper

The `CoreAxi` wrapper serves as the definitive top-level IP boundary explicitly designed for SoC integrators. It comprehensively encapsulates the core pipeline, TCMs, and internal bus translations, exposing only standard AXI4 interfaces.

### External AXI4 Interfaces

The IP explicitly exports two distinct AXI4 interfaces:

- **`axi_slave`**: Used by the host SoC to control the NPU, access the CSRs, and perform backdoor SRAM loading.
- **`axi_master`**: Used by the NPU Core to initiate memory transactions to the broader SoC (e.g., fetching instructions not in ITCM or accessing external DDR memory).

**Bus Widths:**

- **Data Width**: 256-bit data buses (`p.axi2DataBits`).
- **Address Width**: 32-bit physical addresses (`p.axi2AddrBits`).

### Read Data Skid Buffer

To maintain AXI compliance and decouple internal backpressure from the host interface, the `CoreAxi` wrapper integrates a 2-entry `readDataSkid` queue on the master read data channel. This skid buffer ensures that if the host SoC stalls the read data response, the internal data path does not lose critical read responses, preventing transaction dropping and simplifying backpressure resolution at the core boundary.

## AxiSlave to FabricIO Translation

The `AxiSlave` module translates incoming AXI4 transactions from the host into the internal `FabricIO` protocol.

### Arbitration and Dispatch

Read and Write address channels from the AXI4 master are arbitrated using a simulation-safe Round-Robin Arbiter (`CoralNPURRArbiter`). This ensures fairness between host read and write requests entering the core.

### Transaction Support

The `AxiSlave` supports standard AXI4 single-beat transactions. Multi-beat bursts or transaction IDs are not supported at this boundary to maintain integration simplicity and compliance with the AXI4 specification.

### Backpressure and Stall Logic

The `AxiSlave` respects the `periBusy` signal from the internal `FabricMux`. If the target internal peripheral (e.g., TCM) is busy, the `AxiSlave` stalls the AXI4 transaction without dropping data, waiting until `periBusy` is de-asserted before asserting the `FabricIO` valid signals. This low-latency translation abstracts the internal `FabricIO` constraints from the host SoC.

## Internal SRAM Adapter

The `Internal SRAM Adapter` translates incoming internal bus transactions into direct [SRAM](../memory/sram.md) accesses.

### Latency Optimization

The adapter is optimized to eliminate multi-cycle queuing latency:

- **Direct Routing:** Request channels are routed directly to the SRAM interface.
- **Pipelined Metadata:** Metadata tracking uses a `Pipe` with latency 1.
- **Skid Buffering:** The output response channel features a 1-entry skid buffer with flow/pipe enabled to maintain bus compliance while decoupling backpressure.
- **Handshaking Redesign:** Handshaking logic is redesigned to prevent overflow on host stalls, ensuring backpressure is propagated correctly without dropping requests.

### Constraints

- **Single Outstanding:** The `Internal SRAM Adapter` is NOT designed to handle multi-outstanding read transactions. Any downstream arbitration or buffering must adhere to this single-outstanding constraint.

## CoreTlul Wrapper

The `CoreTlul` wrapper provides an alternative top-level interface exposing physical **TLUL** (TileLink Uncached Lite) ports, wrapping the internal `CoreAxi` pipeline. This wrapper ensures compatibility with OpenTitan-style interconnects while maintaining the same core microarchitecture.

### External TLUL Interfaces

The wrapper exposes two distinct TLUL interfaces:

- **`tl_host`**: The NPU acts as a TLUL Host (Master) to initiate transactions to the broader system. Internally, this bridges to the `axi_master` port of the `CoreAxi` pipeline via an `Axi2TLUL` adapter.
- **`tl_device`**: The NPU acts as a TLUL Device (Slave) receiving control and data transactions from the host system. Internally, this bridges to the `axi_slave` port of the `CoreAxi` pipeline via a `TLUL2Axi` adapter.

#### TLUL Port Signals (tl_host - NPU as Host)

| Port      | Channel | Signal    | Sub-field    | Direction | Width                        | Description           |
| --------- | ------- | --------- | ------------ | --------- | ---------------------------- | --------------------- |
| `tl_host` | A (Req) | `valid`   |              | Output    | 1                            | Request valid         |
|           |         | `ready`   |              | Input     | 1                            | Request ready         |
|           |         | `opcode`  |              | Output    | 3                            | Operation code        |
|           |         | `param`   |              | Output    | 3                            | Operation parameters  |
|           |         | `size`    |              | Output    | `log2Ceil(p.axi2DataBits/8)` | Transaction size      |
|           |         | `source`  |              | Output    | `p.axi2IdBits`               | Transaction source ID |
|           |         | `address` |              | Output    | `p.axi2AddrBits`             | Byte address          |
|           |         | `mask`    |              | Output    | `p.axi2DataBits/8`           | Byte mask             |
|           |         | `data`    |              | Output    | `p.axi2DataBits`             | Write data            |
|           |         | `user`    | `rsvd`       | Output    | 5                            | Reserved              |
|           |         |           | `instr_type` | Output    | 4                            | Instruction type      |
|           |         |           | `cmd_intg`   | Output    | 7                            | Command integrity     |
|           |         |           | `data_intg`  | Output    | 7                            | Data integrity        |
|           | D (Rsp) | `valid`   |              | Input     | 1                            | Response valid        |
|           |         | `ready`   |              | Output    | 1                            | Response ready        |
|           |         | `opcode`  |              | Input     | 3                            | Response code         |
|           |         | `param`   |              | Input     | 3                            | Response parameters   |
|           |         | `size`    |              | Input     | `log2Ceil(p.axi2DataBits/8)` | Transaction size      |
|           |         | `source`  |              | Input     | `p.axi2IdBits`               | Transaction source ID |
|           |         | `sink`    |              | Input     | 1                            | Transaction sink ID   |
|           |         | `data`    |              | Input     | `p.axi2DataBits`             | Read data             |
|           |         | `user`    | `rsp_intg`   | Input     | 7                            | Response integrity    |
|           |         |           | `data_intg`  | Input     | 7                            | Data integrity        |
|           |         | `error`   |              | Input     | 1                            | Error flag            |

#### TLUL Port Signals (tl_device - NPU as Device)

| Port        | Channel | Signal    | Sub-field    | Direction | Width                        | Description           |
| ----------- | ------- | --------- | ------------ | --------- | ---------------------------- | --------------------- |
| `tl_device` | A (Req) | `valid`   |              | Input     | 1                            | Request valid         |
|             |         | `ready`   |              | Output    | 1                            | Request ready         |
|             |         | `opcode`  |              | Input     | 3                            | Operation code        |
|             |         | `param`   |              | Input     | 3                            | Operation parameters  |
|             |         | `size`    |              | Input     | `log2Ceil(p.axi2DataBits/8)` | Transaction size      |
|             |         | `source`  |              | Input     | `p.axi2IdBits`               | Transaction source ID |
|             |         | `address` |              | Input     | `p.axi2AddrBits`             | Byte address          |
|             |         | `mask`    |              | Input     | `p.axi2DataBits/8`           | Byte mask             |
|             |         | `data`    |              | Input     | `p.axi2DataBits`             | Write data            |
|             |         | `user`    | `rsvd`       | Input     | 5                            | Reserved              |
|             |         |           | `instr_type` | Input     | 4                            | Instruction type      |
|             |         |           | `cmd_intg`   | Input     | 7                            | Command integrity     |
|             |         |           | `data_intg`  | Input     | 7                            | Data integrity        |
|             | D (Rsp) | `valid`   |              | Output    | 1                            | Response valid        |
|             |         | `ready`   |              | Input     | 1                            | Response ready        |
|             |         | `opcode`  |              | Output    | 3                            | Response code         |
|             |         | `param`   |              | Output    | 3                            | Response parameters   |
|             |         | `size`    |              | Output    | `log2Ceil(p.axi2DataBits/8)` | Transaction size      |
|             |         | `source`  |              | Output    | `p.axi2IdBits`               | Transaction source ID |
|             |         | `sink`    |              | Output    | 1                            | Transaction sink ID   |
|             |         | `data`    |              | Output    | `p.axi2DataBits`             | Read data             |
|             |         | `user`    | `rsp_intg`   | Output    | 7                            | Response integrity    |
|             |         |           | `data_intg`  | Output    | 7                            | Data integrity        |
|             |         | `error`   |              | Output    | 1                            | Error flag            |

<!-- mdformat off -->

<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

<!-- prettier-ignore-end -->

> **Provenance & Traceability** - **Verified As Of:** 2026-07-06 - **Upstream Commit:** e0a6c21ae9cf6ad4d578c0adf1256e4e7a21d2a8 - **Primary Source(s):** `hdl/chisel/src/coralnpu/CoreAxi.scala:L23`, `hdl/chisel/src/coralnpu/AxiSlave.scala:L42`, `hdl/chisel/src/bus/Axi.scala:L124`, `hdl/chisel/src/coralnpu/CoreTlul.scala:L20`, `hdl/chisel/src/bus/TileLinkUL.scala:L22` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
