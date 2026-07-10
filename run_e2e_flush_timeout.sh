#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./rvvi.elf' EXIT

echo "Generating ELF at 0x00000000 via Bazel..."
bazel run //tests/verilator_sim:gen_elf -- "$PWD/rvvi.elf" --repeat 5 0x00100093 0x08000073

# Run simulator via Bazel, using a named pipe to deterministically block the background daemon to trigger flush timeout (5s)
./utils/ensure_writable.sh ./tmp_log

echo "Running RVVI Flush Timeout simulator via Bazel natively..."

rm -f /tmp/trace.rvvi
mkfifo /tmp/trace.rvvi
exec 3<> /tmp/trace.rvvi
dd if=/dev/zero of=/tmp/trace.rvvi bs=1M count=1 oflag=nonblock 2>/dev/null || true
set +e
set -o pipefail
time bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --memory_profile=default --rvvi_out="/tmp/trace.rvvi" "$PWD/rvvi.elf" 2>&1 | tee /tmp/sim.log
EXIT_CODE=$?
set +o pipefail
set -e

rm -f /tmp/trace.rvvi

if [ "$EXIT_CODE" -eq 124 ]; then
  echo "E2E Flush Timeout Test PASSED (Exit code correctly 124)"
else
  echo "E2E Flush Timeout Test FAILED. Expected exit code 124, got $EXIT_CODE"
  cp /tmp/sim.log ./tmp_log/flush_timeout_sim.log || true
  exit 1
fi
