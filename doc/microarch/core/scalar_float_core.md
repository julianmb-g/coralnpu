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

# Scalar float core

> **Intended Audience:** HW Devs, SW/Compiler Devs

The Scalar Float Core (FloatCore) provides hardware support for IEEE 754-2008 compliant single-precision floating-point operations (RV32F) and Bfloat16 conversions (Zfbfmin). It utilizes the `fpnew` execution engine as its primary computational unit for scalar operations.

## Architectural function

The Scalar Float Core is responsible for decoding and executing scalar floating-point instructions dispatched from the main processor pipeline. It manages operand routing between the scalar and floating-point register files and handles floating-point status and control registers.

### Key features

- **RV32F Support**: Full implementation of the RISC-V single-precision floating-point extension.

- **Zfbfmin Support**: Support for scalar `FCVT.S.BF16` and `FCVT.BF16.S` instructions for Bfloat16 integration.

- **Fused Multiply-Add**: High-performance FMADD, FMSUB, NMADD, and NMSUB operations.

- **Exceptions & Rounding**: Hardware management of `fflags` and `frm` fields.

## Scalar FPU execution pipeline

The scalar Floating-Point Unit (FPU) in the CoralNPU architecture is built around a pipelined Fused Multiply-Add (FMA) core. It translates standard floating-point operations into FMA equivalents and processes them through a multi-stage, decoupled pipeline.

### Fpu command translation (`fpucmd`)

The FPU receives `FpuCmd` bundles containing an operation type (`FpuOptype`) and up to three operands (`ina`, `inb`, `inc`). These commands are combinationally translated into standard `FmaCmd` (`ina * inb + inc`) formats.

| Operation Type | `FmaCmd.ina` | `FmaCmd.inb` | `FmaCmd.inc` | Equivalent Math    |
| -------------- | ------------ | ------------ | ------------ | ------------------ |
| `FpuAdd`       | `ina`        | `1.0`        | `inc`        | `ina * 1.0 + inc`  |
| `FpuSub`       | `ina`        | `1.0`        | `-inc`       | `ina * 1.0 - inc`  |
| `FpuMul`       | `ina`        | `inb`        | `0.0`        | `ina * inb + 0.0`  |
| `FpuFma`       | `ina`        | `inb`        | `inc`        | `ina * inb + inc`  |
| `FpuFms`       | `ina`        | `inb`        | `-inc`       | `ina * inb - inc`  |
| `FpuFnma`      | `ina`        | `inb`        | `inc`        | `-(ina * inb + inc)` (RISC-V FNMADD) |
| `FpuFnms`      | `ina`        | `inb`        | `inc`        | `-(ina * inb - inc)` (RISC-V FNMSUB) |

_Note: For addition and subtraction, `inb` is tied to the FP32 representation of `1.0` (`127.U(8.W)` exponent, `0` mantissa)._

### Pipeline stages

The FPU executes operations across a three-stage pipeline, utilizing the underlying `common.Fma` hardware block. The stages are separated by decoupled queues (FIFOs) with a depth of 1 and `flow=true` to handle backpressure gracefully.

- **Stage 1 (`FmaStage1`)**: The translated `FmaCmd` enters the first stage of the FMA arithmetic directly from the input translation logic.

- **Stage 2 (`FmaStage2`)**: State from Stage 1 is buffered through a queue into the second FMA processing stage.

- **Stage 3 (`FmaStage3`)**: State from Stage 2 is buffered through another queue into the final FMA processing stage. The result is then decoupled and driven to the output port.

### Scalar FPU division & square root (ADR-146)

The scalar FPU includes hardware support for scalar floating-point division (`fdiv.s`) and square root (`fsqrt.s`) operations. While the basic `FpuCmd` wrapper handles FMA operations, these complex operations are mapped through `hdl/chisel/src/coralnpu/float/FloatCore.scala`. The instruction decoding logic maps the `funct5` fields `0b00011` to `FpNewOperation.DIV` and `0b01011` to `FpNewOperation.SQRT`. The `FloatCore` implementation dynamically generates the SystemVerilog instantiation (`GenerateCoreShimSource`) and conditionally injects the required physical datapath sources (`fpnew_divsqrt_th_32.sv` and `fpnew_divsqrt_multi.sv`) based on the configured NPU parameters. The internal multi-cycle routing is handled entirely within the third-party `fpnew` IP.

## Interfaces

The Scalar Float Core interfaces with the instruction queue, register files, and the Load/Store Unit (LSU).

### Port definitions

| Port Group | Type | Description |
| :--- | :--- | :--- |
| `inst` | Flipped(Decoupled) | Input instruction stream from the dispatcher (FloatInstruction). |
| `read_ports` | Flipped(Vec(3)) | Read interfaces to the Floating-Point Register File (FRF) (FRegfileRead). |
| `write_ports` | Flipped(Vec(2)) | Write interfaces to the FRF (FRegfileWrite). |
| `rs1`, `rs2` | Flipped | Read interfaces to the Scalar Register File (RegfileReadDataIO). |
| `scalar_rd` | Decoupled | Write interface to the Scalar Register File for F->X moves and comparisons (RegfileWriteDataIO). |
| `csr` | Bundle | Interface to `fflags` and `frm` status and control registers (CsrFloatIO). |
| `lsu_rd` | Flipped(Valid) | Direct write path for floating-point load data from the LSU (FloatRegfileWriteDataIO). |

[Source: [`hdl/chisel/src/coralnpu/float/FloatCoreInterface.scala`](../../../hdl/chisel/src/coralnpu/float/FloatCoreInterface.scala)]

## Execution pipeline

The Scalar Float Core uses a `BlackBox` wrapper (`FloatCoreWrapper`) for the `fpnew_top` module. It manages the handshaking and operand preparation required for the FPU.

### Operand routing

Instructions can source operands from either the floating-point register file or the scalar register file (e.g., `FMV.W.X`, `FCVT.S.W`).

| Opcode | RS1 Source | RS2 Source | RS3 Source |
| :--- | :--- | :--- | :--- |
| `LOADFP` / `STOREFP` | Scalar | FRF (Store only) | N/A |
| `OPFP` | FRF / Scalar | FRF | N/A |
| `MADD` / `MSUB` | FRF | FRF | FRF |

[Source: [`hdl/chisel/src/coralnpu/float/FloatCore.scala`](../../../hdl/chisel/src/coralnpu/float/FloatCore.scala)]

### Rounding mode logic

The Scalar Float Core validates the rounding mode (`rm`) specified in the instruction against the architectural `frm` CSR. If the instruction specifies dynamic rounding (`0b111`), it uses the value from the CSR. If the CSR value is also invalid, the instruction is stalled or marked as invalid.

[Source: [`hdl/chisel/src/coralnpu/float/FloatCoreInterface.scala`](../../../hdl/chisel/src/coralnpu/float/FloatCoreInterface.scala), `FloatCore.scala`]

## Edge cases and behavior

### Zfbfmin conversions

When `enableZfbfmin` is active, the Scalar Float Core supports Bfloat16 to/from FP32 conversions. These are mapped to the `F2F` operation group in `fpnew` with specific format identifiers (`FP16ALT` for BF16).

### Backpressure handling

The Scalar Float Core uses an internal instruction queue (`instQueue`) and tracks the FPU's busy state (`busy_o`). It holds the current instruction until both the FPU and the output scalar writeback bus (if applicable) are ready to accept it.

[Source: [`hdl/chisel/src/coralnpu/float/FloatCore.scala`](../../../hdl/chisel/src/coralnpu/float/FloatCore.scala)]

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** [hdl/chisel/src/coralnpu/float/FloatCore.scala](../../../hdl/chisel/src/coralnpu/float/FloatCore.scala), [hdl/chisel/src/coralnpu/float/FloatCoreInterface.scala](../../../hdl/chisel/src/coralnpu/float/FloatCoreInterface.scala), [`hdl/chisel/src/coralnpu/scalar/Fpu.scala:L32`](../../../hdl/chisel/src/coralnpu/scalar/Fpu.scala) - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
