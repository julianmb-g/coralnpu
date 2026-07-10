#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./ebreak.elf' EXIT

# Generate a valid ELF that loads at 0x00000000 with ebreak...
echo "Generating ELF at 0x00000000 with ebreak via Bazel..."
bazel run //tests/verilator_sim:gen_elf -- "$PWD/ebreak.elf" 0x00100073

# Run simulator via Bazel
echo "Running RVVI simulator via Bazel..."
./utils/ensure_writable.sh ./tmp_log
set +e
set -o pipefail
bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --rvvi_out="$PWD/trace.rvvi" "$PWD/ebreak.elf" 2>&1 | tee "$PWD/tmp_log/ebreak_sim.log"
EXIT_CODE=$?
find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null
set -e
set +o pipefail

echo "Checking RVVI trace output..."
if ! grep -q "00100073" trace.rvvi; then
  echo "Trace file missing expected instruction (00100073)"
  exit 1
fi

if [ "$EXIT_CODE" -ne 0 ]; then
  echo "Simulator failed with exit code $EXIT_CODE"
  exit 1
fi

echo "E2E Ebreak Termination Test PASSED"