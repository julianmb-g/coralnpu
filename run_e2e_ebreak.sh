#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./ebreak.elf' EXIT

if [ -z "${USE_PODMAN}" ]; then
  USE_PODMAN=0
  if ! command -v bazel &> /dev/null && command -v podman &> /dev/null; then
    echo "Bazel not found natively. Falling back to Podman..."
    USE_PODMAN=1
  fi
fi

# Generate a valid ELF that loads at 0x00000000 with ebreak...
echo "Generating ELF at 0x00000000 with ebreak via Bazel..."
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/ebreak.elf 0x00100073"
else
  bazel run //tests/verilator_sim:gen_elf -- "$PWD/ebreak.elf" 0x00100073
fi

# Run simulator via Bazel
echo "Running RVVI simulator via Bazel..."
mkdir -p ./tmp_log
set +e
if [ "$USE_PODMAN" -eq 1 ]; then
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --rvvi_out=\$PWD/trace.rvvi \$PWD/ebreak.elf 2>&1 | tee \$PWD/tmp_log/ebreak_sim.log"
else
  set -o pipefail
  bazel run //tests/verilator_sim:core_rvvi_traced_sim -- --rvvi_out="$PWD/trace.rvvi" "$PWD/ebreak.elf" 2>&1 | tee "$PWD/tmp_log/ebreak_sim.log"
fi
EXIT_CODE=$?
find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null
set -e

echo "Checking RVVI trace output..."
if ! grep -q "00100073" trace.rvvi; then
  echo "Trace file missing expected instruction (00100073)"
  exit 1
fi

if [ "$EXIT_CODE" -ne 0 ]; then
  echo "Simulator failed with exit code $EXIT_CODE"
  exit 1
fi

echo "E2E Ebreak Termination Test PASSED"