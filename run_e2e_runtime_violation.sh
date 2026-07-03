#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./runtime_fail_read.elf ./runtime_fail_write.elf ./runtime_fail_fetch.elf ./runtime_fail_read_rvvi.elf ./trace_runtime_fail.rvvi' EXIT

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
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/runtime_fail_read.elf --address 0x00000000 0x000050B7 0x00008203 0x08000073"
else
  bazel run //tests/verilator_sim:gen_elf -- "$PWD/runtime_fail_read.elf" --address 0x00000000 0x000050B7 0x00008203 0x08000073
fi

echo "Running simulator for Data Read Violation..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_barebones_sim -- \$PWD/runtime_fail_read.elf 2>&1 | tee ./tmp_log/runtime_fail_read.log" && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
else
  set -o pipefail
  bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/runtime_fail_read.elf" 2>&1 | tee ./tmp_log/runtime_fail_read.log && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
fi

if [ $EXIT_CODE -ne 65 ]; then
  echo "Expected exit code 65, got $EXIT_CODE"
  exit 1
fi

grep -q "\[FATAL\] Runtime memory violation" ./tmp_log/runtime_fail_read.log || { echo "Log format mismatch or missing runtime violation log"; exit 1; }
grep -q "Delta: Exceeds bounds by 0x3010 bytes." ./tmp_log/runtime_fail_read.log || { echo "Incorrect Delta calculation or missing Delta in Data Read log"; exit 1; }
echo "Data Read Violation Test PASSED."

# Scenario 2: Data Write Violation
echo "Generating ELF with invalid data write via Bazel..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/runtime_fail_write.elf --address 0x00000000 0x000050B7 0x0000A023 0x08000073"
else
  bazel run //tests/verilator_sim:gen_elf -- "$PWD/runtime_fail_write.elf" --address 0x00000000 0x000050B7 0x0000A023 0x08000073
fi

echo "Running simulator for Data Write Violation..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_barebones_sim -- \$PWD/runtime_fail_write.elf 2>&1 | tee ./tmp_log/runtime_fail_write.log" && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
else
  set -o pipefail
  bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/runtime_fail_write.elf" 2>&1 | tee ./tmp_log/runtime_fail_write.log && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
fi

if [ $EXIT_CODE -ne 65 ]; then
  echo "Expected exit code 65, got $EXIT_CODE"
  exit 1
fi

grep -q "\[FATAL\] Runtime memory violation" ./tmp_log/runtime_fail_write.log || { echo "Log format mismatch or missing runtime violation log"; exit 1; }
grep -q "Delta: Exceeds bounds by 0x3001 bytes." ./tmp_log/runtime_fail_write.log || { echo "Incorrect Delta calculation or missing Delta in Data Write log"; exit 1; }
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
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_barebones_sim -- \$PWD/runtime_fail_fetch.elf 2>&1 | tee ./tmp_log/runtime_fail_fetch.log" && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
else
  set -o pipefail
  bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/runtime_fail_fetch.elf" 2>&1 | tee ./tmp_log/runtime_fail_fetch.log && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
fi

if [ $EXIT_CODE -ne 65 ]; then
  echo "Expected exit code 65, got $EXIT_CODE"
  exit 1
fi

grep -q "\[FATAL\] Runtime memory violation" ./tmp_log/runtime_fail_fetch.log || { echo "Log format mismatch or missing runtime violation log"; exit 1; }
grep -q "Delta: Exceeds bounds by 0x7e8020 bytes." ./tmp_log/runtime_fail_fetch.log || { echo "Incorrect Delta calculation or missing Delta in Instruction Fetch log"; exit 1; }
echo "Instruction Fetch Violation Test PASSED."

# Scenario 4: Data Read Violation in RVVI mode
echo "Generating ELF with invalid data read for RVVI via Bazel..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/runtime_fail_read_rvvi.elf --address 0x00000000 0x000050B7 0x00008203 0x08000073"
else
  bazel run //tests/verilator_sim:gen_elf -- "$PWD/runtime_fail_read_rvvi.elf" --address 0x00000000 0x000050B7 0x00008203 0x08000073
fi

echo "Running simulator in RVVI mode for Data Read Violation..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --rvvi_out=\$PWD/trace_runtime_fail.rvvi \$PWD/runtime_fail_read_rvvi.elf 2>&1 | tee ./tmp_log/runtime_fail_read_rvvi.log" && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
else
  set -o pipefail
  bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --rvvi_out="$PWD/trace_runtime_fail.rvvi" "$PWD/runtime_fail_read_rvvi.elf" 2>&1 | tee ./tmp_log/runtime_fail_read_rvvi.log && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
fi

if [ $EXIT_CODE -ne 65 ]; then
  echo "Expected exit code 65, got $EXIT_CODE"
  exit 1
fi

grep -q "\[FATAL\] Runtime memory violation" ./tmp_log/runtime_fail_read_rvvi.log || { echo "Log format mismatch or missing runtime violation log"; exit 1; }
grep -q "Delta: Exceeds bounds by 0x3010 bytes." ./tmp_log/runtime_fail_read_rvvi.log || { echo "Incorrect Delta calculation or missing Delta in RVVI Data Read log"; exit 1; }

echo "Verifying zero trace loss in RVVI mode..."
if [ ! -f trace_runtime_fail.rvvi ]; then
  echo "Trace file not generated"
  exit 1
fi

if ! grep -q "rvvi" trace_runtime_fail.rvvi; then
  echo "Trace file empty or missing RVVI records"
  exit 1
fi

echo "RVVI Runtime Violation Test PASSED."

echo "All Runtime Violation Tests PASSED."
