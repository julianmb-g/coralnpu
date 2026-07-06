#!/bin/bash
set -e

# Cleanup trap to restore files and remove logs/elf
trap 'git restore tests/verilator_sim/coralnpu/core_barebones_tb.cc tests/verilator_sim/coralnpu/core_rvvi_traced_sim.cc; rm -f ./tmp_log/deadlock_barebones.log ./tmp_log/deadlock_rvvi.log "$PWD/deadlock_dummy.elf"' EXIT

./utils/ensure_writable.sh ./tmp_log

echo "Injecting artificial deadlock condition..."
sed -i 's/current_delta - last_delta > 10000/current_delta - last_delta > 10000 || instruction_count > 5/g' tests/verilator_sim/coralnpu/core_barebones_tb.cc
sed -i 's/current_delta - last_delta > 10000/current_delta - last_delta > 10000 || instruction_count > 5/g' tests/verilator_sim/coralnpu/core_rvvi_traced_sim.cc

echo "Building mandated binary via Bazel..."
bazel build //examples:coralnpu_v2_rvv_add_intrinsic
cp bazel-bin/examples/coralnpu_v2_rvv_add_intrinsic.elf ./deadlock_dummy.elf

echo "Building Barebones simulator..."
bazel build //tests/verilator_sim:core_barebones_sim

echo "Running Barebones simulator (expecting deadlock exit code 1)..."
set +e
set -o pipefail
bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/deadlock_dummy.elf" 2>&1 | tee "$PWD/tmp_log/deadlock_barebones.log"
EXIT_CODE_BB=$?
set +o pipefail
set -e

if [ $EXIT_CODE_BB -ne 1 ]; then
  echo "Barebones simulator did not exit with code 1 (Exit Code: $EXIT_CODE_BB)"
  exit 1
fi

if ! grep -q "Delta cycle deadlock detected" "$PWD/tmp_log/deadlock_barebones.log"; then
  echo "Barebones log missing 'Delta cycle deadlock detected'"
  exit 1
fi

echo "Building RVVI Traced simulator..."
bazel build //tests/verilator_sim:core_rvvi_traced_sim

echo "Running RVVI Traced simulator (expecting deadlock exit code 1)..."
set +e
set -o pipefail
bazel run //tests/verilator_sim:core_rvvi_traced_sim -- "$PWD/deadlock_dummy.elf" 2>&1 | tee "$PWD/tmp_log/deadlock_rvvi.log"
EXIT_CODE_RVVI=$?
set +o pipefail
set -e

if [ $EXIT_CODE_RVVI -ne 1 ]; then
  echo "RVVI simulator did not exit with code 1 (Exit Code: $EXIT_CODE_RVVI)"
  exit 1
fi

if ! grep -q "Delta cycle deadlock detected" "$PWD/tmp_log/deadlock_rvvi.log"; then
  echo "RVVI log missing 'Delta cycle deadlock detected'"
  exit 1
fi

echo "Deadlock Verification Test PASSED."
