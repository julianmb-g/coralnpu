# CoralNPU Verilator Simulations

This directory contains Verilator-based simulation targets for the CoralNPU core, including barebones and RVVI-traced configurations.

## Native Usage (End-Users)

End-users (e.g., DV engineers, external contributors) are expected to compile and run the project natively on their host machines using standard Bazel commands, without the Podman container.

**Build:**
```bash
bazel build //tests/verilator_sim:core_barebones_sim
bazel build //tests/verilator_sim:core_rvvi_traced_sim
```

**Run:**
```bash
./bazel-bin/tests/verilator_sim/core_barebones_sim --memory_profile=default <path_to_elf>
```

## Barebones Core Target (`core_barebones_sim`)

The `core_barebones_sim` target provides a high-speed simulation environment with:
-   **Direct Memory Interface:** Connects the verilated core to an idealized, zero-latency C++ memory interface (`BareCoreInterface`), bypassing AXI.
-   **ELF Loading:** Supports loading CoralNPU ELF binaries via backdoor writes to the memory interface.
-   **Termination:** Exits on `mpause` (0x08000073), `ebreak` (0x00100073), or instruction/cycle timeouts.

**Usage:**

```bash
# Build the barebones simulator
bazel build //tests/verilator_sim:core_barebones_sim

# Run a baseline ELF (example: rvv_add_intrinsic)
./bazel-bin/tests/verilator_sim/core_barebones_sim --memory_profile=default <path_to_elf>
```

## RVVI Traced Core Target (`core_rvvi_traced_sim`)

The `core_rvvi_traced_sim` target extends the barebones simulator with asynchronous RVVI-TEXT tracing:
-   **Thread-Safe Tracing:** Uses a two-stage pipeline with an `SpscRingBuffer` and `TraceDaemon` to capture traces without stalling the simulation.
-   **Symbolication & Disassembly:** The background `TraceDaemon` thread performs asynchronous disassembly.
-   **RVVI-TEXT Output:** Generates comma-separated RVVI traces (`rvvi,0,PC,INST,DISASM...`).
-   **Superscalar Retirement:** Supports 8-issue superscalar retirement, scanning retirement ports `inst_0` to `inst_7`.

**Usage:**

```bash
# Build the RVVI traced simulator
bazel build //tests/verilator_sim:core_rvvi_traced_sim

# Run an RVVI trace test
./bazel-bin/tests/verilator_sim/core_rvvi_traced_sim --memory_profile=default <path_to_elf>
```

## Command-Line Flags

-   `--memory_profile={default|highmem}`: **Mandatory.** Specifies the memory map profile.
    -   `default`: ITCM (0x00000000 - 8KB, Read/Execute), DTCM (0x00010000 - 32KB, Read/Write).
    -   `highmem`: Unified ITCM/DTCM layout (0x00100000 - 1MB, Read/Write/Execute).
    -   **Error (EX_DATAERR 65):** Attempting to load ELF segments outside the configured profile bounds results in a fatal error.

## RVVI-TEXT Output

The RVVI simulators generate trace files in RVVI-TEXT v0.4 format. Golden traces are stored in `tests/verilator_sim/testdata/goldens/` and used for comparison in E2E tests.

## Exit Codes

-   `0`: Success (Core reached `mpause`).
-   `1`: Timeout (Instruction, cycle, or queue flush timeout) or `io_fault` encountered.
-   `2`: `ebreak` encountered.
-   `65` (EX_DATAERR): ELF memory load violation (out-of-bounds).
-   `70` (EX_SOFTWARE): SystemC Delta Cycle Deadlock detected.

## E2E Test Scripts

The `run_e2e_*.sh` scripts in the project root are used for end-to-end testing:
-   `run_e2e_baseline.sh`: Tests the `core_barebones_sim` with a baseline ELF.
-   `run_e2e_rvvi.sh`: Tests `core_rvvi_traced_sim` with RVVI tracing.
-   `run_e2e_rvvi_float.sh`: Tests float operations with RVVI tracing.
-   `run_e2e_backpressure.sh`: Tests RVVI backpressure resilience.
-   `run_e2e_ebreak_barebones.sh`: Tests `ebreak` termination.
-   `run_e2e_memory_violation.sh`: Tests memory bounds violations.
-   `run_e2e_deadlock.sh`: Tests SystemC delta cycle deadlock detection.
