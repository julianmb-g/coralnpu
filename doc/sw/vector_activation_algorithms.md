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
# Vector activation algorithms

> ⚠️ **Disclaimer:** This document was generated or modified by an AI model. While every effort is made to ensure technical accuracy, the underlying source code and hardware RTL implementation remain the absolute source of truth. Use at your risk.

> **Intended Audience:** SW/Compiler Devs

Complex non-linear functions (e.g., Sigmoid, Tanh) are implemented using software-assisted techniques leveraging the CoralNPU vector pipeline.

## Transcendental activations

Complex non-linear functions are approximated via polynomial expansions or reciprocal estimates in floating-point units.

### Approximation Techniques
1. **Polynomial Approximation**: The VFPU's high-throughput FMA units allow for fast execution of Taylor series or minimax polynomial approximations.
2. **Lookup Tables (LUTs)**: For higher performance, software can pre-calculate activation values into a table stored in [TCM](../microarch/memory/tcm.md). The `VRGATHER` instruction can then be used to perform high-speed vector lookups.
3. **Reciprocal Estimates**: The VFPU provides 7-bit reciprocal estimates (`VFRECE7.V`), which can be used to accelerate Sigmoid calculations.

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `hdl/verilog/rvv/design/rvv_backend_sqrt7_rec7.sv`, `hdl/verilog/rvv/design/rvv_backend_mac_unit.sv` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
