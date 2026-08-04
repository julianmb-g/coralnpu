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

# Vector ISA and execution architecture

> **Intended Audience:** SW/Compiler Devs

## Vector control and status registers (csrs)

The CoralNPU implements standard RISC-V Vector CSRs, including `vtype` and `vl`, as defined by the RISC-V Vector Specification v1.0.

### Vector type register (`vtype`)

The `vtype` read-only CSR configures the current default vector layout, element width, and multiplier. In the CoralNPU Chisel/RTL implementation, the fields of `vtype` are mapped as follows:

| Bit Range | Field Name | Width (Bits) | Description |
| :--- | :--- | :--- | :--- |
| `xlen-1` (31) | `vill` | 1 | Vector Instruction Illegal. Set to 1 if the configuration requested by `vset{i}vl{i}` is unsupported. |
| `xlen-2:8` (30:8) | `0` (Reserved) | `xlen - 9` (23) | Reserved bits. Must be hardwired to zero. |
| 7 | `ma` | 1 | Mask Agnostic. Controls how elements under mask are updated. |
| 6 | `ta` | 1 | Tail Agnostic. Controls how tail elements are updated. |
| 5:3 | `sew` | 3 | Selected Element Width. Determines the default element bit width. |
| 2:0 | `lmul_orig` | 3 | Original Vector Register Grouping (LMUL). The multiplier configuration as set in `vset(i)vl(i)`. |

### Vector length register (`vl`)

The `vl` CSR defines the active vector length for vector instruction execution. The width of `vl` is parameterized based on the physical vector register length (`p.rvvVlen`):

- **Width:** `log2Ceil(p.rvvVlen + 1)` bits.

- **Value Range:** `0` to `VLMAX` (where `VLMAX` corresponds to the maximum element index given `sew` and `lmul` grouping constraints).

## Vector ALU instruction encodings (OP-V)

The following tables list the supported Vector ALU operations decoded by `RvvDecode.scala` (Opcode `1010111`).

### Opivv - vector-vector operations (funct3 = 000)

| Instruction    |  Funct6  | VM  | Constraints                              |
| :------------- | :------: | :-: | :--------------------------------------- |
| `VADD`         | `000000` | 0/1 |                                          |
| `VSUB`         | `000010` | 0/1 |                                          |
| `VMINU`        | `000100` | 0/1 |                                          |
| `VMIN`         | `000101` | 0/1 |                                          |
| `VMAXU`        | `000110` | 0/1 |                                          |
| `VMAX`         | `000111` | 0/1 |                                          |
| `VAND`         | `001001` | 0/1 |                                          |
| `VOR`          | `001010` | 0/1 |                                          |
| `VXOR`         | `001011` | 0/1 |                                          |
| `VRGATHER`     | `001100` | 0/1 | No overlap (`vd != vs1` and `vd != vs2`) |
| `VRGATHEREI16` | `001110` | 0/1 | No overlap (`vd != vs1` and `vd != vs2`) |
| `VADC`         | `010000` |  0  | Mask required (`vd != 0`)                |
| `VMADC`        | `010001` | 0/1 |                                          |
| `VSBC`         | `010010` |  0  | Mask required (`vd != 0`)                |
| `VMSBC`        | `010011` | 0/1 |                                          |
| `VMERGE`       | `010111` |  0  | Mask required                            |
| `VMV`          | `010111` |  1  | Mask not available                       |
| `VMSEQ`        | `011000` |  0  | Mask required                            |
| `VMSNE`        | `011001` |  0  | Mask required                            |
| `VMSLTU`       | `011010` |  0  | Mask required                            |
| `VMSLT`        | `011011` |  0  | Mask required                            |
| `VMSLEU`       | `011100` |  0  | Mask required                            |
| `VMSLE`        | `011101` |  0  | Mask required                            |
| `VSADDU`       | `100000` | 0/1 |                                          |
| `VSADD`        | `100001` | 0/1 |                                          |
| `VSSUBU`       | `100010` | 0/1 |                                          |
| `VSSUB`        | `100011` | 0/1 |                                          |
| `VSLL`         | `100101` | 0/1 |                                          |
| `VSMUL`        | `100111` | 0/1 |                                          |
| `VSRL`         | `101000` | 0/1 |                                          |
| `VSRA`         | `101001` | 0/1 |                                          |
| `VSSRL`        | `101010` | 0/1 |                                          |
| `VSSRA`        | `101011` | 0/1 |                                          |
| `VNSRL`        | `101100` | 0/1 |                                          |
| `VNSRA`        | `101101` | 0/1 |                                          |
| `VNCLIPU`      | `101110` | 0/1 |                                          |
| `VNCLIP`       | `101111` | 0/1 |                                          |

### Opivx - vector-scalar operations (funct3 = 100)

| Instruction  |  Funct6  | VM  | Constraints                     |
| :----------- | :------: | :-: | :------------------------------ |
| `VADD`       | `000000` | 0/1 |                                 |
| `VSUB`       | `000010` | 0/1 |                                 |
| `VRSUB`      | `000011` | 0/1 |                                 |
| `VMINU`      | `000100` | 0/1 |                                 |
| `VMIN`       | `000101` | 0/1 |                                 |
| `VMAXU`      | `000110` | 0/1 |                                 |
| `VMAX`       | `000111` | 0/1 |                                 |
| `VAND`       | `001001` | 0/1 |                                 |
| `VOR`        | `001010` | 0/1 |                                 |
| `VXOR`       | `001011` | 0/1 |                                 |
| `VRGATHER`   | `001100` | 0/1 | No overlap (`vd != vs2`)        |
| `VSLIDEUP`   | `001110` | 0/1 | No overlap (`vd != vs2`)        |
| `VSLIDEDOWN` | `001111` | 0/1 | No overlap (`vd != vs2`)        |
| `VADC`       | `010000` |  0  | Mask required (`vd != 0`)       |
| `VMADC`      | `010001` | 0/1 |                                 |
| `VSBC`       | `010010` |  0  | Mask required (`vd != 0`)       |
| `VMSBC`      | `010011` | 0/1 |                                 |
| `VMERGE`     | `010111` |  0  | Mask required                   |
| `VMV`        | `010111` |  1  | Mask not available (`vs2 == 0`) |
| `VMSEQ`      | `011000` |  0  | Mask required                   |
| `VMSNE`      | `011001` |  0  | Mask required                   |
| `VMSLTU`     | `011010` |  0  | Mask required                   |
| `VMSLT`      | `011011` |  0  | Mask required                   |
| `VMSLEU`     | `011100` |  0  | Mask required                   |
| `VMSLE`      | `011101` |  0  | Mask required                   |
| `VMSGTU`     | `011110` |  0  | Mask required                   |
| `VMSGT`      | `011111` |  0  | Mask required                   |
| `VSADDU`     | `100000` | 0/1 |                                 |
| `VSADD`      | `100001` | 0/1 |                                 |
| `VSSUBU`     | `100010` | 0/1 |                                 |
| `VSSUB`      | `100011` | 0/1 |                                 |
| `VSLL`       | `100101` | 0/1 |                                 |
| `VSMUL`      | `100111` | 0/1 |                                 |
| `VSRL`       | `101000` | 0/1 |                                 |
| `VSRA`       | `101001` | 0/1 |                                 |
| `VSSRL`      | `101010` | 0/1 |                                 |
| `VSSRA`      | `101011` | 0/1 |                                 |
| `VNSRL`      | `101100` | 0/1 |                                 |
| `VNSRA`      | `101101` | 0/1 |                                 |
| `VNCLIPU`    | `101110` | 0/1 |                                 |
| `VNCLIP`     | `101111` | 0/1 |                                 |

### Opivi - vector-immediate operations (funct3 = 011)

| Instruction  |  Funct6  | VM  | Constraints                     |
| :----------- | :------: | :-: | :------------------------------ |
| `VADD`       | `000000` | 0/1 |                                 |
| `VRSUB`      | `000011` | 0/1 |                                 |
| `VAND`       | `001001` | 0/1 |                                 |
| `VOR`        | `001010` | 0/1 |                                 |
| `VXOR`       | `001011` | 0/1 |                                 |
| `VRGATHER`   | `001100` | 0/1 | No overlap (`vd != vs2`)        |
| `VSLIDEUP`   | `001110` | 0/1 | No overlap (`vd != vs2`)        |
| `VSLIDEDOWN` | `001111` | 0/1 | No overlap (`vd != vs2`)        |
| `VADC`       | `010000` |  0  | Mask required (`vd != 0`)       |
| `VMADC`      | `010001` | 0/1 |                                 |
| `VMERGE`     | `010111` |  0  | Mask required                   |
| `VMV`        | `010111` |  1  | Mask not available (`vs2 == 0`) |
| `VMSEQ`      | `011000` |  0  | Mask required                   |
| `VMSNE`      | `011001` |  0  | Mask required                   |
| `VMSLEU`     | `011100` |  0  | Mask required                   |
| `VMSLE`      | `011101` |  0  | Mask required                   |
| `VMSGTU`     | `011110` |  0  | Mask required                   |
| `VMSGT`      | `011111` |  0  | Mask required                   |
| `VSADDU`     | `100000` | 0/1 |                                 |
| `VSADD`      | `100001` | 0/1 |                                 |
| `VSLL`       | `100101` | 0/1 |                                 |
| `VMV1R`      | `100111` |  1  | `imm5 == 0`                     |
| `VMV2R`      | `100111` |  1  | `imm5 == 1`, Align 2            |
| `VMV4R`      | `100111` |  1  | `imm5 == 3`, Align 4            |
| `VMV8R`      | `100111` |  1  | `imm5 == 7`, Align 8            |
| `VSRL`       | `101000` | 0/1 |                                 |
| `VSRA`       | `101001` | 0/1 |                                 |
| `VSSRL`      | `101010` | 0/1 |                                 |
| `VSSRA`      | `101011` | 0/1 |                                 |
| `VNSRL`      | `101100` | 0/1 |                                 |
| `VNSRA`      | `101101` | 0/1 |                                 |
| `VNCLIPU`    | `101110` | 0/1 |                                 |
| `VNCLIP`     | `101111` | 0/1 |                                 |

### Opfvv - floating-point vector-vector operations (funct3 = 001)

| Instruction   |  Funct6  |   vs1   | Constraints |
| :------------ | :------: | :-----: | :---------- |
| `VFWCVTBF16`  | `010010` | `01101` | Widening    |
| `VFNCVTBF16`  | `010010` | `11101` |             |
| `VFWMACCBF16` | `111011` |   vs1   | Widening    |

### Opfvf - floating-point vector-scalar operations (funct3 = 101)

| Instruction   |  Funct6  |   rs1   | Constraints |
| :------------ | :------: | :-----: | :---------- |
| `VFWCVTBF16`  | `010010` | `01101` | Widening    |
| `VFWMACCBF16` | `111011` |   rs1   | Widening    |

## Scheduling and execution constraints

### Zero `vstart` requirement

The following operations (typically OPMVV mode, funct3 = 010) are tracked for compressed instruction validation and require `vstart` to be zero to execute without trapping:

- **Reductions**: `vredsum`, `vredand`, `vredor`, `vredxor`, `vredminu`, `vredmin`, `vredmaxu`, `vredmax`, `vwredsumu`, `vwredsum`.

- **Unary Operations**: `vcpop`, `vfirst`, `vmsbf`, `vmsof`, `vmsif`, `viota`.

- **Compression**: `vcompress`.

_Note: These instructions are validated in `RvvDecode.scala` for internal compressed pipeline tracking._

## Hardware `vstart` memory fault behavior (ADR-043)

The CoralNPU hardware does NOT dynamically update the `vstart` CSR on a mid-instruction memory fault (such as a trap during a vector load/store operation).

It is exclusively software-driven via CSR writes. Software trap handlers and operating system contexts must manually manage the `vstart` state via explicit `vstart_write` operations if they intend to resume vector execution after resolving a memory fault. Relying on the hardware to automatically checkpoint the failing element position into `vstart` will result in incorrect execution state recovery.

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/chisel/src/coralnpu/rvv/RvvDecode.scala`, `hdl/chisel/src/coralnpu/rvv/RvvInterface.scala`, `hdl/chisel/src/coralnpu/scalar/Csr.scala` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
