# Vector Backend Multiplier Unit

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

The Vector Backend Multiplier Unit (`rvv_backend_mul_unit.sv`) and its Arbitration Wrapper (`rvv_backend_mulmac.sv`) are responsible for executing vector multiplication, scaling, and MAC (Multiply-Accumulate) operations within the CoralNPU's Vector Core.

## Component Topology and Element Routing

The multiplier core is structurally implemented as a tiled matrix of 8-bit multiplier primitives (`rvv_backend_mul_unit_mul8`). The physical topology routes data across a 4x4 array (16 instances in total per block).

Before multiplication, the 128-bit vector data (`VLEN=128`) is segmented into 16x8 sub-elements. The unit dynamically manages signed/unsigned extensions (`mul_src1_is_signed`, `mul_src2_is_signed`) based on the execution element width (EEW).

### Supported Operand Sizes (EEW)

The datapath calculates and widens results natively for different EEWs:

- **EEW8**: Full result is 16-bit. Widen results fit into 256 bits, while standard operations optionally keep the low 8 bits.
- **EEW16**: Full result is 32-bit. It aggregates partial products from the 8-bit multipliers.
- **EEW32**: Full result is 64-bit. It aggregates a deeper hierarchy of partial products from the 8-bit multipliers.

Rounding and saturation (`vsmul.vv`, `vsmul.vx`) are natively applied by calculating `vsmul_round_incr` and monitoring `vsmul_sat` constraints to update the `vxsat` CSR.

## Arbitration Wrapper (`mulmac`)

The `rvv_backend_mulmac.sv` wrapper instantiates `NUM_MUL` Vector MAC Units (`rvv_backend_mac_unit`). It acts as a structural hazard arbitrator, dispatching micro-operations from the MUL Reservation Station (RS) to the available execution units.

- **Handshake Protocol**: Readiness is asserted if an execution unit is free or if the Reorder Buffer (ROB) can accept the previous result: `mac_ready = ~res_valid_ex2rob | res_ready_rob2ex`.
- **Arbitration Logic**:
  - If only EU0 is ready (`2'b01`), `uop_valid_rs2ex[0]` is routed exclusively to EU0.
  - If only EU1 is ready (`2'b10`), `uop_valid_rs2ex[0]` is routed to EU1.
  - If both are ready (`2'b11`), concurrent dispatch occurs to both units.

## Interfaces

### `rvv_backend_mul_unit` Interfaces

| Signal              | Direction | Width             | Description                                                         |
| :------------------ | :-------- | :---------------- | :------------------------------------------------------------------ |
| `clk`               | Input     | 1-bit             | Clock signal.                                                       |
| `rst_n`             | Input     | 1-bit             | Active-low reset signal.                                            |
| `rs2mul_uop_valid`  | Input     | 1-bit             | Valid signal for the instruction from Reservation Station.          |
| `rs2mul_uop_data`   | Input     | `MUL_RS_t` packet | Instruction data from Reservation Station (operands, opcode, etc.). |
| `mul_stg0_vld_en`   | Input     | 1-bit             | Pipeline valid enable for stage 0.                                  |
| `mul_stg0_data_en`  | Input     | 1-bit             | Pipeline data enable for stage 0.                                   |
| `mul2rob_uop_valid` | Output    | 1-bit             | Valid signal for the result sent to Reorder Buffer (ROB).           |
| `mul2rob_uop_data`  | Output    | `PU2ROB_t` packet | Result data sent to ROB (write data, saturation flags, etc.).       |

### `rvv_backend_mulmac` Interfaces

| Signal             | Direction | Width                  | Description                                             |
| :----------------- | :-------- | :--------------------- | :------------------------------------------------------ |
| `clk`              | Input     | 1-bit                  | Clock signal.                                           |
| `rst_n`            | Input     | 1-bit                  | Active-low reset signal.                                |
| `trap_flush_rvv`   | Input     | 1-bit                  | Trap flush signal to clear pipeline.                    |
| `uop_valid_rs2ex`  | Input     | `NUM_MUL` bits         | Valid signal for instructions from Reservation Station. |
| `mac_uop_rs2ex`    | Input     | `NUM_MUL` x `MUL_RS_t` | Instruction data from Reservation Station.              |
| `pop`              | Output    | `NUM_MUL` bits         | Pop signal to Reservation Station.                      |
| `res_valid_ex2rob` | Output    | `NUM_MUL` bits         | Valid signal for results sent to ROB.                   |
| `res_ex2rob`       | Output    | `NUM_MUL` x `PU2ROB_t` | Result data sent to ROB.                                |
| `res_ready_rob2ex` | Input     | `NUM_MUL` bits         | Ready signal from ROB.                                  |

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------


> **Traceability:** Generated by Gemini. Derived from upstream commit 28fdd2f4b80b1db06a4025b828807fcdc0e76f88. AI-generated/assisted; RTL is the source of truth.
