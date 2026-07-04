#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./dummy_fail.elf ./dummy_fail_tight.elf ./dummy_fail_tight_dtcm.elf' EXIT

echo "Generating out-of-bounds ELF (extreme violation) via Bazel..."
bazel run //tests/verilator_sim:gen_elf -- "$PWD/dummy_fail.elf" --address 0x00800000 0x08000073

mkdir -p ./tmp_log
echo "Running simulator via Bazel natively..."
set -o pipefail
bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/dummy_fail.elf" 2>&1 | tee ./tmp_log/elf_load_fail.log && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
set +o pipefail

if [ $EXIT_CODE -ne 65 ]; then
  echo "Expected exit code 65, got $EXIT_CODE"
  exit 1
fi

grep -q "\[FATAL\] ELF load violation.*Delta: Exceeds bounds by 0x7e8004 bytes\." ./tmp_log/elf_load_fail.log || { echo "Log format mismatch or incorrect delta"; exit 1; }

echo "Generating tight boundary ELF (ITCM upper bound) via Bazel..."
bazel run //tests/verilator_sim:gen_elf -- "$PWD/dummy_fail_tight.elf" --address 0x00002000 0x08000073

echo "Running simulator via Bazel natively (tight boundary)..."
set -o pipefail
bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/dummy_fail_tight.elf" 2>&1 | tee ./tmp_log/elf_load_fail_tight.log && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
set +o pipefail

if [ $EXIT_CODE -ne 65 ]; then
  echo "Expected exit code 65, got $EXIT_CODE"
  exit 1
fi

grep -q "\[FATAL\] ELF load violation.*Delta: Exceeds bounds by 0x4 bytes\." ./tmp_log/elf_load_fail_tight.log || { echo "Log format mismatch or incorrect delta"; exit 1; }

echo "Generating tight boundary ELF (DTCM upper bound) via Bazel..."
bazel run //tests/verilator_sim:gen_elf -- "$PWD/dummy_fail_tight_dtcm.elf" --address 0x00018000 0x08000073

echo "Running simulator via Bazel natively (DTCM tight boundary)..."
set -o pipefail
bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/dummy_fail_tight_dtcm.elf" 2>&1 | tee ./tmp_log/elf_load_fail_tight_dtcm.log && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
set +o pipefail

if [ $EXIT_CODE -ne 65 ]; then
  echo "Expected exit code 65, got $EXIT_CODE"
  exit 1
fi

grep -q "\[FATAL\] ELF load violation.*Delta: Exceeds bounds by 0x4 bytes\." ./tmp_log/elf_load_fail_tight_dtcm.log || { echo "Log format mismatch or incorrect delta"; exit 1; }

echo "E2E ELF Load Fail Test PASSED"