#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./timeout.elf ./barebones_out.log ./rvvi_out.log' EXIT

if [ -z "${USE_PODMAN}" ]; then
  USE_PODMAN=0
  if ! command -v bazel &> /dev/null && command -v podman &> /dev/null; then
    echo "Bazel not found natively. Falling back to Podman..."
    USE_PODMAN=1
  fi
fi

echo "Generating ELF at 0x00000000 with infinite loop (0x0000006f) via Bazel..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/timeout.elf 0x0000006f"
else
  bazel run //tests/verilator_sim:gen_elf -- "$PWD/timeout.elf" 0x0000006f
fi

# Run Barebones simulator and expect timeout
echo "Running Barebones simulator via Bazel (expecting timeout)..."
mkdir -p ./tmp_log
set +e
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_barebones_sim -- --instructions=5000 \$PWD/timeout.elf 2>&1 | tee \$PWD/tmp_log/timeout_barebones_sim.log"
else
  set -o pipefail
  bazel run //tests/verilator_sim:core_barebones_sim -- --instructions=5000 "$PWD/timeout.elf" 2>&1 | tee "$PWD/tmp_log/timeout_barebones_sim.log"
fi
EXIT_CODE=$?
find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null
set -e

if [ $EXIT_CODE -eq 65 ]; then
  echo "Simulator incorrectly exited with 65 (ELF load failure code) instead of 1 on timeout/deadlock!"
  exit 1
fi
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
echo "Running RVVI simulator via Bazel (expecting timeout)..."
set +e
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_rvvi_sim -- --instructions=5000 --rvvi_out=\$PWD/trace.rvvi \$PWD/timeout.elf 2>&1 | tee \$PWD/tmp_log/timeout_rvvi_sim.log"
else
  set -o pipefail
  bazel run //tests/verilator_sim:core_rvvi_sim -- --instructions=5000 --rvvi_out="$PWD/trace.rvvi" "$PWD/timeout.elf" 2>&1 | tee "$PWD/tmp_log/timeout_rvvi_sim.log"
fi
EXIT_CODE=$?
find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null
set -e

if [ $EXIT_CODE -eq 65 ]; then
  echo "Simulator incorrectly exited with 65 (ELF load failure code) instead of 1 on timeout/deadlock!"
  exit 1
fi
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