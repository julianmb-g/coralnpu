# Scalar Floating-Point Unit (FPU)

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

The CoralNPU scalar core integrates a Floating-Point Unit (FPU) based on the `fpnew` execution engine, instantiated via the `FloatCore` module. It supports standard RV32F instructions alongside custom operations.

## Interfaces

### Port Definitions

| Signal Name        | Direction | Type                             | Description                                                      |
| :----------------- | :-------- | :------------------------------- | :--------------------------------------------------------------- |
| `io.inst`          | Input     | `Decoupled(FloatInstruction)`    | Instruction input stream with ready/valid handshake.             |
| `io.read_ports`    | Output    | `Vec(3, FRegfileRead)`           | Float Register File read ports (address and valid out, data in). |
| `io.write_ports`   | Output    | `Vec(2, FRegfileWrite)`          | Float Register File write ports (address, data, and valid out).  |
| `io.rs1`           | Input     | `RegfileReadDataIO`              | Scalar register read data 1 (used for I2F/FMV instructions).     |
| `io.rs2`           | Input     | `RegfileReadDataIO`              | Scalar register read data 2.                                     |
| `io.scalar_rd`     | Output    | `Decoupled(RegfileWriteDataIO)`  | Scalar register file write port with ready/valid handshake.      |
| `io.csr.in.fflags` | Output    | `Valid(UInt(5.W))`               | Floating-point exception flags output to CSR.                    |
| `io.csr.out.frm`   | Input     | `UInt(3.W)`                      | Floating-point rounding mode from CSR.                           |
| `io.lsu_rd`        | Input     | `Valid(FloatRegfileWriteDataIO)` | Load data from Load/Store Unit for float registers.              |

## Floating-Point Hazard Tracking

Intra-cycle and inter-cycle Data Hazards (RAW/WAW) for floating-point instructions are tracked centrally in the **Decode** module via a combinational scoreboard (`fcomb`). The Decode module natively tracks `rs1`, `rs2`, and `rs3` operands. This explicitly ensures intra-cycle RAW hazard avoidance for 4-operand floating-point instructions like `MADD`, `MSUB`, `NMADD`, and `NMSUB` (which require `rs3`). If a same-cycle match against a pending floating-point write is detected, the instruction is stalled.

## Zfbfmin Conversion Support

The FPU supports the RISC-V `Zfbfmin` extension for bfloat16 conversions (`FCVT.S.BF16` and `FCVT.BF16.S`). The `FloatCoreInterface` explicitly decodes these operations (`rs2=0b00110` and `rs2=0b01000`), mapping them to the internal `FP16ALT` format for processing by the underlying `fpnew` cast units.

### Subnormal (Denormal) Handling (ADR-044)

The cast units mathematically support full subnormal (denormal) value processing natively.

- **No Flush-to-Zero**: Denormal values are **NOT** flushed to zero during conversions. Compiler developers should be aware that the hardware fully retains subnormal precision as dictated by the IEEE 754 bfloat16 standard during upcasts/downcasts.

## Float Division and Canonical NaN Inconsistencies (ADR-019)

Vector and scalar float division operations utilize the `fpnew` divider core.

- **Deviation**: Certain rounding modes in float division instructions currently differ from the upstream reference models (e.g. UVM reference model) regarding canonical NaN generation. As a result, explicit UVM tests targeting these specific canonical NaN rounding behaviors are excluded from the test suite. Software utilizing float division with strict canonical NaN propagation expectations should be cautious of this divergence.

For details on the pipeline stages and operand translation logic for FpuCmd, see [Scalar FPU Execution Pipeline](fpu_pipeline.md).

<!-- mdformat off -->
<!-- prettier-ignore -->
## Floating-Point Rounding Modes

The FPU supports the standard RISC-V floating-point rounding modes, which are controlled either by the `frm` field in the `fcsr` register or specified directly in the rounding mode field of individual floating-point instructions:

| Mode | Mnemonic | Description                             |
| :--- | :------- | :-------------------------------------- |
| 000  | RNE      | Round to Nearest, ties to Even          |
| 001  | RTZ      | Round towards Zero                      |
| 010  | RDN      | Round Down (towards $-\infty$)          |
| 011  | RUP      | Round Up (towards $+\infty$)            |
| 100  | RMM      | Round to Nearest, ties to Max Magnitude |

These rounding modes are applied by the `fpnew` execution engine based on the `rm` field in `FloatInstruction` (when `rm` is not `111`). When `rm` is `111`, the rounding mode is taken from the CSR `frm` field (as handled in `FloatInstruction.valid_frm`). These rounding modes equally apply to the Vector Floating-Point Unit (VFPU), which propagates them to vector instructions via the `rvv_backend_falu_unit.sv` and `rvv_backend_fdiv_wrapper.sv` components.

---

**Provenance & Traceability** - **Verified As Of:** 2026-07-22 - **Upstream Commit:** c9d3cd8816886ced4a935722205fd47aeb72eed9 - **Primary Source(s):** `hdl/chisel/src/coralnpu/float/FloatCore.scala`, `hdl/chisel/src/coralnpu/float/FloatCoreInterface.scala:63-71`, `hdl/verilog/rvv/design/rvv_backend_falu_unit.sv`, `hdl/verilog/rvv/design/rvv_backend_fdiv_wrapper.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->

> **Traceability:** Generated by Gemini. Derived from upstream commit f05a63aa421b1c7880e6fb2309e5e2c0e35607c3.
