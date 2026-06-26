#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./dummy_fail.elf' EXIT

if [ -z "${USE_PODMAN}" ]; then
  USE_PODMAN=0
  if ! command -v bazel &> /dev/null && command -v podman &> /dev/null; then
    echo "Bazel not found natively. Falling back to Podman..."
    USE_PODMAN=1
  fi
fi

echo "Generating out-of-bounds ELF via Bazel..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/dummy_fail.elf --address 0x00800000 0x08000073"
else
  bazel run //tests/verilator_sim:gen_elf -- "$PWD/dummy_fail.elf" --address 0x00800000 0x08000073
fi

mkdir -p ./tmp_log
if [ "$USE_PODMAN" -eq 1 ]; then
  echo "Running simulator via Bazel in Podman..."
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_barebones_sim -- \$PWD/dummy_fail.elf 2>&1 | tee ./tmp_log/elf_load_fail.log" && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
else
  echo "Running simulator via Bazel natively..."
  set -o pipefail
  bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/dummy_fail.elf" 2>&1 | tee ./tmp_log/elf_load_fail.log && { echo "Expected failure, but succeeded"; exit 1; } || EXIT_CODE=$?
fi

grep -q "\[FATAL\] ELF load violation" ./tmp_log/elf_load_fail.log || { echo "Log format mismatch"; exit 1; }

echo "E2E ELF Load Fail Test PASSED"
