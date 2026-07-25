#!/bin/bash
set -e

[[ -f /.dockerenv ]] || exit 1

# Cleanup trap
trap 'rm -f ./ebreak.elf' EXIT

# Generate ELF with ebreak (0x00100073)
echo "Generating ELF at 0x00000000 with ebreak..."
bazel run //tests/verilator_sim:gen_elf -- "$PWD/ebreak.elf" 0x00100073

# Run barebones simulator
echo "Running Barebones simulator..."
./utils/ensure_writable.sh ./tmp_log
set +e
set -o pipefail
bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/ebreak.elf" 2>&1 | tee "$PWD/tmp_log/ebreak_barebones.log"
EXIT_CODE=${PIPESTATUS[0]}
set -e
set +o pipefail

if [ "$EXIT_CODE" -ne 0 ]; then
  echo "Simulator failed with exit code $EXIT_CODE"
  exit 1
fi

echo "E2E Barebones Ebreak Termination Test PASSED"
