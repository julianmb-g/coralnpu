# SRAM Banking and Organization

> **Intended Audience:** Hardware Developers, SW/Compiler Developers
> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

The CoralNPU utilizes a modular SRAM banking architecture to optimize memory access latency and bandwidth for the Tile-Coupled Memory (TCM). The SRAM subsystem is composed of multiple independent SRAM blocks, organized to provide high-speed, 128-bit wide data access.

## Architecture

The SRAM subsystem (`Sram_Nx128`) implements a hierarchical banking structure. The number of memory modules and their individual block size are dynamically determined based on the total number of entries (`tcmEntries`) required for a given SRAM instance.

### Banking Logic

- **Block Size**: The SRAM blocks are partitioned into `2048`, `512`, or `128` entry blocks, depending on the divisibility of the total entry count.
- **Bank Selection**: The most significant address bits (`addrBits - 1` down to `sramAddrBits`) are used to select the active SRAM module.
- **Data Access**: Only the selected bank is enabled for read/write operations to conserve power.

## Implementation Details

The banking logic is implemented in Chisel, enabling flexible memory configurations.

| Component | Logic Function | RTL Citation |
| :--- | :--- | :--- |
| Block Size | Dynamically selects block size (2048, 512, 128) | Lines 27-31 |
| Bank Select | Calculates bits for bank selection | Lines 45 |
| Module Instantiation | Maps SRAM modules to sub-blocks | Lines 37-40 |
| Bank Muxing | Logic to select and multiplex bank outputs | Lines 54-57 |

[Source: `hdl/chisel/src/coralnpu/SramNx128.scala`]

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-10 - **Upstream Commit:**
[c9d3cd8816886ced4a935722205fd47aeb72eed9](c9d3cd8816886ced4a935722205fd47aeb72eed9) -
**Primary Source(s):** `hdl/chisel/src/coralnpu/SramNx128.scala` (Lines 27-62) - **Disclaimer:**
AI-generated/assisted; RTL is the source of truth.
