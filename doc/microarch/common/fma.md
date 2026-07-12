# Fused Multiply-Add (FMA)

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

> **Intended Audience:** Hardware Developers

## Overview

The `Fma` module implements a 3-stage Fused Multiply-Add pipeline, executing `(A * B) + C` using single-precision floating-point format (`Fp32`). The pipeline extracts significands, aligns them, computes the sum, and finally normalizes and rounds the result.

## Interface

| Port      | Direction | Type   | Description         |
| --------- | --------- | ------ | ------------------- |
| `cmd.ina` | Input     | `Fp32` | Input operand A     |
| `cmd.inb` | Input     | `Fp32` | Input operand B     |
| `cmd.inc` | Input     | `Fp32` | Input operand C     |
| `out`     | Output    | `Fp32` | Computed FMA result |

## Pipeline Stages

### Stage 1: Multiplication & Alignment Preparation

| Operation            | Logic Component / Behavior                                                                                                                            |
| -------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Multiplication**   | Computes `ina * inb` (significand multiplication, exponent addition).                                                                                 |
| **C Alignment Prep** | Left-pads `inc` significand by 23 bits. Computes the required right shift to align `inc` with the product.                                            |
| **Exponent/Sign**    | Determines the maximum exponent between the product and `inc`. Determines next-cycle subtraction flag (`sub`) if `(ina.sign ^ inb.sign) != inc.sign`. |
| **NaN/Inf Check**    | Detects Infinity and NaN operands early.                                                                                                              |

### Stage 2: Alignment & Accumulation

| Operation           | Logic Component / Behavior                                                                                                |
| ------------------- | ------------------------------------------------------------------------------------------------------------------------- |
| **Alignment Shift** | Shifts the `ab_significand` or applies the right shift.                                                                   |
| **Sum/Subtract**    | Performs addition or subtraction (`ab_significand + c_significand`). `c_significand` is inverted if `state1.sub` is true. |
| **Sign Resolution** | Calculates absolute value of the sum and determines the resulting sign.                                                   |

### Stage 3: Normalization & Rounding

| Operation           | Logic Component / Behavior                                                                                    |
| ------------------- | ------------------------------------------------------------------------------------------------------------- |
| **Normalization**   | Uses `PriorityEncoder` to find the leading one and left-shifts the significand.                               |
| **Rounding**        | Extracts 25-bit significand and adds 1 for rounding, subsequently producing a 23-bit mantissa.                |
| **Exponent Update** | Updates exponent based on the normalization shift amount. Checks for overflow (`Inf`) and underflow (`Zero`). |
| **Special Values**  | Emits `NaN`, `Inf`, or `Zero` based on propagation rules and calculated bounds.                               |

<!-- mdformat off -->
<!-- prettier-ignore-start -->

--------------------------------------------------------------------------------

> **Provenance & Traceability** - **Verified As Of:** 2026-07-07 - **Upstream Commit:** 8ba6f4108901602e14e28345b4bd009e6f3b6897 - **Primary Source(s):** `hdl/chisel/src/common/Fma.scala:L57` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- prettier-ignore-end -->
<!-- mdformat on -->
