#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./rvvi.elf' EXIT

# Generate a valid ELF that loads at 0x00000000
echo "Generating ELF at 0x00000000 via Bazel..."
bazel run //tests/verilator_sim:gen_elf -- "$PWD/rvvi.elf" 0x00000013 0x08000073

# Run simulator via Bazel
echo "Running RVVI simulator via Bazel natively..."
./utils/ensure_writable.sh ./tmp_log
set -o pipefail
bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --rvvi_out="$PWD/trace.rvvi" "$PWD/rvvi.elf" 2>&1 | tee /tmp/sim.log || (cp /tmp/sim.log ./tmp_log/termination_sim.log; find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null; exit 1)
set +o pipefail

echo "Checking RVVI trace output for graceful termination..."
if ! grep -q "08000073" trace.rvvi; then
  echo "Trace file missing expected mpause instruction (08000073)"
  exit 1
fi

echo "E2E Graceful Termination Test PASSED"