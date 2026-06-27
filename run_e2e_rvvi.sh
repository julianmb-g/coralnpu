#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./test.elf' EXIT

if [ -z "${USE_PODMAN}" ]; then
  USE_PODMAN=0
  if ! command -v bazel &> /dev/null && command -v podman &> /dev/null; then
    echo "Bazel not found natively. Falling back to Podman..."
    USE_PODMAN=1
  fi
fi

mkdir -p ./tmp_log

if [ "$USE_PODMAN" -eq 1 ]; then
  echo "Building binary and running simulator in Podman..."
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -w $PWD -v /tmp/bazel_cache:/home/builder/.cache localhost/coralnpu bash -c "set -o pipefail; bazel build //examples:coralnpu_v2_rvv_add_intrinsic && cp bazel-bin/examples/coralnpu_v2_rvv_add_intrinsic.elf \$PWD/test.elf && bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --instructions=50000 --rvvi_out=\$PWD/trace.rvvi \$PWD/test.elf 2>&1 | tee \$PWD/tmp_log/rvvi_sim.log || exit 1"
else
  echo "Building binary and running simulator natively..."
  set -o pipefail
  bazel build //examples:coralnpu_v2_rvv_add_intrinsic
  cp bazel-bin/examples/coralnpu_v2_rvv_add_intrinsic.elf ./test.elf
  bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --instructions=50000 --rvvi_out="$PWD/trace.rvvi" "$PWD/test.elf" 2>&1 | tee "$PWD/tmp_log/rvvi_sim.log" || exit 1
fi

echo "Checking RVVI trace output for vector instructions and vector registers..."
if ! grep -Eq "[0-9a-fA-F]{6}57" trace.rvvi; then
  echo "Trace file missing vector instructions (opcode 57)"
  exit 1
fi
if ! grep -Eq "v[0-9]+:[0-9a-fA-F]*[1-9a-fA-F][0-9a-fA-F]*" trace.rvvi; then
  echo "Trace file missing non-zero vector register updates"
  exit 1
fi

if ! grep -Eq "v[w]?add" trace.rvvi; then
  echo "Trace file missing vector instruction disassembly (e.g., vadd)"
  exit 1
fi

if ! grep -Eq "vsetvli" trace.rvvi; then
  echo "Trace file missing vector setup instruction disassembly (vsetvli)"
  exit 1
fi

if [ "$(grep -c "rvvi" trace.rvvi)" -lt 10 ]; then
  echo "Trace file too short (potential memcpy truncation)"
  exit 1
fi

echo "E2E RVVI Fidelity Test PASSED"
