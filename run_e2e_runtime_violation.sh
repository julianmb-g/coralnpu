#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./runtime_fail_read.elf ./runtime_fail_write.elf ./runtime_fail_fetch.elf' EXIT

if [ -z "${USE_PODMAN}" ]; then
  USE_PODMAN=0
  if ! command -v bazel &> /dev/null && command -v podman &> /dev/null; then
    echo "Bazel not found natively. Falling back to Podman..."
    USE_PODMAN=1
  fi
fi

mkdir -p ./tmp_log

# Scenario 1: Data Read Violation
echo "Generating ELF with invalid data read via Bazel..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/runtime_fail_read.elf --address 0x00000000 0x008000B7 0x00008203 0x08000073"
else
  bazel run //tests/verilator_sim:gen_elf -- "$PWD/runtime_fail_read.elf" --address 0x00000000 0x008000B7 0x00008203 0x08000073
fi

echo "Running simulator for Data Read Violation..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_barebones_sim -- \$PWD/runtime_fail_read.elf 2>&1 | tee ./tmp_log/runtime_fail_read.log" && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
else
  set -o pipefail
  bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/runtime_fail_read.elf" 2>&1 | tee ./tmp_log/runtime_fail_read.log && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
fi

if [ $EXIT_CODE -ne 65 ]; then
  echo "Expected exit code 65, got $EXIT_CODE"
  exit 1
fi

grep -q "\[FATAL\] Runtime memory violation" ./tmp_log/runtime_fail_read.log || { echo "Log format mismatch or missing runtime violation log"; exit 1; }
echo "Data Read Violation Test PASSED."

# Scenario 2: Data Write Violation
echo "Generating ELF with invalid data write via Bazel..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/runtime_fail_write.elf --address 0x00000000 0x008000B7 0x0000A023 0x08000073"
else
  bazel run //tests/verilator_sim:gen_elf -- "$PWD/runtime_fail_write.elf" --address 0x00000000 0x008000B7 0x0000A023 0x08000073
fi

echo "Running simulator for Data Write Violation..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_barebones_sim -- \$PWD/runtime_fail_write.elf 2>&1 | tee ./tmp_log/runtime_fail_write.log" && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
else
  set -o pipefail
  bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/runtime_fail_write.elf" 2>&1 | tee ./tmp_log/runtime_fail_write.log && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
fi

if [ $EXIT_CODE -ne 65 ]; then
  echo "Expected exit code 65, got $EXIT_CODE"
  exit 1
fi

grep -q "\[FATAL\] Runtime memory violation" ./tmp_log/runtime_fail_write.log || { echo "Log format mismatch or missing runtime violation log"; exit 1; }
echo "Data Write Violation Test PASSED."

# Scenario 3: Instruction Fetch Violation
echo "Generating ELF with invalid instruction fetch via Bazel..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/runtime_fail_fetch.elf --address 0x00000000 0x008000B7 0x00008067"
else
  bazel run //tests/verilator_sim:gen_elf -- "$PWD/runtime_fail_fetch.elf" --address 0x00000000 0x008000B7 0x00008067
fi

echo "Running simulator for Instruction Fetch Violation..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_barebones_sim -- \$PWD/runtime_fail_fetch.elf 2>&1 | tee ./tmp_log/runtime_fail_fetch.log" && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
else
  set -o pipefail
  bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/runtime_fail_fetch.elf" 2>&1 | tee ./tee ./tmp_log/runtime_fail_fetch.log" && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
fi

if [ $EXIT_CODE -ne 65 ]; then
  echo "Expected exit code 65, got $EXIT_CODE"
  exit 1
fi

grep -q "\[FATAL\] Runtime memory violation" ./tmp_log/runtime_fail_fetch.log || { echo "Log format mismatch or missing runtime violation log"; exit 1; }
echo "Instruction Fetch Violation Test PASSED."

echo "All Runtime Violation Tests PASSED."
