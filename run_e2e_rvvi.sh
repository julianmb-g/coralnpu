#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./test.elf ./float_test.elf' EXIT

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
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -w $PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel localhost/coralnpu bash -c "set -o pipefail; bazel build //examples:coralnpu_v2_rvv_add_intrinsic && cp bazel-bin/examples/coralnpu_v2_rvv_add_intrinsic.elf /tmp/test.elf && bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --instructions=50000 --rvvi_out=\$PWD/trace.rvvi /tmp/test.elf 2>&1 | tee \$PWD/tmp_log/rvvi_sim.log || exit 1"
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -w $PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel localhost/coralnpu bash -c "set -o pipefail; bazel build //examples:coralnpu_v2_hello_world_add_floats && cp bazel-bin/examples/coralnpu_v2_hello_world_add_floats.elf /tmp/float_test.elf && bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --instructions=50000 --rvvi_out=\$PWD/trace_float.rvvi /tmp/float_test.elf 2>&1 | tee \$PWD/tmp_log/rvvi_float_sim.log || exit 1"
else
  echo "Building binary and running simulator natively..."
  set -o pipefail
  bazel build //examples:coralnpu_v2_rvv_add_intrinsic
  cp bazel-bin/examples/coralnpu_v2_rvv_add_intrinsic.elf ./test.elf
  bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --instructions=5000 --rvvi_out="$PWD/trace.rvvi" "$PWD/test.elf" 2>&1 | tee "$PWD/tmp_log/rvvi_sim.log" || exit 1

  bazel build //examples:coralnpu_v2_hello_world_add_floats
  cp bazel-bin/examples/coralnpu_v2_hello_world_add_floats.elf ./float_test.elf
  bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --instructions=5000 --rvvi_out="$PWD/trace_float.rvvi" "$PWD/float_test.elf" 2>&1 | tee "$PWD/tmp_log/rvvi_float_sim.log" || exit 1
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

if ! grep -Eq "vsetvli.*,x[1-9][0-9]*:" trace.rvvi; then
  echo "Trace file missing vector setup instruction disassembly (vsetvli) with register update"
  exit 1
fi

echo "Checking RVVI trace output for vector load/store instructions..."
if ! grep -Eq ",[0-9a-fA-F]{6}07," trace.rvvi; then
  echo "Trace file missing Vector Load opcode (07) in instruction field"
  exit 1
fi
if ! grep -Eq "vload" trace.rvvi; then
  echo "Trace file missing vector load instruction disassembly (vload)"
  exit 1
fi
if ! grep -Eq "vload.*v[0-9]+:" trace.rvvi; then
  echo "Trace file missing vector load register updates"
  exit 1
fi
if ! grep -Eq "vstore" trace.rvvi; then
  echo "Trace file missing vector store instruction disassembly (vstore)"
  exit 1
fi

if [ "$(grep -c "rvvi" trace.rvvi)" -lt 10 ]; then
  echo "Trace file too short (potential memcpy truncation)"
  exit 1
fi

echo "Checking boundary vector registers v0-v4 and their sizes..."
for r in v0 v1 v2 v3 v4; do
  if ! grep -Eq ",${r}:[0-9a-fA-F]{32}" trace.rvvi; then
    echo "Trace file trace.rvvi is missing boundary vector register update or has invalid size: ${r}"
    exit 1
  fi
done

echo "Checking boundary FPR register f0 and f1..."
for r in f0 f1; do
  if ! grep -Eq ",${r}:[0-9a-fA-F]{8}" trace_float.rvvi; then
    echo "Trace file trace_float.rvvi is missing boundary FPR register update or has invalid size: ${r}"
    exit 1
  fi
done

echo "E2E RVVI Fidelity Test PASSED"
