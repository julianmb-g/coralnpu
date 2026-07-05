#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./test.elf ./float_test.elf' EXIT

./utils/ensure_writable.sh ./tmp_log

echo "Building binary and running simulator natively..."
set -o pipefail
bazel build //examples:coralnpu_v2_rvv_add_intrinsic
cp bazel-bin/examples/coralnpu_v2_rvv_add_intrinsic.elf ./test.elf
bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --instructions=50000 --rvvi_out="$PWD/trace.rvvi" "$PWD/test.elf" 2>&1 | tee "$PWD/tmp_log/rvvi_sim.log" || exit 1

bazel build //examples:coralnpu_v2_hello_world_add_floats
cp bazel-bin/examples/coralnpu_v2_hello_world_add_floats.elf ./float_test.elf
bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --instructions=50000 --rvvi_out="$PWD/trace_float.rvvi" "$PWD/float_test.elf" 2>&1 | tee "$PWD/tmp_log/rvvi_float_sim.log" || exit 1
set +o pipefail

echo "Checking RVVI trace output for scalar GPR writes (OP-IMM)..."
if ! grep -Eq ",[0-9a-fA-F]{6}13," trace.rvvi; then
  echo "Trace file missing OP-IMM instructions (opcode 13)"
  exit 1
fi

if ! grep -Eq ",[0-9a-fA-F]{6}13,.*,x[1-9][0-9]*:" trace.rvvi; then
  echo "Trace file missing GPR register updates for OP-IMM instructions"
  exit 1
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
for reg_name in v0 v1 v2 v3 v4; do
  if ! grep -Eq ",${reg_name}:[0-9a-fA-F]{32}" trace.rvvi; then
    echo "Trace file trace.rvvi is missing boundary vector register update or has invalid size: ${reg_name}"
    exit 1
  fi
done

echo "Checking boundary FPR register f0 and f1..."
for reg_name in f0 f1; do
  if ! grep -Eq ",${reg_name}:[0-9a-fA-F]{8}" trace_float.rvvi; then
    echo "Trace file trace_float.rvvi is missing boundary FPR register update or has invalid size: ${reg_name}"
    exit 1
  fi
done

echo "E2E RVVI Fidelity Test PASSED"
