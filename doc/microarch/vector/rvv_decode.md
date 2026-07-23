# RVV Instruction Decoding

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

> **Intended Audience:** Hardware Developers, SW/Compiler Devs

The CoralNPU Vector Core implements customized decoding logic to enforce architectural constraints and hardware limitations. This document details the constraints applied during the vector instruction decoding phase.

## Register Field Extraction

The following table details the exact bit positions of the fields extracted from standard 32-bit RISC-V vector instructions during decoding, as well as the internal 25-bit compressed payload used in the vector pipeline.

| Field    | Description                                  | 32-bit Inst Bits | 25-bit Payload Bits |
| :------- | :------------------------------------------- | :--------------- | :------------------ |
| `vd`     | Destination Vector Register                  | `inst(11,7)`     | `bits(4,0)`         |
| `funct3` | Function 3 / Mode / Width                    | `inst(14,12)`    | `bits(7,5)`         |
| `vs1`    | Source Vector Register 1 / imm5 / scalar reg | `inst(19,15)`    | `bits(12,8)`        |
| `vs2`    | Source Vector Register 2                     | `inst(24,20)`    | `bits(17,13)`       |
| `vm`     | Vector Mask                                  | `inst(25)`       | `bits(18)`          |
| `mop`    | Addressing Mode (Load/Store)                 | `inst(27,26)`    | `bits(20,19)`       |
| `mew`    | MEW (Load/Store)                             | `inst(28)`       | `bits(21)`          |
| `funct6` | Function 6                                   | `inst(31,26)`    | `bits(24,19)`       |

> [!NOTE] > `funct3` acts as `mode` for Vector ALU operations and `width` for Vector Load/Store operations.

## Instruction Classification Logic

The decoding logic categorizes instructions to determine their dataflow constraints, specifically defining whether an instruction writes to a vector register or a scalar register. This categorization is vital for tracking data dependencies and managing vector register file allocations.

### Vector Register Writes (`writesVectorRegister`)

An instruction writes to a vector register if it is:

1. A **Vector Load** operation (`RVVLOAD`).
2. A **Vector ALU** operation (`RVVALU`) that does _not_ write to an integer scalar register (`writesRd()`) and does _not_ write to a floating-point scalar register (`writesFrd()`).

Vector Store operations (`RVVSTORE`) do not write to vector registers.

### Scalar Register Exclusions

The following instructions are explicitly excluded from writing to vector registers because they write to scalar targets instead:

- **Integer Scalar Writes (`writesRd()`)**:
  - `vsetvl` / `vsetvli` configuration instructions.
  - `vmv.x.s` (Vector-scalar move).
  - `vcpop` (Population count).
  - `vfirst` (Find first set bit).
  - These are identified internally as `RVVALU` with `funct3 == "b010"` and `funct6 == "b010000"`.
- **Floating-Point Scalar Writes (`writesFrd()`)**:
  - `vfmv.f.s` (Vector-float scalar move).
  - Identified internally as `RVVALU` with `funct3 == "b001"` and `funct6 == "b010000"`.

## Zero-vstart Constraints

Certain vector instructions within the CoralNPU architecture enforce a strict zero `vstart` constraint at the decode stage. If these instructions are executed when the `vstart` CSR is non-zero, the hardware will generate a trap. This constraint is explicitly modeled by the `requireZeroVstart()` function in the decoder.

This behavior primarily affects instructions that accumulate state across vector elements or perform complex permutations where resuming from an arbitrary element index is microarchitecturally unsupported.

### Affected Instructions

The following classes of instructions require `vstart == 0`:

#### Reduction Instructions

All vector reduction operations require a zero `vstart`.

- `vredsum`
- `vredand`
- `vredor`
- `vredxor`
- `vredminu`
- `vredmin`
- `vredmaxu`
- `vredmax`
- `vwredsumu`
- `vwredsum`

#### Mask and Unary Operations (VWXUNARY0 / VMUNARY0)

Specific unary and mask population operations enforce this constraint.

- `vcpop` (Vector count population in mask)
- `vfirst` (Vector find first set mask bit)
- `vmsbf` (Vector mask set-before-first)
- `vmsof` (Vector mask set-only-first)
- `vmsif` (Vector mask set-including-first)
- `viota` (Vector iota)

#### Compression Instructions

- `vcompress` (Vector compress instruction)

## Hardware Support for BFloat16 Vector Extensions (Zvfbfmin & Zvfbfwma)

The CoralNPU hardware natively supports BFloat16 vector extensions (`Zvfbfmin` and `Zvfbfwma`) to facilitate high-throughput widening multiply-accumulate and format rounding/conversions.

### Chisel-Level Configuration

Hardware support is conditionally parameterized at build time via the Chisel parameter `enableVectorBf16` (defined in `Parameters.scala`). It is controlled via the build system using the Chisel compiler flag:

```bash
--enableVectorBf16=True
```

When enabled, `RvvCore.scala` generates the `RVV_CONFIG_SVH` header with the `ZVFBFWMA_ON` macro defined. If disabled, the macro remains undefined in the generated header.

### Instruction Decoder Gating

The instruction decoder in Chisel (`RvvDecode.scala`) decodes BFloat16 opcodes unconditionally. However, the backend SystemVerilog decoders (`rvv_backend_decode_unit_ari.sv` and `rvv_backend_decode_unit_ari_de2.sv`) utilize conditional logic gated by the `ZVFBFWMA_ON` macro to validate and process these opcodes. When enabled, the decoder maps the following instructions:

| Instruction       | Operation                    | Type / Format                                        |
| :---------------- | :--------------------------- | :--------------------------------------------------- |
| **`VFWMACCBF16`** | Widening Multiply-Accumulate | BFloat16 $\times$ BFloat16 + FP32 $\rightarrow$ FP32 |
| **`VFNCVTBF16`**  | Narrowing Convert            | FP32 $\rightarrow$ BFloat16                          |
| **`VFWCVTBF16`**  | Widening Convert             | BFloat16 $\rightarrow$ FP32                          |

These operations are classified as floating-point widening ops (`is_float = true.B`, `is_widening = true.B`).

### Floating-Point ALU (FALU) Submodule Configuration

Within the execution pipeline, the Floating-Point ALU (`rvv_backend_falu_unit.sv`) instantiates `fpnew_fma_multi` submodules (e.g., `addmul` in sub-unit 0) from the FPnew package to execute these instructions.

- When `ZVFBFWMA_ON` is defined, the `FpFmtConfig` parameter on `fpnew_fma_multi` is set to `5'b10001`, which activates alternative FP16 support (specifying alternative half-precision BFloat16 formatting).
- When `ZVFBFWMA_ON` is undefined, the parameter defaults to `5'b10000`, which disables BFloat16 formatting paths.

### Negative Space & Structural Boundaries

- **No Standard IEEE-754 FP16 Support**: The hardware is **NOT** designed to support standard IEEE-754 half-precision FP16 operations under these extensions; it exclusively supports the alternative BFloat16 (FP16ALT) format. Downstream software and compiler developers must ensure that any FP16 data is formatted according to the alternative BFloat16 structure, as executing standard IEEE-754 half-precision instructions will result in undefined or unsupported hardware states.

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-22 - **Upstream Commit:** 5c2647afd951f70d6244ea06b5a8b7fa1fdf2918 - **Primary Source(s):** `hdl/chisel/src/coralnpu/rvv/RvvDecode.scala`, `hdl/verilog/rvv/design/rvv_backend_falu_unit.sv`, `hdl/chisel/src/coralnpu/rvv/RvvCore.scala`, `hdl/verilog/rvv/design/rvv_backend_decode_unit_ari.sv`, `hdl/verilog/rvv/design/rvv_backend_decode_unit_ari_de2.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->

> **Traceability:** Generated by Gemini. Derived from upstream commit f05a63aa421b1c7880e6fb2309e5e2c0e35607c3.
