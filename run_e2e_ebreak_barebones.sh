#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./ebreak.elf' EXIT

USE_PODMAN=${USE_PODMAN:-0}

# Generate ELF with ebreak (0x00100073)
echo "Generating ELF at 0x00000000 with ebreak..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=791913,gid=89939 -v $PWD:$PWD -w $PWD -e GIT_CONFIG_GLOBAL=/tmp/gitconfig localhost/coralnpu bash -c "touch /tmp/gitconfig && git config --global --add safe.directory /usr/local/google/home/julianmb/coralnpu-verilator-core && bazel run //tests/verilator_sim:gen_elf -- \$PWD/ebreak.elf 0x00100073"
else
  bazel run //tests/verilator_sim:gen_elf -- "$PWD/ebreak.elf" 0x00100073
fi

# Run barebones simulator
echo "Running Barebones simulator..."
mkdir -p ./tmp_log
set +e
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=791913,gid=89939 -v $PWD:$PWD -w $PWD -e GIT_CONFIG_GLOBAL=/tmp/gitconfig localhost/coralnpu bash -c "touch /tmp/gitconfig && git config --global --add safe.directory /usr/local/google/home/julianmb/coralnpu-verilator-core && bazel run //tests/verilator_sim:core_barebones_sim -- \$PWD/ebreak.elf 2>&1 | tee \$PWD/tmp_log/ebreak_barebones.log"
else
  set -o pipefail
  bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/ebreak.elf" 2>&1 | tee "$PWD/tmp_log/ebreak_barebones.log"
fi
EXIT_CODE=$?
set -e

if [ "$EXIT_CODE" -ne 0 ]; then
  echo "Simulator failed with exit code $EXIT_CODE"
  exit 1
fi

echo "E2E Barebones Ebreak Termination Test PASSED"
