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

# Validation environment

> **Intended Audience:** HW Devs, SW/Compiler Devs, HW Integrators

The CoralNPU IP block utilizes a multi-tiered verification strategy spanning unit testing, block-level constrained-random simulation, and architectural-level co-simulation. This document provides an onboarding guide to the standalone IP verification environments.

## Cocotb verification environment

The block-level unit verification framework is built on Python-based Cocotb, driving standard AXI4 interfaces on CoralNPU sub-modules. Cocotb tests are orchestrated using Bazel.

### Run cocotb tests

To execute the standard Cocotb test suite, run the following command from the repository root:

```bash

bazel test //tests/cocotb:core_mini_axi_sim_cocotb

```

### Simulation flows

The Cocotb environment supports two distinct simulation flows defined in `rules/coco_tb.bzl` ([Source](../../rules/coco_tb.bzl)):

* **Regular flow (`vcs_cocotb_test`)**: Compiles and runs the simulation in a single step (ideal for standard functional verification).

* **Split flow (`vcs_simulation_split_test`)**: Separates compilation and execution into discrete Bazel targets (necessary for gate-level power and netlist analyses where waveforms must be cached build outputs).

To prevent divergence between these flows, the pre-upload suite triggers a Starlark macro signature signature checker (`utils/check_macro_signatures.py` [[Source](../../utils/check_macro_signatures.py)]) as part of the presubmit checks.

## Uvm testbench environment

For exhaustive, constrained-random verification, a standard SystemVerilog UVM (Universal Verification Methodology) testbench is provided under `tests/uvm/` ([Source](../../tests/uvm/)).

### Testbench structure

The environment instantiates the `RvvCoreMiniVerificationAxi` DUT ([Source](../../hdl/chisel/src/coralnpu/CoreAxi.scala)) and wraps standard interfaces:

* **AXI master agent**: Drives command streams into the CoralNPU memory-mapped register slave.

* **AXI slave agent**: Emulates external system memory response channels.

* **IRQ agent**: Simulates interrupt status pins and handshakes.

### Compiling and running UVM tests

The UVM environment is managed using a local Makefile under `tests/uvm/` ([Source](../../tests/uvm/Makefile)):

```bash

# Navigate to the UVM directory

cd tests/uvm

# Compile the simulator executable (uses synopsys VCS with UVM 1.2)

make compile

# Run the simulation sequence

make run

```

The testbench supports backdoor memory loading to inject test program binaries (`program.elf`) directly into the DUT’s SRAM before starting execution.

## Verilator simulation environment

For high-performance cycle-accurate C++ simulation, a Verilator-based testbench is defined under `tests/verilator_sim/` ([Source](../../tests/verilator_sim/)). This environment is particularly suited for software co-design and architectural execution analysis.

The main simulator entry point is compiled using Bazel:

```bash

bazel build //tests/verilator_sim:core_mini_axi_sim

```

--------------------------------------------------------------------------------

**Provenance & Traceability** - **Verified As Of:** 2026-08-03 - **Upstream Commit:** [1126ed3fa244b38ee06fa002a5c640df9dec36f4](https://github.com/google/coralnpu/commit/1126ed3fa244b38ee06fa002a5c640df9dec36f4) - **Primary Source(s):** `tests/cocotb/README.md`, `tests/uvm/README.md`, `tests/verilator_sim/CPPLINT.cfg` - **Disclaimer:** AI-generated/assisted; RTL is the source of truth.
