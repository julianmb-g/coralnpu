# Activation Functions

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

> **Intended Audience:** SW/Compiler Developers


The CoralNPU handles non-linear activation functions through a combination of specialized Vector ISA instructions and software-assisted techniques, leveraging the [Vector Core (RVV)](../microarch/vector/rvv.md) to support a wide variety of activation types (ReLU, Sigmoid, Tanh, etc.).

## Overview

In machine learning, activation functions introduce non-linearity into the model, allowing it to learn complex patterns. The CoralNPU provides hardware support for common activations:

- **ReLU (Rectified Linear Unit)**: Performed using vector minimum/maximum instructions.
- **ReLU6**: ReLU with a cap at 6.0, often used in mobile models.
- **Sigmoid / Tanh**: Handled via polynomial approximation or Lookup Tables (LUTs) leveraging the [VFPU](../microarch/vector/vfpu.md).
- **Saturating Arithmetic**: Prevents overflow during accumulation, providing a natural "clip" for many integer-based activation schemes.

## ReLU & Clipping

ReLU and its variants (ReLU6, Leaky ReLU) are the most common activations in modern NPU workloads. The CoralNPU executes these using the [Vector ALU (VALU)](../microarch/vector/valu.md).

### Implementation via VMIN/VMAX

- **ReLU**: `VMAX.VX vd, vs2, x0` (where `x0` contains 0).
- **ReLU6**: A combination of `VMAX` (bottom clip at 0) and `VMIN` (top clip at 6).
- **Leaky ReLU**: Implemented using a masked approach where negative elements are multiplied by a small scalar constant before being merged back.

[Source: `hdl/verilog/rvv/design/rvv_backend_alu_unit_addsub.sv` | Instructions: VMIN, VMAX]

## Saturating Arithmetic

For quantized (integer) models, activation functions often involve clipping the result to the range of the target precision (e.g., -128 to 127 for int8). The CoralNPU supports **saturating arithmetic** which automatically performs this clipping.

- **VALU Instructions**: `VSADD` (Saturating Add), `VSSUB` (Saturating Subtract).
- **Vector MAC Instructions**: `VSMUL` (Saturating Multiply).
- **Status Flag**: If saturation occurs, the `vxsat` bit in the CSR is set, allowing software to detect and handle overflow if necessary.

[Source: `hdl/verilog/rvv/design/rvv_backend_alu_unit_addsub.sv` (VSADD/VSSUB)]

## Transcendental Activations (Sigmoid/Tanh)

Complex non-linear functions like Sigmoid ($1 / (1 + e^{-x})$) and Tanh are implemented using the [Vector Floating-Point Unit (VFPU)](../microarch/vector/vfpu.md).

### Approximation Techniques

1. **Polynomial Approximation**: The VFPU's high-throughput FMA units allow for fast execution of Taylor series or minimax polynomial approximations.
2. **Lookup Tables (LUTs)**: For higher performance, software can pre-calculate activation values into a table stored in [TCM](../microarch/memory/tcm.md). The `VRGATHER` instruction can then be used to perform high-speed vector lookups.
3. **Reciprocal Estimates**: The VFPU provides 7-bit reciprocal estimates (`VFRECE7.V`), which can be used to accelerate Sigmoid calculations.
4. **Vectorized Exponentials (Online Softmax)**: Optimized vectorized exponential approximation routines are also used in high-throughput multi-head operations. For a detailed architectural breakdown of the natural exponent approximation algorithm, see the [Multi-Head FlashAttention RVV Kernel](./flash_attention.md).

[Source: `hdl/verilog/rvv/design/rvv_backend_falu_unit.sv`]

## Zfbfmin Subnormal Handling (ADR-044)

The CoralNPU cast units mathematically support full subnormal (denormal) value processing natively. When converting between 32-bit single-precision and 16-bit bfloat16 formats (via `FCVT.S.BF16` and `FCVT.BF16.S`), subnormal values are **NOT** flushed to zero. They are processed mathematically, preserving precision at the lower boundaries of the exponent range. This behavior is crucial for numerical stability across complex activation profiles.

## Performance

| Activation    | Mechanism   | Unit    | Throughput                 |
| ------------- | ----------- | ------- | -------------------------- |
| ReLU          | VMAX        | VALU    | 1 instr / cycle            |
| ReLU6         | VMAX + VMIN | VALU    | 0.5 instr / cycle          |
| Sigmoid (LUT) | VRGATHER    | PMT/RDT | 0.125 - 0.25 instr / cycle |
| Tanh (Poly)   | FMA         | VFPU    | ~0.1 instr / cycle         |

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-03 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_alu_unit_addsub.sv`, `hdl/verilog/rvv/design/rvv_backend_falu_unit.sv`, `hdl/verilog/rvv/design/rvv_backend_pmtrdt_unit_permutation.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->

> **Traceability:** Generated by Gemini. Derived from upstream commit 9a1e82634c2b0f3d42310f89cd1484d8f3302ec9.
