#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./dummy.elf' EXIT

USE_PODMAN=${USE_PODMAN:-0}

# Generate a dummy valid ELF
echo "Generating dummy ELF via Bazel..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/dummy.elf 0x08000073"
else
  bazel run //tests/verilator_sim:gen_elf -- "$PWD/dummy.elf" 0x08000073
fi

# Run simulator via Bazel
mkdir -p ./tmp_log
if [ "$USE_PODMAN" -eq 1 ]; then
  echo "Running simulator via Bazel in Podman..."
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_barebones_sim -- \$PWD/dummy.elf 2>&1 | tee /tmp/sim.log || (cp /tmp/sim.log ./tmp_log/baseline_sim.log; find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null; exit 1)"
else
  echo "Running simulator via Bazel natively..."
  set -o pipefail
  bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/dummy.elf" 2>&1 | tee /tmp/sim.log || (cp /tmp/sim.log ./tmp_log/baseline_sim.log; find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null; exit 1)
fi

echo "E2E Baseline Emulation Test PASSED"
