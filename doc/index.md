# CoralNPU Architecture Wiki

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

> **Intended Audience:** System Integrators, Hardware Developers


Welcome to the Coral NPU microarchitecture and hardware documentation. This wiki serves as the definitive reference for the standalone IP block.

## Getting Started

If you are new to the CoralNPU project, begin your journey by reviewing the core architectural documentation:

- [System Architecture Overview](#system-architecture-overview)
- [Hardware/Software Interface](hardware_software_interface.md)
- [Interfaces and Buses](microarch/interfaces_and_buses.md)
- [Verification Strategy](verification.md)

## Role-Based Navigation

To find the information most relevant to your work, please follow the links tailored to your persona:

| Persona                 | Primary Focus Areas                                                                          | Key Documents                                                                                                                                        |
| :---------------------- | :------------------------------------------------------------------------------------------- | :--------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Hardware Developers** | RTL implementation, microarchitecture, cycle-level timing, pipeline hazards.                 | [Core Execution pipeline](microarch/microarch.md), [Interfaces & Buses](microarch/interfaces_and_buses.md)                                           |
| **SW/Compiler Devs**    | Instruction encoding, CSR programming, memory map, exception handling, dataflow constraints. | [Hardware/Software Interface](hardware_software_interface.md), [Verification](verification.md), [FlashAttention Kernel](sw/flash_attention.md)       |
| **HW Integrators**      | IP integration, AXI4 bus boundaries, reset sequencing, memory parameters.                    | [Interfaces & Buses](microarch/interfaces_and_buses.md), [Top-Level Subsystem](top_level_subsystem.md)                                               |
| **Software Folks**      | High-level data movement (DMA), software configuration, ring buffers, interrupts.            | [Software DMA](microarch/infrastructure/dma.md), [Command Ring Buffer](sw/command_ring.md), [Synchronization Primitives](sw/synchronization.md), [FlashAttention Kernel](sw/flash_attention.md) |

## System Architecture Overview

The following diagram illustrates the top-level CoralNPU SoC Subsystem architecture, highlighting the `CoreAxi` interfaces and internal IP boundaries (replacing legacy interfaces).

_For the top-level CoralNPU SoC Subsystem architecture diagram, please refer to the [Top-Level Subsystem](top_level_subsystem.md#top-level-system-architecture-diagram) documentation._

## Architectural Domains

### System Overview

- [Top-Level Subsystem](top_level_subsystem.md)
- [Hardware/Software Interface](hardware_software_interface.md)
- [Interfaces and Buses](microarch/interfaces_and_buses.md)

### Microarchitecture Deep Dives

#### Core Execution (Scalar)

- [CoralNPU Microarchitecture](microarch/microarch.md)
- [SCore Pipeline Wiring](microarch/core/score.md)
- [Core Pipeline Wrapper](microarch/core/core.md)
- [Uncached Fetch Unit](microarch/core/fetch.md)
- [Core Decode and Dispatch Unit](microarch/core/decode.md)
- [CoralNPU Dispatch Rules](microarch/core/dispatch.md)
- [Scalar Register File (Regfile)](microarch/core/regfile.md)
- [Arithmetic Logic Unit (ALU)](microarch/core/alu.md)
- [Branch Resolution Unit (BRU)](microarch/core/bru.md)
- [Multiplier Unit (MLU) Architecture](microarch/core/mlu.md)
- [Divider Unit (DVU) Architecture](microarch/core/dvu.md)
- [Scalar Floating-Point Unit (FPU)](microarch/core/fpu.md)
- [FloatCore](microarch/core/float_core.md)
- [Scalar FPU Execution Pipeline](microarch/core/fpu_pipeline.md)
- [Floating-Point Register File (FRegfile)](microarch/core/fregfile.md)
- [Retirement Buffer](microarch/core/retirement_buffer.md)
- [Fault Manager](microarch/core/faultmanager.md)
- [Instruction Buffer](microarch/common/instruction_buffer.md)
- [Fused Multiply-Add (FMA)](microarch/common/fma.md)
- [Chisel Integer Divider (`IDiv`)](microarch/common/idiv.md)
- [Dedicated Integer Divider](microarch/common/intdivider.md)

#### Memory Hierarchy

- [Memory Hierarchy](microarch/memory_hierarchy.md)
- [Load Store Unit](microarch/memory/lsu.md)
- [Tightly Coupled Memory (TCM128)](microarch/memory/tcm.md)
- [Cache Architecture](microarch/memory/cache_architecture.md)
- [SRAM Wrappers](microarch/memory/sram.md)
- [CircularBufferMulti](microarch/common/circular_buffer_multi.md)
- [Fifo Hardware Primitive](microarch/common/fifo.md)
- [Index Allocator (IndexAllocator.scala)](microarch/common/index_allocator.md)

#### Vector Core (RVV)

- [Vector Core (RVV)](microarch/vector/rvv.md)
- [Vector Core Pipeline (RvvCore)](microarch/vector/rvv_core.md)
- [Vector Frontend Wrapper](microarch/vector/rvv_frontend.md)
- [Vector Backend Pipeline](microarch/vector/rvv_backend.md)
- [RVV Instruction Decoding](microarch/vector/rvv_decode.md)
- [Vector Backend Decoder (DE2)](microarch/vector/backend_decode.md)
- [Vector Backend Secondary Decode Stage (DE2)](microarch/vector/backend_decode_de2.md)
- [Vector Dispatch Unit](microarch/vector/vector_dispatch.md)
- [Vector Dispatch Operand Processing](microarch/vector/dispatch_operand.md)
- [Vector Dispatch RAW Hazard Tracking](microarch/vector/dispatch_raw_hazard.md)
- [Vector Dispatch Bypass and Hazard Logic](microarch/vector/dispatch_advanced.md)
- [Vector Register File (VRF)](microarch/vector/vrf.md)
- [Vector Backend Reorder Buffer (ROB)](microarch/vector/rob.md)
- [Vector Backend ROB Writeback Arbiter](microarch/vector/rvv_backend_arb.md)
- [Vector Arithmetic Logic Unit (RvvAlu)](microarch/vector/rvvalu.md)
- [Vector Backend ALU Array](microarch/vector/backend_alu.md)
- [Vector Backend Multiplier Unit](microarch/vector/mul_unit.md)
- [Vector Floating-Point Unit (VFPU)](microarch/vector/vfpu.md)
- [Vector Backend FALU Array](microarch/vector/backend_falu.md)
- [Vector Divider Unit (VDIV)](microarch/vector/vdiv.md)
- [Vector Backend Floating-Point Divider (FDIV)](microarch/vector/backend_fdiv.md)
- [Vector Backend Reciprocal & Square Root Estimates](microarch/vector/backend_sqrt7_rec7.md)
- [Vector MAC Engine (Tensor Processing)](microarch/vector/mac_engine.md)
- [Vector Permutation and Reduction Unit (PMTRDT)](microarch/vector/pmtrdt.md)
- [Vector Floating-Point Reduction Unit](microarch/vector/freduction.md)
- [Vector Scatter/Gather Hardware](microarch/vector/scatter_gather.md)
- [Vector LSU Remap Unit](microarch/vector/lsu_remap.md)
- [Vector Retirement Unit](microarch/vector/retire.md)
- [MultiFifo Hardware Primitive](microarch/vector/multi_fifo.md)

#### Infrastructure

- [Hardware Generation Parameters](microarch/parameters.md)
- [Interconnects & Interfaces](microarch/infrastructure/interconnects.md)
- [Internal Memory Fabric](microarch/infrastructure/fabric.md)
- [Simulation-Safe Round-Robin Arbiter (CoralNPUArbiter)](microarch/common/coralnpu_arbiter.md)
- [Aligner](microarch/common/aligner.md)
- [Control and Status Registers (CSR)](microarch/infrastructure/csr.md)
- [System Control and Status Registers (CSRs)](microarch/infrastructure/system_csr.md)
- [AXI4 Slave Interface](microarch/infrastructure/axi_slave.md)
- [IBus2Axi Bridge](microarch/infrastructure/ibus2axi.md)
- [DBus to AXI4 Bridge (DBus2Axi)](microarch/infrastructure/dbus2axi.md)
- [Direct Memory Access (DMA) Engine](microarch/infrastructure/dma.md)
- [Debug Module](microarch/infrastructure/debug.md)
- [Clock Gate](microarch/infrastructure/clock_gate.md)
- [Reset Synchronization (RstSync)](microarch/infrastructure/rstsync.md)
- [RvviTrace](microarch/infrastructure/rvvi_trace.md)

#### Software & Kernel Specifications

- [Vector ISA and Execution Architecture](sw/rvv_isa.md)
- [Activation Functions](sw/activations.md)
- [Software DMA](microarch/infrastructure/dma.md)
- [Synchronization Primitives](sw/synchronization.md)
- [Command Ring Buffer & Execution Queue](sw/command_ring.md)
- [Multi-Head FlashAttention RVV Kernel](sw/flash_attention.md)

### Validation

- [Verification Architecture](verification.md)

<!-- mdformat off -->
<!-- prettier-ignore -->
> **Traceability:** Generated by Gemini. Derived from upstream commit f5f6c88d3dff8cb198cd89420919b6863667f3e0.
<!-- mdformat on -->

---

**Provenance & Traceability** - **Verified As Of:** 2026-07-16 - **Upstream Commit:** N/A - **Primary Source(s):** RTL - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
