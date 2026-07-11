# Glossary

<!--
 Copyright 2024 Google LLC

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

- **L1 Cache**: The primary set-associative cache hierarchy (L1I and L1D) managed by the core's memory subsystem, arbitrating access between the scalar execution pipelines and the AXI4 interconnect. **Note:** These caches are dormant/uninstantiated in the production CoralNPU IP block and serve as reference designs for custom hierarchies.

- **linkOk**: A hardware check in the Retirement Buffer that verifies the fetched Program Counter matches the expected target of the previous instruction to handle fetch mispredictions.

- **LMUL**: Length Multiplier. A RISC-V Vector field that specifies the grouping of multiple vector registers to form a single larger vector.

- **LRU**: Least Recently Used. A deterministic eviction protocol applied in the core's reference set-associative caches (L1D/L1I), prioritizing the retention of recently accessed memory lines by maintaining access age metrics in a dedicated state array.

- **LSU**: Load/Store Unit. Handles all memory operations issued by the core by translating memory instructions into transactions on the appropriate AXI4 bus, utilizing an internal command queue and slot mechanism for scheduling.

- **MLU**: Multiply Unit. A pipelined integer multiplier unit in the scalar core (`doc/microarch/mlu.md`) responsible for executing RISC-V M-extension instructions with deterministic latency.

- **mpause**: A custom RISC-V instruction that halts the core execution and gates its clock until an interrupt or external debug request is received.

- **NPUSim**: A high-level, bit-accurate functional simulator for the Coral NPU. It provides Python and C++ bindings to allow rapid software development and verification without the overhead of full RTL simulation.

- **PMT/RDT**: Vector Permutation and Reduction Unit. Handles cross-lane data movement and vector-to-scalar reductions.

- **RAS**: Return Address Stack. A microarchitectural LIFO structure within the Branch Unit (BRU) that maintains a dynamic history of function calls to preemptively resolve return jumps, avoiding pipeline flushes on standard subroutine returns.

- **RS**: **Reservation Station**. A distributed buffer in the[Vector Backend](microarch/rvv_backend.md) used for out-of-order instruction issue.

- **RT**: **Retire Unit** (Vector Backend). The final sequential pipeline stage that orchestrates the in-order writeback of completed execution results to the Vector Register File, signaling the safe deallocation of Reservation Stations.

- **ROB**: **Retirement Buffer** (most common usage). Manages the lifecycle of in-flight instructions and ensures in-order retirement. May also refer to a**Reorder Buffer** in specific units like the VFPU, Fetcher, or[Vector Backend](microarch/rvv_backend.md).

- **RVV**: RISC-V Vector extension. The architectural specification implemented by the Vector Core, specifically adhering to the Zve32f profile for 32-bit floating-point support.

- **Scoreboarding**: The hardware dependency-tracking matrix within the RVV Backend's Dispatch Unit, resolving RAW, WAR, and WAW hazards by monitoring in-flight register reservations.

- **SIMD**: Single Instruction, Multiple Data. The data-level parallelism paradigm implemented by the RVV Vector Backend, utilizing wide 128-bit datapaths to process multiple scalar elements per cycle.

- **Speculative Fetching**: The ability of the [IFU](microarch/fetch.md) to initiate instruction requests for addresses that have not yet been definitively reached by the program execution (e.g., predicted branch targets or next-line prefetching).

- **SRF**: Scalar Register File. The 31-entry 32-bit architectural register file in the scalar core frontend, implemented using flop arrays to support high-speed, multi-ported access for instruction decode and writeback.

- **System CSR**: Exposes the AXI4-mapped architectural baseline for external host management, providing dedicated interface boundaries for managing lifecycle state transitions, power gating, and exception vectors distinct from datapath logic.

- **TCM**: Tightly-Coupled Memory. The collective SRAM architecture (ITCM and DTCM) accessed via the FabricIO protocol, providing guaranteed single-cycle access latency for real-time inference workloads.

- **UQ**: **Uop Queue**. An intermediate decoupling buffer positioned between the decode and dispatch stages of the vector backend, absorbing varying throughput latencies of the stripmining logic.

- **VALU**: Vector Arithmetic Logic Unit. The primary SIMD execution pipeline within the RVV backend (`doc/microarch/rvvalu.md`), utilizing an array of parallel adders/shifters and directly accessing the VRF to process decoupled vector micro-operations.

- **VADC / VSBC**: Vector Add/Subtract with Carry. SIMD instructions utilizing a dedicated carry-in/borrow-in bit from the v0 mask register, essential for chaining operations across multi-word precision boundaries.

- **VMADC / VMSBC**: Vector Masked Add/Subtract with Carry. Produces a carry/borrow bit into a mask register.

- **VNCLIP**: Vector Narrowing Fixed-Point Clip. Narrowing instruction with rounding and saturation.

- **VDIV**: Vector Integer Divide Unit. A dedicated SIMD execution unit in the RVV backend (`doc/microarch/vector/vdiv.md`) that processes multi-lane integer division, handling variable latency and issuing completion signals to the vector retirement buffer.

- **VFPU**: Vector Floating-Point Unit. A high-performance, multi-lane SIMD engine in the RVV backend (`doc/microarch/vfpu.md`) wrapping multiple `fpnew` instances to execute Zve32f vector floating-point instructions in parallel.

- **VLEN**: Vector Length in bits. The fixed width of a single vector register in the hardware (128 bits for CoralNPU).

- **volt_sel**: Voltage Selection. A hardware signal used to select between different operating voltages for SRAM bitcells, allowing for power-performance tradeoffs.

- **VRF**: Vector Register File. The primary storage for vector operands (32 registers, 128-bit VLEN), providing high-bandwidth, multi-ported access for concurrent execution by multiple lanes and dispatch units.

- **VSTART**: Vector Start Index. A CSR that tracks the starting element index for vector operations, used for resuming operations after an interrupt.

- **vxsat**: Vector Fixed-Point Saturation flag. A CSR bit that indicates if a fixed-point operation resulted in saturation.

- **vxrm**: Vector Fixed-Point Rounding Mode. A CSR that controls the rounding behavior of vector fixed-point instructions.



---

> **Traceability:** Generated by Gemini. Derived from upstream commit 7d6e35cff17732cad8f99b866333423ea1f4d750.
