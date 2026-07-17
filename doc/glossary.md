# Glossary

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

>
> **Intended Audience:** Hardware Developers, SW/Compiler Developers

This document defines acronyms and domain-specific terms used throughout the Coral NPU architecture wiki.

- **ALU**: Arithmetic Logic Unit. The central execution unit within the scalar core's pipeline (`doc/microarch/core/alu.md`), handling integer arithmetic, logic, and generating addresses for the LSU.
- **AXI4**: Advanced eXtensible Interface. The primary 256-bit host memory interface used by the CoralNPU for off-chip memory transfers and CSR configuration via AXI4.
- **BRU**: Branch Unit. Evaluates control flow instructions and mispredictions within the scalar core, generating redirect signals and resolving speculative execution state in the IFU's Fetch Reorder Buffer.
- **Clock Gating**: A power-management technique implemented via the `ClockGate` primitive, dynamically disabling clock distribution to idle pipeline stages or execution units, with an override (`te`) for DFT scan chains.
- **CQ**: **Command Queue**. A FIFO buffer that receives and holds `RVVCmd` packets from the frontend decoder, decoupling instruction fetch from execution. It manages flow control by asserting backpressure signals (e.g., `cq_almost_full`) to stall the frontend when the [Vector Backend](microarch/rvv_backend.md) is congested.
- **CSR**: Control and Status Register. A memory-mapped interface exposed to the host via AXI4, facilitating dynamic parameter tuning, interrupt status polling, and operational lifecycle control of the NPU IP.
- **DE**: **Decode Unit** (Vector Backend). A pipeline stage that expands complex architectural RISC-V Vector instructions into sequences of hardware-specific micro-operations (uops), factoring in LMUL and operand widths.
- **DP**: **Dispatch Unit** (Vector Backend). An arbiter within the vector pipeline that resolves register dependencies via scoreboarding and issues decoupled micro-operations to parallel execution lanes (e.g., VALU, VFPU) when their execution conditions are met.
- **DTCM**: Data Tightly-Coupled Memory. The primary multi-banked, single-cycle latency SRAM block servicing vector load/store operations and scalar data accesses via the Internal Memory Fabric.
- **DVU**: Divide Unit. An iterative, multi-cycle execution engine integrated into the scalar core (`doc/microarch/dvu.md`) that handles integer division without stalling the main pipeline until writeback.
- **EMUL**: Effective LMUL. The product of the architectural `LMUL` and the ratio of operand widths, used to determine the total number of elements in a vector group.
- **Fault Manager**: A centralized unit for aggregating and prioritizing hardware exceptions.
- **fpnew**: A high-performance, parameterizable floating-point unit from the PULP project, used as the underlying execution engine for the CoralNPU FPU and VFPU.
- **FPU**: Floating-Point Unit. Integrates the `fpnew` IP into the scalar core to execute single-precision operations (RV32F), directly interfacing with the FRF and the scalar pipeline's writeback stage.
- **FRF**: Floating-Point Register File. Provides 32 architectural registers (f0-f31) for single-precision floating-point operations, integrated with the FPU and VFPU execution pipelines.

- **ITCM**: Instruction Tightly-Coupled Memory. The dedicated SRAM block providing single-cycle instruction delivery to the UncachedFetch unit, bypassing the standard memory hierarchy for deterministic execution.

- **L0 Cache**: A fully-associative micro-cache embedded directly within the Instruction Fetch Unit to buffer recently fetched cache lines and minimize latency for tight execution loops.

- **L1 Cache**: The primary set-associative cache hierarchy (L1I and L1D) managed by the core's memory subsystem, arbitrating access between the scalar execution pipelines and the AXI4 interconnect. **Note:** 

... [544,001 characters omitted] ...

                          |
+| `VSLL`       | `100101` | 0/1 |                                 |
+| `VMV1R`      | `100111` |  1  | `imm5 == 0`                     |
+| `VMV2R`      | `100111` |  1  | `imm5 == 1`, Align 2            |
+| `VMV4R`      | `100111` |  1  | `imm5 == 3`, Align 4            |
+| `VMV8R`      | `100111` |  1  | `imm5 == 7`, Align 8            |
+| `VSRL`       | `101000` | 0/1 |                                 |
+| `VSRA`       | `101001` | 0/1 |                                 |
+| `VSSRL`      | `101010` | 0/1 |                                 |
+| `VSSRA`      | `101011` | 0/1 |                                 |
+| `VNSRL`      | `101100` | 0/1 |                                 |
+| `VNSRA`      | `101101` | 0/1 |                                 |
+| `VNCLIPU`    | `101110` | 0/1 |                                 |
+| `VNCLIP`     | `101111` | 0/1 |                                 |

### OPFVV - Floating-Point Vector-Vector Operations (funct3 = 001)

| Instruction   |  Funct6  |   vs1   | Constraints |
| :------------ | :------: | :-----: | :---------- |
| `VFWCVTBF16`  | `010010` | `01101` | Widening    |
| `VFNCVTBF16`  | `010010` | `11101` |             |
| `VFWMACCBF16` | `111011` |   vs1   | Widening    |

### OPFVF - Floating-Point Vector-Scalar Operations (funct3 = 101)

| Instruction   |  Funct6  |   rs1   | Constraints |
| :------------ | :------: | :-----: | :---------- |
| `VFWCVTBF16`  | `010010` | `01101` | Widening    |
| `VFWMACCBF16` | `111011` |   rs1   | Widening    |

## Scheduling and Execution Constraints

### Zero `vstart` Requirement

The following operations (typically OPMVV mode, funct3 = 010) are tracked for compressed instruction validation and require `vstart` to be zero to execute without trapping:

- **Reductions**: `vredsum`, `vredand`, `vredor`, `vredxor`, `vredminu`, `vredmin`, `vredmaxu`, `vredmax`, `vwredsumu`, `vwredsum`.
- **Unary Operations**: `vcpop`, `vfirst`, `vmsbf`, `vmsof`, `vmsif`, `viota`.
- **Compression**: `vcompress`.

_Note: These instructions are validated in `RvvDecode.scala` for internal compressed pipeline tracking._

## Hardware `vstart` Memory Fault Behavior (ADR-043)

The CoralNPU hardware does NOT dynamically update the `vstart` CSR on a mid-instruction memory fault (such as a trap during a vector load/store operation).

It is exclusively software-driven via CSR writes. Software trap handlers and operating system contexts must manually manage the `vstart` state via explicit `vstart_write` operations if they intend to resume vector execution after resolving a memory fault. Relying on the hardware to automatically checkpoint the failing element position into `vstart` will result in incorrect execution state recovery.

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-16 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/chisel/src/coralnpu/rvv/RvvDecode.scala`, `hdl/chisel/src/coralnpu/scalar/Csr.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->