#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./rvvi.elf' EXIT

if [ -z "${USE_PODMAN}" ]; then
  USE_PODMAN=0
  if ! command -v bazel &> /dev/null && command -v podman &> /dev/null; then
    echo "Bazel not found natively. Falling back to Podman..."
    USE_PODMAN=1
  fi
fi

echo "Generating ELF at 0x00000000 via Bazel..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/rvvi.elf --repeat 5 0x00100093 0x08000073"
else
  bazel run //tests/verilator_sim:gen_elf -- "$PWD/rvvi.elf" --repeat 5 0x00100093 0x08000073
fi

# Run simulator via Bazel with 10s artificial delay to trigger flush timeout (5s)
mkdir -p ./tmp_log
echo "Running RVVI Flush Timeout simulator via Bazel..."

set +e
if [ "$USE_PODMAN" -eq 1 ]; then
  time podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --memory_profile=default --rvvi_out=\$PWD/trace.rvvi --artificial_delay_ms=10000 \$PWD/rvvi.elf 2>&1 | tee /tmp/sim.log"
  EXIT_CODE=$?
else
  time bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --memory_profile=default --rvvi_out="$PWD/trace.rvvi" --artificial_delay_ms=10000 "$PWD/rvvi.elf" 2>&1 | tee /tmp/sim.log
  EXIT_CODE=$?
fi
set -e

if [ "$EXIT_CODE" -eq 124 ]; then
  echo "E2E Flush Timeout Test PASSED (Exit code correctly 124)"
else
  echo "E2E Flush Timeout Test FAILED. Expected exit code 124, got $EXIT_CODE"
  cp /tmp/sim.log ./tmp_log/flush_timeout_sim.log || true
  exit 1
fi
