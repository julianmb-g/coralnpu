#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./real.elf' EXIT

USE_PODMAN=${USE_PODMAN:-0}

# Generate a valid ELF that loads at 0x00000000
echo "Generating ELF at 0x00000000 via Bazel..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/real.elf 0x08000073"
else
  bazel run //tests/verilator_sim:gen_elf -- "$PWD/real.elf" 0x08000073
fi

# Run the simulator
echo "Running simulator with real ELF via Bazel..."
mkdir -p ./tmp_log
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_barebones_sim -- \$PWD/real.elf 2>&1 | tee \$PWD/tmp_log/real_address_sim.log || exit 1"
else
  set -o pipefail
  bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/real.elf" 2>&1 | tee "$PWD/tmp_log/real_address_sim.log" || exit 1
fi

echo "E2E Real Address Loading Test PASSED"