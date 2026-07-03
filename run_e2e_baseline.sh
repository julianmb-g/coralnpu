#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./dummy.elf' EXIT

if [ -z "${USE_PODMAN}" ]; then
  USE_PODMAN=0
  if ! command -v bazel &> /dev/null && command -v podman &> /dev/null; then
    echo "Bazel not found natively. Falling back to Podman..."
    USE_PODMAN=1
  fi
fi

# Build mandated binary
echo "Building mandated binary via Bazel..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel build //examples:coralnpu_v2_rvv_add_intrinsic && cp bazel-bin/examples/coralnpu_v2_rvv_add_intrinsic.elf \$PWD/dummy.elf"
else
  bazel build //examples:coralnpu_v2_rvv_add_intrinsic
  cp bazel-bin/examples/coralnpu_v2_rvv_add_intrinsic.elf ./dummy.elf
fi

# Run simulator via Bazel
mkdir -p ./tmp_log
if [ "$USE_PODMAN" -eq 1 ]; then
  echo "Running simulator via Bazel in Podman..."
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_barebones_sim -- \$PWD/dummy.elf 2>&1 | tee /tmp/sim.log || (cp /tmp/sim.log ./tmp_log/baseline_sim.log; find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null; exit 1)"
else
  echo "Running simulator via Bazel natively..."
  set -o pipefail
  bazel run //tests/verilator_sim:core_barebones_sim -- "$PWD/dummy.elf" 2>&1 | tee /tmp/sim.log || (cp /tmp/sim.log ./tmp_log/baseline_sim.log; find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null; exit 1)
fi

echo "E2E Baseline Emulation Test PASSED"
