#!/bin/bash
set -e

# Run simulator via Bazel in Podman
echo "Building binary and running simulator in Podman..."
mkdir -p ./tmp_log
podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -w $PWD -v /tmp/bazel_cache:/home/builder/.cache localhost/coralnpu bash -c "set -o pipefail; bazel build //examples:coralnpu_v2_rvv_add_intrinsic && cp bazel-bin/examples/coralnpu_v2_rvv_add_intrinsic.elf /tmp/test.elf && bazel run //tests/verilator_sim:core_rvvi_sim -- --cycles=50000 --rvvi_out=\$PWD/trace.rvvi /tmp/test.elf 2>&1 | tee \$PWD/tmp_log/rvvi_sim.log || exit 1"

echo "Checking RVVI trace output for vector instructions and vector registers..."
if ! grep -Eq "[0-9a-fA-F]{6}57" trace.rvvi; then
  echo "Trace file missing vector instructions (opcode 57)"
  exit 1
fi
if ! grep -Eq "v[0-9]+:[0-9a-fA-F]*[1-9a-fA-F][0-9a-fA-F]*" trace.rvvi; then
  echo "Trace file missing non-zero vector register updates"
  exit 1
fi

echo "E2E RVVI Fidelity Test PASSED"
