#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./rvvi.elf' EXIT

USE_PODMAN=${USE_PODMAN:-0}

# Generate a valid ELF that loads at 0x00000000
echo "Generating ELF at 0x00000000 via Bazel..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/rvvi.elf 0x00000013 0x08000073"
else
  bazel run //tests/verilator_sim:gen_elf -- "$PWD/rvvi.elf" 0x00000013 0x08000073
fi

# Run simulator via Bazel
mkdir -p ./tmp_log
if [ "$USE_PODMAN" -eq 1 ]; then
  echo "Running RVVI simulator via Bazel in Podman..."
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_rvvi_sim -- --rvvi_out=\$PWD/trace.rvvi \$PWD/rvvi.elf 2>&1 | tee /tmp/sim.log || (cp /tmp/sim.log ./tmp_log/termination_sim.log; find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null; exit 1)"
else
  echo "Running RVVI simulator via Bazel natively..."
  set -o pipefail
  bazel run //tests/verilator_sim:core_rvvi_sim -- --rvvi_out="$PWD/trace.rvvi" "$PWD/rvvi.elf" 2>&1 | tee /tmp/sim.log || (cp /tmp/sim.log ./tmp_log/termination_sim.log; find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null; exit 1)
fi

echo "Checking RVVI trace output for graceful termination..."
if ! grep -q "08000073" trace.rvvi; then
  echo "Trace file missing expected mpause instruction (08000073)"
  exit 1
fi

echo "E2E Graceful Termination Test PASSED"