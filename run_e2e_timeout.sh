#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./timeout.elf ./barebones_out.log ./rvvi_out.log' EXIT

echo "Generating ELF at 0x00000000 with infinite loop (0x0000006f) via Bazel..."
bazel run //tests/verilator_sim:gen_elf -- "$PWD/timeout.elf" 0x0000006f

# Run Barebones simulator and expect timeout
echo "Running Barebones simulator via Bazel (expecting timeout)..."
./utils/ensure_writable.sh ./tmp_log
set +e
set -o pipefail
bazel run //tests/verilator_sim:core_barebones_sim -- --instructions=5000 "$PWD/timeout.elf" 2>&1 | tee "$PWD/tmp_log/timeout_barebones_sim.log"
EXIT_CODE=$?
set +o pipefail
find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null
set -e

if [ $EXIT_CODE -eq 65 ]; then
  echo "Simulator incorrectly exited with 65 (ELF load failure code) instead of 124 on timeout/deadlock!"
  exit 1
fi
if [ $EXIT_CODE -ne 124 ]; then
  echo "Barebones simulator did not exit with code 124 (Exit Code: $EXIT_CODE)"
  exit 1
fi

if ! grep -q "Simulation TIMEOUT" ./tmp_log/timeout_barebones_sim.log; then
  echo "Barebones simulator output missing 'Simulation TIMEOUT'"
  exit 1
fi

echo "Barebones simulator timed out as expected."

# Run RVVI simulator and expect timeout
echo "Running RVVI simulator via Bazel (expecting timeout)..."
set +e
set -o pipefail
bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --instructions=5000 --rvvi_out="$PWD/trace.rvvi" "$PWD/timeout.elf" 2>&1 | tee "$PWD/tmp_log/timeout_rvvi_sim.log"
EXIT_CODE=$?
set +o pipefail
find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null
set -e

if [ $EXIT_CODE -eq 65 ]; then
  echo "Simulator incorrectly exited with 65 (ELF load failure code) instead of 124 on timeout/deadlock!"
  exit 1
fi
if [ $EXIT_CODE -ne 124 ]; then
  echo "RVVI simulator did not exit with code 124 (Exit Code: $EXIT_CODE)"
  exit 1
fi

if ! grep -q "Simulation TIMEOUT" ./tmp_log/timeout_rvvi_sim.log; then
  echo "RVVI simulator output missing 'Simulation TIMEOUT'"
  exit 1
fi

echo "RVVI simulator timed out as expected."

echo "E2E Timeout Verification Test PASSED"