#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./hang.elf' EXIT

# Generate ELF with wfi (0x10500073)
echo "Generating ELF at 0x00000000 with wfi (0x10500073)..."
bazel run //tests/verilator_sim:gen_elf -- "$PWD/hang.elf" 0x10500073

# Run barebones simulator with huge limit
echo "Running Barebones simulator with huge limit..."
./utils/ensure_writable.sh ./tmp_log
set +e
set -o pipefail
bazel run //tests/verilator_sim:core_barebones_sim -- --instructions=5000 "$PWD/hang.elf" 2>&1 | tee "$PWD/tmp_log/hang_barebones.log"
EXIT_CODE=${PIPESTATUS[0]}
set -e
set +o pipefail

# Check for hang detection (Exit code 124 and HANG message)
if [ "$EXIT_CODE" -eq 124 ] && grep -q "Simulation HANG detected" "$PWD/tmp_log/hang_barebones.log"; then
  echo "E2E Barebones Hang Test PASSED"
  exit 0
else
  echo "Simulation did not HANG as expected (Code: $EXIT_CODE)"
  cat "$PWD/tmp_log/hang_barebones.log"
  exit 1
fi
