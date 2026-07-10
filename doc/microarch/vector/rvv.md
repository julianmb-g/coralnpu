# Vector Core (RVV)

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model.
> While every effort is made to ensure technical accuracy, the underlying source
> code and hardware RTL implementation remain the absolute source of truth. Use
> at your own risk.

> **Intended Audience:** Hardware Developers

The Vector Core in CoralNPU is a high-performance SIMD engine that implements a
subset of the RISC-V Vector (RVV) 1.0 specification.

[Source: hdl/chisel/src/coralnpu/rvv/RvvCore.scala | As of: 2026-06-30 | Commit: 77bc1ffe06dbf3b7bafc7eab167ead2b42668df9]

## Overview

The Vector Core is decoupled from the scalar frontend by a command FIFO. This
allows the scalar core to continue fetching and dispatching instructions while
the Vector Core processes long-running vector operations. For details on instruction assembly and dispatch from the scalar core, see the **[RVV Frontend](../../microarch/vector/rvv_frontend.md)** documentation.

[Source: hdl/verilog/rvv/design/RvvFrontEnd.sv]

### Key Specifications

- **Registers**: 32 vector registers (v0..v31).
- **VLEN**: 128 bits.
- **ELEN**: 32 bits (supports 8, 16, and 32-bit elements).
- **Lanes**: 4 instruction lanes (matching scalar core).
- **Dispatch**: Up to 3 micro-ops per cycle from a centralized command queue.

## Microarchitecture

The Vector Core is a decoupled, out-of-order execution engine. It consists of
several functional units implemented in the Verilog backend:

1. **[ALU](../../microarch/vector/rvvalu.md)**: Two integer arithmetic and logical units (VALU0 and
   VALU1). VALU0 supports all instructions including comparisons; VALU1 supports
   all arithmetic except comparisons.
1. **[PMT/RDT](../../microarch/vector/pmtrdt.md)**: One permutation and reduction unit.
1. **[DIV](../../microarch/vector/vdiv.md)**: One vector division unit.
1. **[LSU](../../microarch/memory/lsu.md)**: Two load/store units for vector memory operations.

[Source: hdl/verilog/rvv/design/rvv_backend.sv] \[Source:
hdl/verilog/rvv/inc/rvv_backend_define.svh\]

### Activation Functions

While the Vector Core does not contain a dedicated, fixed-function "Activation
Unit," it supports a wide variety of **[Activation Functions](../../sw/activations.md)**
(e.g., ReLU, Sigmoid, Tanh) through a combination of specialized hardware
features.

- **ReLU/ReLU6**: Implemented using standard vector `VMAX` and `VMIN`
  instructions in the Vector ALU.
- **Sigmoid/Tanh**: Typically implemented via software-assisted lookup tables
  (LUTs) or polynomial approximations leveraging the **[VFPU](../../microarch/vector/vfpu.md)** for
  high-precision intermediate calculations.
- **Saturating Arithmetic**: Fixed-point activations are supported through
  saturating instructions (e.g., `VSADD`, `VSSUB`) which provide automatic
  clipping to EEW limits.

For more details, see the
**[Activation Functions Documentation](../../sw/activations.md)**.

### Scalar/Vector Interop

The Vector Core supports direct movement of data between scalar floating-point
registers and vector registers:

- **`vfmv.s.f`**: Moves a scalar FP32 register value (`frs1`) into the first
  element (index 0) of a vector register.
- **`vfmv.f.s`**: Moves the first element of a vector register into a scalar
  FP32 register (`frd`).

To support these instructions, the
**[Floating-Point Register File (FRF)](../../microarch/core/fregfile.md)** arbitrates its write port
between the LSU (for floating-point loads) and the RVV core's asynchronous
writeback path. The LSU typically takes priority in case of a collision.

[Source: hdl/chisel/src/coralnpu/scalar/SCore.scala (val fRegfile arbitration)]

## System Verification Environment (SVE)

The Vector Core (RVV) relies on a dedicated SystemVerilog Verification Environment (SVE) using UVM. For comprehensive details on the specific testbenches (including `rvv_backend_tb` and `rvv_fifo_tb`) and their UVM interfaces, please refer to the **[Verification Documentation](../../verification.md#rvv-systemverilog-environment-sve)**.

## Interfaces

### Control Interface

The interface to the scalar core uses the `RvvCoreShim`.

| Signal Name | Direction | Description                         |
| ----------- | --------- | ----------------------------------- |
| inst_valid  | Input     | Valid signal for instruction lanes  |
| inst_bits   | Input     | Instruction data (PC, opcode, bits) |
| inst_ready  | Output    | Ready signal for backpressure       |

[Source: hdl/chisel/src/coralnpu/rvv/RvvCore.scala (class RvvCoreShim)]

### LSU Interface

| Signal Name           | Direction | Description        |
| --------------------- | --------- | ------------------ |
| rvv2lsu_valid         | Output    | LSU request valid  |
| rvv2lsu_vregfile_data | Output    | Vector store data  |
| lsu2rvv_valid         | Input     | LSU response valid |
| lsu2rvv_data          | Input     | Vector load data   |

[Source: hdl/chisel/src/coralnpu/rvv/RvvInterface.scala]

### Register File Interface

| Signal Name | Direction | Description                                    |
| ----------- | --------- | ---------------------------------------------- |
| rs          | Input     | Synchronous scalar register read data          |
| rd          | Output    | Synchronous scalar register write data         |
| frs         | Input     | Synchronous floating-point register read data  |
| async_rd    | Output    | Asynchronous scalar register writeback         |
| async_frd   | Output    | Asynchronous floating-point register writeback |

### CSR & Configuration Interface

| Signal Name | Direction | Description                                                             |
| ----------- | --------- | ----------------------------------------------------------------------- |
| csr         | In/Out    | Read/write interface for vector CSRs (`vstart`, `vxrm`, `vxsat`, `frm`) |
| configState | Output    | Current vector configuration state (`vl`, `vtype` fields)               |

### Status & Trap Interface

| Signal Name    | Direction | Description                                |
| -------------- | --------- | ------------------------------------------ |
| trap           | Output    | Asynchronous trap reporting interface      |
| rvv_idle       | Output    | Indicates if the vector core is fully idle |
| queue_capacity | Output    | Remaining capacity in the command queue    |

<!-- mdformat off -->
<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-03 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/chisel/src/coralnpu/rvv/RvvCore.scala`, `hdl/chisel/src/coralnpu/rvv/RvvInterface.scala`, `hdl/verilog/rvv/design/rvv_backend.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- prettier-ignore-end -->
<!-- mdformat on -->
> **Traceability:** Generated by Gemini. Derived from upstream commit 7d6e35cff17732cad8f99b866333423ea1f4d750.
