#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./ebreak.elf' EXIT

if [ -z "${USE_PODMAN}" ]; then
  USE_PODMAN=0
  if ! command -v bazel &> /dev/null && command -v podman &> /dev/null; then
    echo "Bazel not found natively. Falling back to Podman..."
    USE_PODMAN=1
  fi
fi

# Generate ELF with ebreak (0x00100073)
echo "Generating ELF at 0x00000000 with ebreak..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 -v $PWD:$PWD -w $PWD -e GIT_CONFIG_GLOBAL=/tmp/gitconfig localhost/coralnpu bash -c "touch /tmp/gitconfig && git config --global --add safe.directory /usr/local/google/home/julianmb/coralnpu-verilator-core && bazel run //tests/verilator_sim:gen_elf -- \$PWD/ebreak.elf 0x00100073"
else
  bazel run //tests/verilator_sim:gen_elf -- "$PWD/ebreak.elf" 0x00100073
fi

# Run barebones simulator
echo "Running Barebones simulator..."
mkdir -p ./tmp_log
set +e
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 -v $PWD:$PWD -w $PWD -e GIT_CONFIG_GLOBAL=/tmp/gitconfig localhost/coralnpu bash -c "set -o pipefail; touch /tmp/gitconfig && git config --global --add safe.directory /usr/local/google/home/julianmb/coralnpu-verilator-core && bazel run //tests/verilator_sim:core_barebones_sim -- \$PWD/ebreak.elf 2>&1 | tee \$PWD/tmp_log/ebreak_barebones.log; exit \${PIPESTATUS[0]}"
else
  set -o pipefail
  bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/ebreak.elf" 2>&1 | tee "$PWD/tmp_log/ebreak_barebones.log"
  EXIT_CODE=${PIPESTATUS[0]}
fi
if [ "$USE_PODMAN" -eq 1 ]; then
  EXIT_CODE=$?
fi
set -e

if [ "$EXIT_CODE" -ne 0 ]; then
  echo "Simulator failed with exit code $EXIT_CODE"
  exit 1
fi

echo "E2E Barebones Ebreak Termination Test PASSED"
