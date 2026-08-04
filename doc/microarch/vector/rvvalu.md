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

# Vector ALU (VALU)

> **Intended Audience:** HW Devs, SW/Compiler Devs

## Architectural function

The `RvvAlu` (Vector ALU) module implements the primary execution unit for vector operations within the CoralNPU. It provides high-throughput arithmetic, logical, and permutation capabilities across a wide range of vector data types and precisions, processing instructions dispatched by the Vector Backend.

## Interfaces

The VALU interfaces with the `RvvCore` using standardized `Decoupled` and `Regfile` interfaces.

| Interface Name | Direction | Type | Description |
| :--- | :--- | :--- | :--- |
| `io.rs` | Input | `RegfileReadDataIO` | Vector register file read operands. |
| `io.rd` | Output | `RegfileWriteDataIO` | Vector register file writeback results. |
| `io.configState`| Input | `RvvConfigState` | Current vector execution configuration (vtype, vl). |

[Source: `hdl/chisel/src/coralnpu/rvv/RvvInterface.scala`]

## Viota.m latency

The `viota.m` instruction performs a population count of set mask bits. Its execution latency is 4 cycles in the Vector ALU due to the serial dependency chain required for mask prefix summation.

[Source: `hdl/verilog/rvv/design/rvv_backend_tpe.sv`]

The `RvvAlu` supports a comprehensive set of RVV-compliant operations including:

| Category | Operations |
| :--- | :--- |
| Integer Arithmetic | `VADD`, `VSUB`, `VRSUB` |
| Comparison | `VMIN`, `VMAX`, `VMSEQ`, `VMSNE`, `VMSLT`, `VMSLE`, `VMSGT` |
| Bitwise Logical | `VAND`, `VOR`, `VXOR` |
| Permutation | `VRGATHER`, `VSLIDEUP`, `VSLIDEDOWN` |
| Fixed-Point | `VSADD`, `VSSUB`, `VSMUL`, `VNCLIP` |
| Floating-Point | `VFNCVTBF16`, `VFWCVTBF16`, `VFWMACCBF16` |

[Source: `hdl/chisel/src/coralnpu/rvv/RvvAlu.scala`]

## Execution and precision

The `RvvAlu` operates on a configurable `vtype` defined by the current vector register state, passed via `io.configState`.

- **Integer precision**: Supports standard RISC-V Selected Element Widths (SEW) of 8, 16, and 32 bits.

- **Floating-point/BF16 precision**: Supports specialized Brain Floating Point (BF16) precision for machine learning workloads. The FALU units compute natively with internal precision normalization to maintain IEEE 754 compliance for FMA operations, ensuring intermediate results maintain sufficient mantissa and exponent range before final rounding to the destination format.

- **Widening operations**: Capable of executing instructions that operate on operands of different widths, facilitating mixed-precision arithmetic.

### Bf16/fp16alt nan-boxing

To interface with the `fpnew` FMA unit, which expects 32-bit single-precision inputs, 16-bit floating-point values (BF16 and FP16ALT) are NaN-boxed. This involves padding the 16-bit value with `16'hFFFF` in the most significant bits, creating a 32-bit value. This allows the `fpnew` unit to process these smaller floating-point types correctly within its single-precision datapath.

[Source: `hdl/verilog/rvv/design/rvv_backend_falu_unit.sv`, `hdl/verilog/rvv/design/Zvt/fp_align.sv`]

## Pipelining stages

The `RvvAlu` (specifically `rvv_backend_alu_unit`) implements a variable-latency pipeline with two primary stages (P0 and P1) to manage operation throughput and resolve structural hazards.

- **Stage 0 (P0)**: Contains parallel functional units including `u_alu_addsub`, `u_alu_shift`, `u_alu_mask`, and `u_alu_other`.

- **Stage 1 (P1)**: Contains the `rvv_backend_alu_unit_execution_p1` module.

- **Variable latency & collision resolution**:

  - `VADDSUB` operations are intrinsically 2-cycle, requiring processing in both P0 and P1.
  - `Shift`, `Mask`, and other operations can complete in 1 cycle directly from P0 if P1 is idle.

  - If P1 is busy (e.g., submitting a result from an `addsub` instruction), colliding results from P0 functional units are buffered into P1. They are passed through P1 without re-execution in the subsequent cycle, ensuring in-order submission to the Reorder Buffer (ROB) and resolving writeback collisions (WAW hazards) without pipeline stalls.
  - **Pipeline hazard resolution**: RAW hazards are resolved at dispatch by checking ROB source register availability. WAW hazards are managed at retirement using write strobes for identical destination registers. Internal execution collisions are resolved by the P0-to-P1 buffering mechanism described above.

[Source: [`rvv_backend_alu_unit.sv`](../../../hdl/verilog/rvv/design/rvv_backend_alu_unit.sv)]

## Data paths

- **Instruction input**: Decoded instructions (`ALU_RS_t`) are broadcast to all functional units in P0.

- **Result writeback**: Results from P0 and P1 are arbitrated. Priority for ROB submission (`result_valid`) is given to P1 results or non-addsub P0 results when P1 is idle.

- **Buffering path**: When P1 is busy submitting a result, P0 results are redirected to the P1 buffer (`alu_uop_p1`) and passed through to the ROB in the next cycle via `result_p1`.

- **Side-effects**: Saturation flags and masks are propagated along with the data.

[Source: [`rvv_backend_alu_unit.sv`](../../../hdl/verilog/rvv/design/rvv_backend_alu_unit.sv)]

## Execution side-effects

### Saturating arithmetic and vxsat commit timing

Operations that saturate (e.g., `VSADD`, `VSSUB` in `RvvAlu` / `rvv_backend_alu_unit.sv`; `VSMUL` in `rvv_backend_mac_unit.sv` or `rvv_backend_mul_unit.sv`) detect element-level overflow by checking if the result exceeds the maximum or minimum representable limit for the active Selected Element Width (SEW).

- **Execution timing**: When overflow occurs on any active vector element, a bitwise saturation mask `vsaturate` (width `VLENB` based on the vector register length VLEN = 128 bits, indicating the byte lanes where saturation occurred) is set in the execution uop data. This mask is written into the Reorder Buffer (ROB) during execution and remains speculative until instruction retirement.

- **Retirement commit**: At the retirement stage (`rvv_backend_retire.sv`), when the uop reaches the head of the ROB and successfully retires without a trap (`!trap_flag[0]`), the retirement logic checks the `w_vxsat` register which aggregates the uop's `vsaturate` flags. The retirement logic then asserts the `rt2vxsat_write_valid` signal:
    `rt2vxsat_write_valid = |(w_vrf_valid & rt2rob_write_ready & w_vxsat) && !trap_flag[0]`
    This writes/commits the bit to the architectural `vxsat` CSR. This ensures precise, non-speculative exception and status timing.

> [!IMPORTANT]
> **Known Limitation (vxsat Commit Failure - #QA-047):** The hardware implementation currently fails to commit saturation results to the architectural `vxsat` CSR. The `wr_vxsat` signals are not fully connected in the `RvvCore` top-level, causing `vxsat` to remain 0 despite saturation occurring in functional units. Reference: `tests/cocotb/rvv/arithmetics/vnclip_test.cc`.

[Source: [`rvv_backend_retire.sv`](../../../hdl/verilog/rvv/design/rvv_backend_retire.sv#L223), [`rvv_backend_alu_unit_execution_p1.sv`](../../../hdl/verilog/rvv/design/rvv_backend_alu_unit_execution_p1.sv)]

### Inactive element masking handling

Mask-driven instructions execute under a vector mask register (`v0.t`). The mask controls whether elements are active or inactive.

- **Inactive undisturbed (`ma = 0`)**: Inactive elements must preserve their previous values. During the bypass stage (`rvv_backend_dispatch_bypass.sv`), if a byte lane is `BODY_INACTIVE`, the bypass logic routes the original values from the destination register (`vs3`/`vd`) directly, maintaining "undisturbed" semantics without requiring a read from the VRF.

- **Inactive agnostic (`ma = 1`)**: The hardware is allowed to write `1`s (injecting `8'hFF`) to inactive element byte lanes to simplify routing and save power. This is physically handled in the bypass stage by a direct 1-injection override which outputs `8'hFF` instead of forwarding the destination register data.

- **Write suppression**: At the final writeback stage, the write strobe mask `vrfres_strobe` selectively enables VRF writes only for active elements, physically suppressing write operations to the Vector Register File (VRF) for elements that are undisturbed/masked-off.

[Source: [`rvv_backend_dispatch_bypass.sv`](../../../hdl/verilog/rvv/design/rvv_backend_dispatch_bypass.sv), [`rvv_backend_retire.sv`](../../../hdl/verilog/rvv/design/rvv_backend_retire.sv)]

### Complex instruction scheduling and dispatch blockage

Complex vector mask instructions like `viota.m` (Vector Instruction-Register-To-Array) calculate a prefix-sum of active elements under a mask.

- **Hardware implementation**: This is implemented via a multi-level carry-reduction/summation adder tree spanning sub-modules like `rvv_backend_alu_unit_mask_viota7` and `rvv_backend_alu_unit_mask_viota32` inside `rvv_backend_alu_unit_mask_viota.sv`.

- **Latency and scheduling**: Due to carry-propagation delays in the reduction tree, `viota.m` has a multi-cycle execution latency of up to 4 cycles. Because `RvvAlu` contains a variable-latency pipeline and shares execution resources, the dispatch unit tracks in-flight multi-cycle operations.

- **Dispatch blockage**: If a multi-cycle instruction is active in the pipeline, the reservation station and dispatch scheduler implement a structural interlock, blocking dispatch to subsequent dependent instructions in the same execution group to prevent pipeline bubbles and out-of-order writeback collisions.

[Source: [`rvv_backend_alu_unit_mask_viota.sv`](../../../hdl/verilog/rvv/design/rvv_backend_alu_unit_mask_viota.sv), [`rvv_backend_dispatch.sv`](../../../hdl/verilog/rvv/design/rvv_backend_dispatch.sv)]

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/coralnpu/rvv/RvvAlu.scala`, `hdl/chisel/src/coralnpu/rvv/RvvInterface.scala`, `hdl/verilog/rvv/design/rvv_backend_alu_unit.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
