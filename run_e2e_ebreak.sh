#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./ebreak.elf' EXIT

# Generate a valid ELF that loads at 0x00000000 with ebreak...
echo "Generating ELF at 0x00000000 with ebreak via Bazel..."
podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/ebreak.elf 0x00100073"

# Run simulator via Bazel in Podman
echo "Running RVVI simulator via Bazel in Podman..."
mkdir -p ./tmp_log
set +e
podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_rvvi_sim -- --rvvi_out=\$PWD/trace.rvvi \$PWD/ebreak.elf 2>&1 | tee \$PWD/tmp_log/ebreak_sim.log"
EXIT_CODE=$?
find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null
set -e

echo "Checking RVVI trace output..."
if ! grep -q "00100073" trace.rvvi; then
  echo "Trace file missing expected instruction (00100073)"
  exit 1
fi

if ! grep -q "io_fault" ./tmp_log/ebreak_sim.log; then
  echo "Simulator did not fault on ebreak as expected!"
  exit 1
fi

echo "E2E Ebreak Termination Test PASSED"