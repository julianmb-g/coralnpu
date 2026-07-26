# CoralNPU Hardware Initialization

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your own risk.

> **Intended Audience:** HW Integrators and Folks writing software.

This document describes the reset and initialization sequences for the CoralNPU IP.

## Reset Domain
The CoralNPU utilizes an asynchronous reset synchronization mechanism to ensure stable power-on states across all modules.

| Reset Signal | Description | Synchronization |
| :--- | :--- | :--- |
| `rst_n` | Main asynchronous reset input | `RstSync` (Asynchronous) |

[Source: `hdl/verilog/RstSync.sv`]

## Initialization Sequence

The NPU initialization follows a host-driven multi-step sequence:

1. Assert `rst_n` asynchronously.
2. Configure initial CSR states via AXI slave interface.
3. De-assert `rst_n` to begin internal reset synchronization.
4. Poll status registers (e.g., `mstatus`) for ready state.

[Source: `hdl/chisel/src/coralnpu/CoreAxiCSR.scala`]

## Default CSR State
Key status registers upon reset and initial boot sequence.

| Register | Default Value | Description |
| :--- | :--- | :--- |
| `mstatus` | `0x0` | RISC-V Machine Status |
| `misa` | `0x40001104` | ISA feature identification |
| `mie` | `0x0` | Machine Interrupt Enable |

[Source: `hdl/chisel/src/coralnpu/CoreAxiCSR.scala`]

---

**Provenance & Traceability**
- **Verified As Of:** 2026-07-26
- **Upstream Commit:** [fcb74cfe79dbd184b9c53539490994e701981f80](https://github.com/google/coralnpu/commit/fcb74cfe79dbd184b9c53539490994e701981f80)
- **Primary Source(s):** `hdl/verilog/RstSync.sv`, `hdl/chisel/src/coralnpu/CoreAxiCSR.scala`
- **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
