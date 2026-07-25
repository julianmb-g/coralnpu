# Vector Frontend Wrapper

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

> **Intended Audience:** HW Devs


The `RvvFrontEnd` module acts as the vector instruction frontend wrapper, handling architectural configuration state (e.g., `LMUL`, `SEW`), instruction decoupling, and alignment before dispatching commands to the vector instruction queue.

## Instruction Decoupling FIFO and Alignment

The frontend receives up to $N$ unaligned instructions per cycle (`inst_valid_i`). It performs a prefix sum (`valid_in_psum`) against the available `queue_capacity_i` to calculate which instructions can be accepted, asserting `inst_ready_o`. Instructions are buffered for one cycle to allow scalar register file reads (like `rs1`) to complete, and then an `Aligner` module ensures that the output commands (`cmd_data_o`) are tightly packed and contiguous before entering the vector queue.

## Configuration Synchronization Mechanism

The `RvvFrontEnd` module maintains the architectural vector state (`RVVConfigState`), tracking `vstart`, `vxrm`, `vxsat`, `frm`, `vl`, `vtype` (`sew`, `lmul`, `ta`, `ma`), and `vill`. When configuration instructions like `vsetvl`, `vsetvli`, or `vsetivli` are decoded, the frontend dynamically calculates the new vector length (`vlmax` based on `LMUL` and `SEW`) and updates the state. These configuration commands also generate scalar register writebacks (`reg_write_valid_o`) with the updated `vl`. If an illegal configuration is requested, the `vill` flag is asserted, driving `vl` to 0 and triggering an instruction trap.

## Structural Hazard Tracking

Hardware assertions ensure that valid instructions requiring scalar register reads (`requires_rs1_read`, `requires_rs2_read`) accurately track the availability of data from the scalar register file (`reg_read_valid_i`). Memory operations (LSU) and configuration operations (like `vsetvl`) are checked against `rs1` and `rs2` validity boundaries.

## Interface

### Parameters
| Name | Default | Description |
| :--- | :--- | :--- |
| `N` | `4` | Number of instructions per cycle. |
| `CAPACITYBITS` | `$clog2(2*N + 1)` | Bit-width for queue capacity count. |
| `REDUCE_LMUL` | `1` | Configuration parameter for LMUL reduction. |

### Ports
| Port Name | Direction | Type / Width | Description |
| :--- | :--- | :--- | :--- |
| `clk` | Input | `logic` | Clock signal. |
| `rstn` | Input | `logic` | Active-low reset. |
| `vstart_i` | Input | ``logic [`VSTART_WIDTH-1:0]`` | Vector start element index. |
| `vxrm_i` | Input | ``logic [`VCSR_VXRM_WIDTH-1:0]`` | Vector fixed-point rounding mode. |
| `vxsat_i` | Input | ``logic [`VCSR_VXSAT_WIDTH-1:0]`` | Vector fixed-point saturation flag. |
| `frm_i` | Input | `logic [2:0]` | Floating-point rounding mode. |
| `inst_valid_i` | Input | `logic [N-1:0]` | Valid signals for incoming instructions. |
| `inst_data_i` | Input | `RVVInstruction [N-1:0]` | Incoming vector instruction packets. |
| `inst_ready_o` | Output | `logic [N-1:0]` | Ready signals to accept instructions. |
| `reg_read_valid_i` | Input | `logic [(2*N)-1:0]` | Valid signals for scalar register reads. |
| `reg_read_data_i` | Input | `logic [(2*N)-1:0][31:0]` | Scalar register read data. |
| `freg_read_data_i` | Input | `logic [N-1:0][31:0]` | Floating-point register read data (for `rs1` OPFVF). |
| `reg_write_valid_o` | Output | `logic [N-1:0]` | Valid signals for scalar register writeback. |
| `reg_write_addr_o` | Output | `logic [N-1:0][4:0]` | Addresses for scalar register writeback. |
| `reg_write_data_o` | Output | `logic [N-1:0][31:0]` | Data for scalar register writeback. |
| `cmd_valid_o` | Output | `logic [N-1:0]` | Valid signals for output vector commands. |
| `cmd_data_o` | Output | `RVVCmd [N-1:0]` | Output vector commands to queue. |
| `queue_capacity_i` | Input | `logic [CAPACITYBITS-1:0]` | Available capacity in the vector queue. |
| `queue_capacity_o` | Output | `logic [CAPACITYBITS-1:0]` | Forwarded queue capacity. |
| `trap_valid_o` | Output | `logic` | Trap condition raised. |
| `trap_data_o` | Output | `RVVInstruction` | Instruction causing the trap. |
| `config_state_valid` | Output | `logic` | Valid signal for architectural config state. |
| `config_state` | Output | `RVVConfigState` | Architectural vector configuration state. |

<!-- mdformat off -->
<!-- prettier-ignore -->


--------------------------------------------------------------------------------

**Provenance & Traceability**
- **Verified As Of:** 2026-07-24
- **Upstream Commit:** [2be7892532110edbcd0ca4e7ff56e4360a428df7](https://github.com/google/coralnpu/commit/2be7892532110edbcd0ca4e7ff56e4360a428df7)
- **Primary Source(s):** `hdl/verilog/rvv/design/RvvFrontEnd.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

> **Traceability:** Generated by Gemini. Derived from upstream commit 6a8cc54a67fb4ca7ecda116453fbdc4a97994ebf.
