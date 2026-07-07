#!/bin/bash
set -e

# Cleanup trap to remove logs/elf
trap 'rm -f ./tmp_log/deadlock_barebones.log ./tmp_log/deadlock_rvvi.log "$PWD/deadlock_dummy.elf"' EXIT

./utils/ensure_writable.sh ./tmp_log

echo "Building mandated binary via Bazel..."
bazel build //examples:coralnpu_v2_deadlock_trigger
cp bazel-bin/examples/coralnpu_v2_deadlock_trigger.elf ./deadlock_dummy.elf

echo "Building Barebones simulator..."
bazel build //tests/verilator_sim:core_barebones_sim

echo "Running Barebones simulator (expecting deadlock exit code 1)..."
set +e
set -o pipefail
bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/deadlock_dummy.elf" --simulate_deadlock --instructions 100 2>&1 | tee "$PWD/tmp_log/deadlock_barebones.log"
EXIT_CODE_BB=$?
set +o pipefail
set -e

if [ $EXIT_CODE_BB -ne 1 ]; then
  echo "Barebones simulator did not exit with deadlock code 1 (Exit Code: $EXIT_CODE_BB)"
  exit 1
fi

echo "Building RVVI Traced simulator..."
bazel build //tests/verilator_sim:core_rvvi_traced_sim

echo "Running RVVI Traced simulator (expecting deadlock exit code 1)..."
set +e
set -o pipefail
bazel run //tests/verilator_sim:core_rvvi_traced_sim -- "$PWD/deadlock_dummy.elf" --simulate_deadlock --instructions 100 2>&1 | tee "$PWD/tmp_log/deadlock_rvvi.log"
EXIT_CODE_RVVI=$?
set +o pipefail
set -e

if [ $EXIT_CODE_RVVI -ne 1 ]; then
  echo "RVVI simulator did not exit with deadlock code 1 (Exit Code: $EXIT_CODE_RVVI)"
  exit 1
fi

echo "Deadlock Verification Test PASSED."
