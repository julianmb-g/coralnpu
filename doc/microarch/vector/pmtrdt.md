# Vector Permutation and Reduction Unit (PMTRDT)

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

> **Intended Audience:** HW Developers, SW/Compiler Developers

## Overview

The Vector Backend Permutation and Reduction Wrapper (`rvv_backend_pmtrdt`) manages the execution of three specific classes of vector instructions within the Coral NPU's Vector Core:

1. **Compare Instructions**
2. **Reduction Instructions**
3. **Permutation Instructions**

This module encapsulates the permutation and reduction execution units (`rvv_backend_pmtrdt_unit`), bridging the gap between the reservation station (RS) and the Reorder Buffer (ROB).

## Architectural Constraints & Capabilities

- **Latency:** The pipeline has a strict **2-cycle latency** for each micro-operation (uop).
- **EMUL Constraint:** For compare and reduction instructions, the effective multiplier (EMUL) of the destination vector (`vd`) is architecturally hardcoded to **1**.
- **Compress Specifics:** The vector compress instruction is treated structurally as a specialized subset of permutation instructions, executed within this same unit.
- **Floating-Point Reduction:** Floating-point reduction capability is conditionally generated based on the `ZVE32F_ON` parameter, ensuring the hardware only allocates logic if the FPU extensions are enabled.

## Core Interfaces

The `rvv_backend_pmtrdt` wrapper interfaces heavily with the vector core infrastructure, routing operations and resolving structural hazards via the ROB.

| Interface Group              | Port Name                 | Direction | Width / Type          | Description                                                                              |
| :--------------------------- | :------------------------ | :-------- | :-------------------- | :--------------------------------------------------------------------------------------- |
| **Reservation Station (RS)** | `pmtrdt_uop_rs2ex`        | Input     | `NUM_PMTRDT`          | Micro-ops issued from the dispatch reservation station.                                  |
| **Reservation Station (RS)** | `fifo_almost_empty_rs2ex` | Input     | `NUM_PMTRDT`          | Flow-control flag dictating uop validity (`~fifo_almost_empty_rs2ex`).                   |
| **Reservation Station (RS)** | `pop_ex2rs`               | Output    | `NUM_PMTRDT`          | Pop control asserting that the uop was successfully accepted by the unit.                |
| **VRF Read Interface**       | `rd_index_pmt2vrf`        | Output    | `REGFILE_INDEX_WIDTH` | Dedicated read address targeting the Vector Register File (VRF) for permutation indices. |
| **VRF Read Interface**       | `rd_data_vrf2pmt`         | Input     | `VLEN`                | Resulting vector read data returned from the VRF.                                        |
| **Reorder Buffer (ROB)**     | `result_ex2rob`           | Output    | `PU2ROB_t`            | Computed result bus formatted for the writeback arbitration network.                     |
| **Reorder Buffer (ROB)**     | `result_valid_ex2rob`     | Output    | `NUM_PMTRDT`          | Valid signal asserting the computation has completed its 2-cycle pipeline.               |
| **Reorder Buffer (ROB)**     | `result_ready_rob2ex`     | Input     | `NUM_PMTRDT`          | Backpressure signal from the ROB writeback arbiter.                                      |
| **Global Control**           | `trap_flush_rvv`          | Input     | 1-bit                 | Global vector trap signal that flushes the pipeline state.                               |

## Internal Topology

The wrapper instantiates `NUM_PMTRDT` identical units (`rvv_backend_pmtrdt_unit`), connecting the global RS arrays to discrete functional blocks. Each unit is parameterized:

- `GEN_RDT`: Enables integer reduction logic.
- `GEN_FRDT`: Enables floating-point reduction logic (tied to `ZVE32F_ON`).
- `GEN_PMT`: Enables permutation logic.

The instantiation explicitly passes the `trap_flush_rvv` signal to ensure any active multi-cycle permutations or reductions are cleanly aborted on an exception.

## Reduction ALU Implementation

The `rvv_backend_pmtrdt_unit_reduction_alu` submodule provides the combinational logic for integer reduction and compare operations.

### Data Path and EEW Handling

The Reduction ALU operates on a parameterized byte width (`ALU_BYTE`). It dynamically adjusts its element-wise processing boundaries based on the Effective Element Width (`vs2_eew`). Depending on the EEW (8, 16, 32, or 64 bits), the logic calculates the appropriate `element_byte` offset to correctly isolate arithmetic boundaries.

### Operation Mapping

The unit implements a unified adder tree that computes sum, less-than (via carry out `cout`), and bitwise operations (`AND`, `OR`, `XOR`) simultaneously.

- **Summation (`VREDSUM`, `VWREDSUM`, `VWREDSUMU`)**: Directly routes the `sum_dst` (the result of `src2 + src1 + cin`) to the output.
- **Min/Max (`VREDMIN[U]`, `VREDMAX[U]`)**: Evaluates the `lt` (less than) flag derived from the carry bit at the element boundary to select between `src1` and `src2`. Sign inversion on the input operands (`src1_tmp`, `src2_tmp`) combined with carry-in (`cin`) injection is utilized to correctly calculate signed vs. unsigned comparisons using the shared adder logic.
- **Bitwise (`VREDAND`, `VREDOR`, `VREDXOR`)**: Evaluates standard logical operations element-by-element, multiplexing them directly to `dst`.

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-03 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_pmtrdt.sv`, `hdl/verilog/rvv/design/rvv_backend_pmtrdt_unit_reduction_alu.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
