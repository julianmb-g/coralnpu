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

# Vector register file (VRF)

> **Intended Audience:** HW Devs

The Vector Register File (VRF) is the primary architectural state storage for the vector execution backend. It provides high-bandwidth, multi-ported access to the 32 architectural vector registers.

## Capacity and layout

The VRF is physically instantiated as an array of 32 registers, each `VLEN` bits wide (VLEN = 128).
The core storage is managed by the `rvv_backend_vrf_reg` module.

### Vector length multiplier (LMUL)

The effective size of vector registers and the number of elements they can hold are influenced by the `LMUL` (Vector Length Multiplier) setting in the `vtype` CSR. LMUL allows the vector registers to be grouped to form larger effective registers.

- **LMUL Encoding:** The `lmul` field in `vtype` is 3 bits wide.

- **Fractional LMUL:** Supported, allowing for register sizes smaller than `VLEN`.

- **Register Grouping:** When LMUL > 1, multiple adjacent vector registers are treated as a single, larger register. For example, with LMUL=2, registers v0 and v1 form a group, v2 and v3 form another, and so on.

- **Hardware Support:** The `RvvCore.scala` and `RvvInterface.scala` modules manage the `lmul` and `lmul_orig` signals, which are used by the vector decode and dispatch logic to determine the effective register size and element count for vector operations.

[Source: [`hdl/chisel/src/coralnpu/rvv/RvvCore.scala`](../../../hdl/chisel/src/coralnpu/rvv/RvvCore.scala), [`hdl/chisel/src/coralnpu/rvv/RvvInterface.scala`](../../../hdl/chisel/src/coralnpu/rvv/RvvInterface.scala)]

## Port configuration

The VRF exposes multiple read and write ports.

### Read ports

The VRF supports 4 explicit read port configurations. Data is unpacked from the core register array and routed to specific functional units:

- **Dispatch Read Ports:** 4 read ports (`NUM_DP_VRF`) are provided to the Vector Dispatch unit (`vrf2dp_rd_data`), accessed via the `dp2vrf_rd_index` array.

- **V0 Dedicated Port:** A dedicated `VLEN`-wide read port (`vrf2dp_v0_data`) permanently outputs the contents of `v0`. This provides zero-latency access to the architectural mask register for masked vector operations.

- **Permutation Read Port:** A dedicated read port (`vrf2pmt_rd_data`) is provided to the Permutation unit, accessed via `pmt2vrf_rd_index`.

### Write ports

The VRF supports 4 parallel write ports (`NUM_RT_UOP`) from the vector retirement logic.

- **Write Unpacking:** The incoming `RT2VRF_t` structures are unpacked into valid bits, destination addresses, write data, and byte strobes (`rt_strobe`).

- **Bit-Enable Generation:** The byte strobes are expanded into bit-level write enables (`wr_web`).

- **Retire Data Merging:** The writes from all active retirement ports are merged combinatorially using logical OR operations across the `NUM_RT_UOP` ports before being applied to the `rvv_backend_vrf_reg` core. This relies on the upstream Vector Retire unit to properly resolve Write-After-Write (WAW) hazards, ensuring no two retirement slots attempt to write to the same `VLEN` bits of the same register simultaneously.

## Interfaces

| Port Name | Direction | Type | Description |
| :--- | :--- | :--- | :--- |
| `clk` | Input | `logic` | Clock signal. |
| `rst_n` | Input | `logic` | Active-low reset signal. |
| `dp2vrf_rd_index` | Input | `logic [NUM_DP_VRF-1:0][REGFILE_INDEX_WIDTH-1:0]` | Read indices for dispatch units. |
| `vrf2dp_rd_data` | Output | `logic [NUM_DP_VRF-1:0][VLEN-1:0]` | Read data for dispatch units. |
| `vrf2dp_v0_data` | Output | `logic [VLEN-1:0]` | Dedicated read data for `v0`. |
| `pmt2vrf_rd_index` | Input | `logic [REGFILE_INDEX_WIDTH-1:0]` | Read index for permutation unit. |
| `vrf2pmt_rd_data` | Output | `logic [VLEN-1:0]` | Read data for permutation unit. |
| `rt2vrf_wr_valid` | Input | `logic [NUM_RT_UOP-1:0]` | Valid signals for retirement writes. |
| `rt2vrf_wr_data` | Input | `RT2VRF_t [NUM_RT_UOP-1:0]` | Data/strobe signals for retirement writes. |

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** [hdl/verilog/rvv/design/rvv_backend_vrf.sv](../../../hdl/verilog/rvv/design/rvv_backend_vrf.sv) - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
