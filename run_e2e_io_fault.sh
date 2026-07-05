#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./io_fault.elf ./tmp_log/io_fault_barebones.log ./tmp_log/io_fault_rvvi.log; git checkout -- tests/verilator_sim/coralnpu/core_barebones_tb.cc tests/verilator_sim/coralnpu/core_rvvi_traced_sim.cc' EXIT

mkdir -p ./tmp_log

echo "Generating dummy ELF..."
bazel run //tests/verilator_sim:gen_elf -- "$PWD/io_fault.elf" 0x00000013

echo "Injecting io_fault into testbench for E2E validation..."
# We inject a fake io_fault after 100 cycles to verify the exit code logic
sed -i 's/if (io_fault)/if (cycle() > 100) { fprintf(stderr, "io_fault asserted\\n"); had_io_fault = true; sc_stop(); } if (io_fault)/' tests/verilator_sim/coralnpu/core_barebones_tb.cc
sed -i 's/if (io_fault)/if (cycle() > 100) { fprintf(stderr, "io_fault asserted\\n"); had_io_fault = true; sc_stop(); } if (io_fault)/' tests/verilator_sim/coralnpu/core_rvvi_traced_sim.cc

echo "Running Barebones simulator (expecting io_fault exit code 1)..."
set +e
set -o pipefail
bazel run //tests/verilator_sim:core_barebones_sim -- --instructions=1000 "$PWD/io_fault.elf" 2>&1 | tee "$PWD/tmp_log/io_fault_barebones.log"
EXIT_CODE_BB=$?
set +o pipefail
set -e

if [ $EXIT_CODE_BB -eq 124 ]; then
  echo "Barebones simulator timed out (Exit Code 124) instead of io_fault (Exit Code 1)!"
fi

if [ $EXIT_CODE_BB -ne 1 ]; then
  echo "Barebones simulator did not exit with code 1 (Exit Code: $EXIT_CODE_BB)"
  exit 1
fi

if ! grep -q "io_fault asserted" "$PWD/tmp_log/io_fault_barebones.log"; then
  echo "Barebones log missing 'io_fault asserted'"
  exit 1
fi

echo "Running RVVI Traced simulator (expecting io_fault exit code 1)..."
set +e
set -o pipefail
bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --instructions=1000 "$PWD/io_fault.elf" 2>&1 | tee "$PWD/tmp_log/io_fault_rvvi.log"
EXIT_CODE_RVVI=$?
set +o pipefail
set -e

if [ $EXIT_CODE_RVVI -eq 124 ]; then
  echo "RVVI simulator timed out (Exit Code 124) instead of io_fault (Exit Code 1)!"
fi

if [ $EXIT_CODE_RVVI -ne 1 ]; then
  echo "RVVI simulator did not exit with code 1 (Exit Code: $EXIT_CODE_RVVI)"
  exit 1
fi

if ! grep -q "io_fault asserted" "$PWD/tmp_log/io_fault_rvvi.log"; then
  echo "RVVI log missing 'io_fault asserted'"
  exit 1
fi

echo "IO Fault Verification Test PASSED."
