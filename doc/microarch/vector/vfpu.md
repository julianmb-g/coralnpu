# Vector Floating-Point Unit (VFPU)

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

> **Intended Audience:** Hardware Developers, Compiler Engineers

## Overview

The Vector Floating-Point Unit (VFPU) is responsible for executing vector floating-point operations. It instantiates the `fpnew` floating-point engine to handle core arithmetic operations.

## Supported Vector Extensions

The VFPU supports the following standard RISC-V Vector Extensions (RVV) for floating-point and Brain Floating-Point (BF16) operations:

- **Zvfbfmin**: Vector BF16 Conversions (Converts between FP32 and BF16).
- **Zvfbfwma**: Vector BF16 Widening Multiply-Accumulate.

### Key Instructions

| Instruction   | Extension  | Description                             |
| :------------ | :--------- | :-------------------------------------- |
| `VFWCVTBF16`  | `Zvfbfmin` | Widening conversion from BF16 to FP32.  |
| `VFNCVTBF16`  | `Zvfbfmin` | Narrowing conversion from FP32 to BF16. |
| `VFWMACCBF16` | `Zvfbfwma` | Widening multiply-accumulate on BF16.   |

## Zvfbfmin/Zvfbfwma Subnormal Handling (ADR-044)

The CoralNPU fully supports the `Zvfbfmin` and `Zvfbfwma` extensions for converting and operating on Brain Floating-Point (BF16) formats in the vector pipeline.

Crucially for numerical stability and compiler expectations, the cast units within the `fpnew` instantiation handle subnormal (denormal) values natively. Subnormal values encountered during vector conversions (such as `VFWCVTBF16` and `VFNCVTBF16`) are **NOT** flushed to zero. They are processed mathematically, preserving precision at the lower boundaries of the exponent range.

## Rounding Semantics (FpNewRoundingMode)

The following rounding modes are supported, mapped directly to the `fpnew` engine as defined in `FloatCore.scala`:

| Mnemonic | Value | Description                                     |
| :------- | :---- | :---------------------------------------------- |
| **RNE**  | 0     | Round to nearest, ties to even                  |
| **RTZ**  | 1     | Round to zero                                   |
| **RDN**  | 2     | Round down (towards -inf)                       |
| **RUP**  | 3     | Round up (towards +inf)                         |
| **RMM**  | 4     | Round to nearest, ties to max magnitude         |
| **DYN**  | 7     | Dynamic rounding mode (embedded in instruction) |

These modes correspond to the standard RISC-V rounding modes. See the RISC-V Unprivileged spec, Chapter 20.2 for details.

## Core Operations (FpNewOperation)

The VFPU supports the following operations, mapped to the `fpnew` core arithmetic engine:

| Mnemonic     | Value | Description                                 |
| :----------- | :---- | :------------------------------------------ |
| **FMADD**    | 0     | Fused Multiply-Add                          |
| **FNMSUB**   | 1     | Fused Negative Multiply-Subtract            |
| **ADD**      | 2     | Floating-Point Addition/Subtraction         |
| **MUL**      | 3     | Floating-Point Multiplication               |
| **DIV**      | 4     | Floating-Point Division                     |
| **SQRT**     | 5     | Floating-Point Square Root                  |
| **SGNJ**     | 6     | Sign-Injection (SGNJ, SGNJN, SGNJX)         |
| **MINMAX**   | 7     | Floating-Point Minimum/Maximum              |
| **CMP**      | 8     | Floating-Point Comparisons                  |
| **CLASSIFY** | 9     | Floating-Point Classify                     |
| **F2F**      | 10    | Floating-Point to Floating-Point Conversion |
| **F2I**      | 11    | Floating-Point to Integer Conversion        |
| **I2F**      | 12    | Integer to Floating-Point Conversion        |
| **CPKAB**    | 13    | Cast and Pack AB                            |
| **CPKCD**    | 14    | Cast and Pack CD                            |
| **STORE**    | 15    | FP Store (Special value, internal use only) |

Note: `STORE` (15) is used internally by the VFPU to identify store-related data movements and is not a functional `fpnew` arithmetic operation.

## Exception Reporting

Floating-point exceptions are reported via a 5-bit `status_o` vector from the `fpnew` engine, which is routed directly to the `fflags` field of the CSR interface (`io.csr.in.fflags.bits`). Exception flags are updated when valid results are produced (`out_valid_o`) and the instruction is not a floating-point move (`!fmv`).

| Bit | Mnemonic | Description       |
| :-- | :------- | :---------------- |
| 4   | **NV**   | Invalid Operation |
| 3   | **DZ**   | Divide by Zero    |
| 2   | **OF**   | Overflow          |
| 1   | **UF**   | Underflow         |
| 0   | **NX**   | Inexact           |

Note: The bit mapping follows the standard RISC-V `fflags` definition.

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-22 - **Upstream Commit:** 5c2647afd951f70d6244ea06b5a8b7fa1fdf2918 - **Primary Source(s):** `hdl/chisel/src/coralnpu/float/FloatCore.scala`, `hdl/chisel/src/common/Fp.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
