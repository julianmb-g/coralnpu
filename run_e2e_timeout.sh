#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./timeout.elf ./barebones_out.log ./rvvi_out.log' EXIT

echo "Generating ELF at 0x00000000 with 600,000 NOPs via Bazel..."
podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/timeout.elf --repeat 600000 0x00000013"

# Run Barebones simulator and expect timeout
echo "Running Barebones simulator via Bazel in Podman (expecting timeout)..."
mkdir -p ./tmp_log
set +e
podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_barebones_sim -- --cycles=5000 \$PWD/timeout.elf 2>&1 | tee \$PWD/tmp_log/timeout_barebones_sim.log"
EXIT_CODE=$?
find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null
set -e

if [ $EXIT_CODE -ne 1 ]; then
  echo "Barebones simulator did not exit with code 1 (Exit Code: $EXIT_CODE)"
  exit 1
fi

if ! grep -q "Simulation TIMEOUT" ./tmp_log/timeout_barebones_sim.log; then
  echo "Barebones simulator output missing 'Simulation TIMEOUT'"
  exit 1
fi

echo "Barebones simulator timed out as expected."

# Run RVVI simulator and expect timeout
echo "Running RVVI simulator via Bazel in Podman (expecting timeout)..."
set +e
podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_rvvi_sim -- --cycles=5000 --rvvi_out=\$PWD/trace.rvvi \$PWD/timeout.elf 2>&1 | tee \$PWD/tmp_log/timeout_rvvi_sim.log"
EXIT_CODE=$?
find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null
set -e

if [ $EXIT_CODE -ne 1 ]; then
  echo "RVVI simulator did not exit with code 1 (Exit Code: $EXIT_CODE)"
  exit 1
fi

if ! grep -q "Simulation TIMEOUT" ./tmp_log/timeout_rvvi_sim.log; then
  echo "RVVI simulator output missing 'Simulation TIMEOUT'"
  exit 1
fi

echo "RVVI simulator timed out as expected."

echo "E2E Timeout Verification Test PASSED"