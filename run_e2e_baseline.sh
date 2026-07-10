#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./dummy.elf' EXIT

# Build mandated binary
echo "Building mandated binary via Bazel..."
bazel build //examples:coralnpu_v2_rvv_add_intrinsic
cp bazel-bin/examples/coralnpu_v2_rvv_add_intrinsic.elf ./dummy.elf

# Run simulator via Bazel
./utils/ensure_writable.sh ./tmp_log
echo "Running simulator via Bazel natively..."
set -o pipefail
bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/dummy.elf" 2>&1 | tee /tmp/sim.log || (cp /tmp/sim.log ./tmp_log/baseline_sim.log; find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null; exit 1)
set +o pipefail

echo "E2E Baseline Emulation Test PASSED"
