# Vector Register File (VRF)

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
> **Intended Audience:** Hardware Developers

The Vector Register File (VRF) is the primary architectural state storage for the vector execution backend. It provides high-bandwidth, multi-ported access to the 32 architectural vector registers.

## Capacity and Layout

The VRF is physically instantiated as an array of 32 registers, each `VLEN` bits wide.
The core storage is managed by the `rvv_backend_vrf_reg` module.

## Port Configuration

The VRF exposes multiple read and write ports.

### Read Ports

The VRF supports 4 explicit read port configurations. Data is unpacked from the core register array and routed to specific functional units:

- **Dispatch Read Ports:** 4 read ports (`NUM_DP_VRF`) are provided to the Vector Dispatch unit (`vrf2dp_rd_data`), accessed via the `dp2vrf_rd_index` array.
- **V0 Dedicated Port:** A dedicated `VLEN`-wide read port (`vrf2dp_v0_data`) permanently outputs the contents of `v0`. This provides zero-latency access to the architectural mask register for masked vector operations.
- **Permutation Read Port:** A dedicated read port (`vrf2pmt_rd_data`) is provided to the Permutation unit, accessed via `pmt2vrf_rd_index`.

### Write Ports

The VRF supports 4 parallel write ports (`NUM_RT_UOP`) from the vector retirement logic.

- **Write Unpacking:** The incoming `RT2VRF_t` structures are unpacked into valid bits, destination addresses, write data, and byte strobes (`rt_strobe`).
- **Bit-Enable Generation:** The byte strobes are expanded into bit-level write enables (`wr_web`).
- **Retire Data Merging:** The writes from all active retirement ports are merged combinatorially using logical OR operations across the `NUM_RT_UOP` ports before being applied to the `rvv_backend_vrf_reg` core. This relies on the upstream Vector Retire unit to properly resolve Write-After-Write (WAW) hazards, ensuring no two retirement slots attempt to write to the same `VLEN` bits of the same register simultaneously.

## Interfaces

| Signal             | Direction | Width                                       | Description                                             |
| :----------------- | :-------- | :------------------------------------------ | :------------------------------------------------------ |
| `clk`              | Input     | 1-bit                                       | Global clock signal.                                    |
| `rst_n`            | Input     | 1-bit                                       | Global active-low asynchronous reset signal.            |
| `dp2vrf_rd_index`  | Input     | ``NUM_DP_VRF` * ``REGFILE_INDEX_WIDTH` bits | Read indices from Dispatch unit.                        |
| `vrf2dp_rd_data`   | Output    | ``NUM_DP_VRF` * ``VLEN` bits                | Read data to Dispatch unit.                             |
| `vrf2dp_v0_data`   | Output    | ``VLEN` bits                                | Dedicated read data for register v0 to Dispatch unit.   |
| `pmt2vrf_rd_index` | Input     | ``REGFILE_INDEX_WIDTH` bits                 | Read index from Permutation unit.                       |
| `vrf2pmt_rd_data`  | Output    | ``VLEN` bits                                | Read data to Permutation unit.                          |
| `rt2vrf_wr_valid`  | Input     | ``NUM_RT_UOP` bits                          | Write valid signals from Retire unit.                   |
| `rt2vrf_wr_data`   | Input     | ``NUM_RT_UOP` `RT2VRF_t` packets            | Write data packets from Retire unit.                    |
| `vrf_data`         | Output    | ``NUM_VRF` * ``VLEN` bits                   | Full VRF state dump for verification (TB_SUPPORT only). |

<!-- mdformat off -->

<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

<!-- prettier-ignore-end -->

> **Provenance & Traceability** - **Verified As Of:** 2026-07-03 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_vrf.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
