#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./real.elf' EXIT

# Generate a valid ELF that loads at 0x00000000
echo "Generating ELF at 0x00000000 via Bazel..."
bazel run //tests/verilator_sim:gen_elf -- "$PWD/real.elf" 0x08000073

# Run the simulator
echo "Running simulator with real ELF via Bazel..."
./utils/ensure_writable.sh ./tmp_log
set -o pipefail
bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/real.elf" 2>&1 | tee "$PWD/tmp_log/real_address_sim.log" || exit 1
set +o pipefail

echo "E2E Real Address Loading Test PASSED"