# Vector Backend Reciprocal & Square Root Estimates

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

> **Intended Audience:** Hardware Developers

The `rvv_backend_sqrt7_rec7` module provides hardware support for the RISC-V Vector extension's 7-bit reciprocal (`vfrece7.v`) and square-root (`vfsqrt7.v`) estimate instructions. It is designed to compute low-precision approximations at high throughput, bypassing the more complex and iterative standard floating-point division/sqrt units to accelerate transcendental functions like Sigmoid and Tanh.

## Architectural Overview

The unit implements a fully pipelined 4-stage execution architecture to maximize throughput while computing 7-bit estimates. It shares a single datapath for both reciprocal and square-root estimates, distinguishing the operation using the least significant bit of the source register index (`vs1_i[0]`).

- `vs1_i[0] == 1`: Executes 7-bit Reciprocal Estimate (`vfrece7.v`).
- `vs1_i[0] == 0`: Executes 7-bit Square Root Estimate (`vfsqrt7.v`).

### 4-Stage Execution Pipeline

The estimator employs a fully-pipelined, 4-stage data path without internal backpressure stalling (stall is only asserted if `out_ready_i` is low at the final stage).

1. **Stage 1 (Input & Subnormal Detection):** Registers input signals (`operand_i`, `rnd_mode_i`, `tag_i`) and determines if the input operand is a subnormal number by checking for a zero exponent and non-zero mantissa (`pip1_subn`).
2. **Stage 2 (Normalization & Table Lookup):** Computes the leading zero count of the mantissa (`leading_zero_cnt`) to normalize the exponent (`pip1_exp_normed`). It combinationally performs the ROM table lookups for both the square-root and reciprocal mantissa estimates (`pip2_sqrt7_man` and `pip2_rec7_man`). Specifically:
   - The SQRT7 lookup table leverages `pip2_exp_normed[0]` and the upper 6 bits of the normalized mantissa index to fetch a 7-bit estimate.
   - The REC7 lookup table uses the full 7-bit `pip2_man_index` to fetch a 7-bit estimate.
   - Stage 2 also dynamically evaluates special cases like `NaN`, `qNaN`, `zero`, and `infinity` based on standard IEEE 754 representations.
3. **Stage 3 (Exponent Calculation & Assembly):** Calculates the final normalized exponents (`pip2_sqrt7_exp_bf_sft`, `pip2_sqrt7_exp`, `pip2_rec7_exp`). Aligns the estimated 7-bit mantissas into their proper 23-bit positions (`{pip3_sqrt7_man, 16'h0}`). For reciprocal subnormal cases, bounds checking against exponent underflow/overflow is performed, triggering potential clamping behavior based on the dynamic/architectural rounding mode (`rnd_mode_i`).
4. **Stage 4 (Final Selection & Writeback):** Selects the correct final result (`sqrt7_result` or `rec7_result`) and floating-point exception flags (`pip4_sqrt7_fexp` or `pip4_rec7_fexp`) based on the active operation type (`in_valid_i` vs `rec7_vld`). Propagates the result along with the tracking `tag_o`.

## Hardware Interfaces

The module utilizes a standardized decoupled handshake (`valid`/`ready`) with flush capability:

| Port                          | Direction | Width                 | Description                                                                 |
| :---------------------------- | :-------- | :-------------------- | :-------------------------------------------------------------------------- |
| `operand_i`                   | Input     | `WORD_WIDTH`          | 32-bit single-precision floating-point operand.                             |
| `vs1_i`                       | Input     | `REGFILE_INDEX_WIDTH` | Source 1 register index. `vs1_i[0]` selects between REC7 and SQRT7.         |
| `rnd_mode_i`                  | Input     | `RVFRM`               | Dynamic rounding mode (e.g., `FRUP`, `FRDN`, `FRTZ`).                       |
| `tag_i` / `tag_o`             | In/Out    | `TagType`             | Metadata tag used to track the micro-operation through the pipeline.        |
| `in_valid_i` / `in_ready_o`   | In/Out    | 1                     | Standard decoupled handshake for receiving an operation.                    |
| `flush_i`                     | Input     | 1                     | Asynchronous flush signal to clear the pipeline on traps/exceptions.        |
| `result_o`                    | Output    | `WORD_WIDTH`          | Computed 32-bit floating-point estimate.                                    |
| `tbl_status_o`                | Output    | `RVFEXP_t`            | Accompanying floating-point exception flags (e.g., `nv`, `dz`, `of`, `nx`). |
| `out_valid_o` / `out_ready_i` | In/Out    | 1                     | Standard decoupled handshake for delivering the result to the ROB.          |

## Exception Handling & Edge Cases

The unit natively detects and handles several critical floating-point edge cases without stalling the pipeline:

- **Divide by Zero (`dz`):** If the input is exactly zero, both operations return infinity with the appropriate sign and assert the `dz` flag.
- **Invalid Operation (`nv`):** Square root of a negative number or operations on `NaN` return the canonical NaN (`sign=0`, `exp=8'hFF`, `man=23'h400000`) and assert the `nv` flag.
- **Subnormal Inputs/Outputs & Subnormal Overflow:** The module includes a custom `leading_zero_cnt` tree and `man7_normed` function to handle the normalization of subnormal inputs. Additionally, when subnormal outputs occur (especially in `rec7` rounding to zero/down), it can correctly generate max subnormal values (`exp=0xFE`, `man=0x7FFFFF`) and assert overflow (`of`) and inexact (`nx`) flags, dynamically returning either infinity or `MAX_FLOAT` based on the specified dynamic rounding mode (`FRUP`, `FRDN`, `FRTZ`).



---

> **Traceability:** Generated by Gemini. Derived from upstream commit 7d6e35cff17732cad8f99b866333423ea1f4d750.
