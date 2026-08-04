# Glossary of terms

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
> **Intended Audience:** HW Devs

Defines technical terms and acronyms used in CoralNPU documentation.

## Terminology

| Term | Definition |
| :--- | :--- |
| **AXI4** | Advanced eXtensible Interface, a standard bus protocol for high-bandwidth, low-latency interconnects. [Source: `hdl/chisel/src/coralnpu/CoreAxi.scala`] |
| **BRU** | Branch Unit. Handles control flow instructions and mispredictions. [Source: `hdl/chisel/src/coralnpu/scalar/Bru.scala`] |
| **CLINT** | Core Local Interruptor. Manages software and timer interrupts. [Source: `hdl/chisel/src/bus/Clint.scala`] |
| **CSR** | Control and Status Register. Used for software configuration and monitoring of the hardware. [Source: `hdl/chisel/src/coralnpu/scalar/Csr.scala`] |
| **DMA** | Direct Memory Access. A subsystem for autonomous data movement between TCM, Shared SRAM, and host interfaces. [Source: `hdl/chisel/src/bus/DmaEngine.scala`] |
| **ITCM** | Instruction Tightly Coupled Memory. High-speed, local memory for instruction storage. [Source: `hdl/chisel/src/coralnpu/SramNx128.scala`] |
| **Matrix Execution Pipeline** | The pipeline within the Tensor Processing Engine (TPE) dedicated to executing high-throughput matrix multiplication and accumulation. [Source: `hdl/verilog/rvv/design/Zvt/zvt_ctrl.sv`] |
| **PLIC** | Platform-Level Interrupt Controller. Aggregates and prioritizes external interrupts. [Source: `hdl/chisel/src/bus/Plic.scala`] |
| **RVV** | RISC-V Vector extension. Defines the vector architecture supported by the CoralNPU. [Source: `hdl/chisel/src/coralnpu/rvv/RvvDecode.scala`] |
| **TCM** | Tightly Coupled Memory. Local, high-speed memory for instruction (ITCM) or data (DTCM). [Source: `hdl/chisel/src/coralnpu/SramNx128.scala`] |
| **TPE** | Tensor Processing Engine. The dedicated hardware block integrated within the RVV pipeline (Zvt array) responsible for accelerating matrix multiplication and tensor processing workloads. [Source: `hdl/verilog/rvv/design/Zvt/zvt.sv`] |
| **Zvt** | Custom Tensor Extensions for CoralNPU, providing specialized configuration instructions (`mset*`) and hardware acceleration for matrix manipulation and tensor processing. [Source: `hdl/chisel/src/coralnpu/rvv/RvvDecode.scala`] |

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-04 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/coralnpu/Parameters.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
