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

# System Overview

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model.
> While every effort is made to ensure technical accuracy, the underlying source
> code and hardware RTL implementation remain the absolute source of truth. Use
> at your own risk.
>
> **Intended Audience:** HW Integrators, HW Devs

This document provides a high-level overview of the CoralNPU system architecture, host interfaces, memory hierarchy, and hardware capability boundaries.

## Top-level architecture

The CoralNPU IP is designed as a standalone accelerator block for SoC integration. It features a scalar frontend driving a decoupled vector backend, optimized for ML workloads.

For a detailed block diagram and subsystem relationships, refer to the [Top-Level Subsystem](top_level_subsystem.md) documentation.

## Host interface boundaries

The CoralNPU IP interacts with the host system through standardized **AXI4** interfaces, retaining **TLUL** ports in the RTL implementation to maintain implementation reality.

- **AXI4 Host (Master)**: Used by the NPU to access external shared SRAM or DRAM for instruction/data fetching when internal TCMs miss.
- **AXI4 Device (Slave)**: Used by the host CPU to access the NPU's Control and Status Registers (CSRs) for configuration and control.

## Memory hierarchy

The CoralNPU features a deterministic memory hierarchy optimized for real-time inference.

- **Tightly Coupled Memory (TCM)**:
  - **DTCM**: Data TCM, multi-banked SRAM for vector and scalar data access.
- **Shared SRAM/DRAM**: Accessed via the external AXI4 bus.
- **CSR Space**: Dedicated memory-mapped region for controlling the IP.

## Clocking, reset, and power

### Clock management

The CoralNPU IP block receives an external source clock (`aclk`). Internal clock distribution is managed via coarse-grained clock gating (`ClockGate`) to minimize dynamic power consumption.
The clock is enabled only when active work is pending, interrupts are registered, or debug mode is active. Fine-grained clock gating is explicitly **not** supported within functional modules.

### Reset sequencing

Reset synchronization is handled by the `RstSync` module, which synchronizes external asynchronous resets into internal synchronous de-assertion.
To ensure clean startup transitions, the internal clock is actively held disabled during reset and for a fixed delay post-deassertion.

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-23 - **Upstream Commit:** 5c2647afd951f70d6244ea06b5a8b7fa1fdf2918 - **Primary Source(s):** `doc/system_overview.md` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
