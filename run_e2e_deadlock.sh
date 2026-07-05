#!/bin/bash
set -e

# Cleanup trap
trap 'git restore tests/verilator_sim/coralnpu/core_barebones_tb.cc tests/verilator_sim/coralnpu/core_rvvi_traced_sim.cc 2>/dev/null || true; rm -f ./deadlock.elf ./tmp_log/deadlock_barebones.log ./tmp_log/deadlock_rvvi.log' EXIT

./utils/ensure_writable.sh ./tmp_log

echo "Generating E2E ELF..."
bazel run //tests/verilator_sim:gen_elf -- "$PWD/deadlock.elf" 0x00000013

echo "Injecting artificial delta loop to test monitor_delta..."
sed -i 's/if (sc_pending_activity_at_current_time()) {/if (true) { \/\/ INJECTED DEADLOCK/g' tests/verilator_sim/coralnpu/core_barebones_tb.cc
sed -i 's/if (sc_pending_activity_at_current_time()) {/if (true) { \/\/ INJECTED DEADLOCK/g' tests/verilator_sim/coralnpu/core_rvvi_traced_sim.cc

echo "Building Barebones simulator..."
bazel build //tests/verilator_sim:core_barebones_sim

echo "Running Barebones simulator (expecting deadlock exit code 1)..."
set +e
set -o pipefail
bazel run //tests/verilator_sim:core_barebones_sim -- --instructions=1000 "$PWD/deadlock.elf" 2>&1 | tee "$PWD/tmp_log/deadlock_barebones.log"
EXIT_CODE_BB=$?
set +o pipefail
set -e

if [ $EXIT_CODE_BB -eq 124 ]; then
  echo "Barebones simulator timed out (Exit Code 124) instead of deadlocking (Exit Code 1)!"
fi

if [ $EXIT_CODE_BB -ne 1 ]; then
  echo "Barebones simulator did not exit with code 1 (Exit Code: $EXIT_CODE_BB)"
  exit 1
fi

echo "Building RVVI Traced simulator..."
bazel build //tests/verilator_sim:core_rvvi_traced_sim

echo "Running RVVI Traced simulator (expecting deadlock exit code 1)..."
set +e
set -o pipefail
bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --instructions=1000 "$PWD/deadlock.elf" 2>&1 | tee "$PWD/tmp_log/deadlock_rvvi.log"
EXIT_CODE_RVVI=$?
set +o pipefail
set -e

if [ $EXIT_CODE_RVVI -eq 124 ]; then
  echo "RVVI simulator timed out (Exit Code 124) instead of deadlocking (Exit Code 1)!"
fi

if [ $EXIT_CODE_RVVI -ne 1 ]; then
  echo "RVVI simulator did not exit with code 1 (Exit Code: $EXIT_CODE_RVVI)"
  exit 1
fi

echo "Deadlock Verification Test PASSED."
