# Vector Divider Unit (VDIV)

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

> **Intended Audience:** Hardware Developers

The Vector Divider Unit (VDIV) is responsible for executing integer vector division and remainder instructions. It is instantiated within the Vector Backend and interfaces directly with the Vector Reorder Buffer (ROB).

## Architectural Overview

The top-level `rvv_backend_div` wrapper instantiates `NUM_DIV` parallel divider execution units. If floating-point support (`ZVE32F_ON`) is enabled, it also instantiates parallel floating-point divider units and multiplexes their results to the ROB using a round-robin arbiter (`arb_round_robin`).

The core integer division logic is handled by `rvv_backend_div_unit`, which supports the following operations:

- **VDIV, VDIVU**: Signed and unsigned vector division.
- **VREM, VREMU**: Signed and unsigned vector remainder.

These operations are supported for both vector-vector (`OPMVV`) and vector-scalar (`OPMVX`) instruction formats.

## Data Path and Element Sizing

To optimize hardware resources, the Vector Divider dynamically routes elements to appropriately sized hardware dividers based on the Effective Element Width (EEW).

The hardware provides a heterogeneous mix of divider widths:

- `VLENB/2` instances of 8-bit dividers
- `VLENH/2` instances of 16-bit dividers
- `VLENW` instances of 32-bit dividers

During execution, operands are signed-extended or zero-extended depending on the opcode (`DIV_SIGN` or `DIV_ZERO`) and routed to the corresponding hardware dividers. The `div_uop_ready` signal is asserted only when all active constituent dividers assert their `div_ready` flags.

## Arbitration and ROB Interface

When floating-point is enabled, the integer and floating-point divider units share the writeback port to the ROB. Arbitration is performed via a 2-request round-robin arbiter (`arb2rob`):

- `req[0]`: Integer result valid
- `req[1]`: Floating-point result valid

The arbiter grants access to the ROB writeback port (`result_valid`, `result_data`), ensuring starvation-free multiplexing of variable-latency division results.

## Floating-Point Division Pipeline (Zve32f)

The Vector Floating-Point Division Unit handles floating-point division and square root operations.

- **Supported Instructions**:
  - `VFDIV` (Vector float divide)
  - `VFRDIV` (Vector float reverse divide)
  - `VFSQRT` (Vector float square root, encoded as `VFUNARY1`)
- **Underlying IP**: It utilizes the `fpnew_divsqrt_th_64_multi` core from the PULP `fpnew` library, configured for FP32 format (`FpFmtConfig = 5'b10000`).
- **Rounding Modes**: Supports dynamic rounding modes extracted from the `frm` field of the instruction (e.g., RNE, RTZ, RDN, RUP, RMM).
- **Exception Handling**: Accumulates floating-point exception flags (`sub_fpexp`) and packs them into the ROB result struct (`result.fpexp`).

<!-- mdformat off -->
<!-- prettier-ignore -->
--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-07-03 - **Upstream Commit:** f5f6c88d3dff8cb198cd89420919b6863667f3e0 - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_div.sv`, `hdl/verilog/rvv/design/rvv_backend_div_unit.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.

<!-- mdformat on -->
