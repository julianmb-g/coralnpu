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

# Vector backend ALU array

> **Intended Audience:** HW Devs

The Vector Backend ALU (`rvv_backend_alu`) manages the multi-lane integer arithmetic pipeline within the Vector Core. It arbitrates and issues micro-operations (uops) from the ALU Reservation Station to multiple underlying ALU units (`rvv_backend_alu_unit`).

## Structural overview

The integer ALU array consists of a configurable number of execution lanes, defined by the `` `NUM_ALU `` parameter. The top-level wrapper is responsible for receiving valid uops and routing them correctly based on current availability and operation type.

### Lane arbitration and multi-issue

The `rvv_backend_alu` uses a combinatorial block decoding `result_ready` from the ROB to determine which ALU units are available for multi-issue dispatch.

- **ALU Unit 0 (`u_alu_cmp_unit`)**: This is the primary execution lane. It is explicitly parameterized with `CMP_SUPPORT (1'b1)` to handle comparison instructions.

- **ALU Unit 1...N (`u_alu_unit`)**: These lanes handle standard arithmetic logic. The arbitration logic explicitly prevents comparison operations (`!uop[x].is_cmp`) from being routed to these secondary units, forcing all compares into Unit 0.

If both ALU 0 and ALU 1 are ready (`result_ready == 2'b11`), the dispatcher will issue to both lanes concurrently, provided the operation directed to ALU 1 is not a comparison.

## Interfaces and boundaries

| Port | Direction | Description |
| :--- | :--- | :--- |
| `clk` | Input | Clock signal |
| `rst_n` | Input | Active-low reset |
| `pop` | Output | ALU Reservation Station pop signal |
| `uop_valid` | Input | Valid uop signal from reservation station |
| `uop` | Input | ALU micro-operation data |
| `result_valid` | Output | Valid calculation result |
| `result` | Output | Result data to ROB |
| `result_ready` | Input | Ready signal from ROB |
| `trap_flush_rvv` | Input | Flush vector pipeline signal on trap |

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_alu.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
