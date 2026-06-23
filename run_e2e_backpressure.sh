#!/bin/bash
set -e

# Cleanup trap
trap 'rm -f ./rvvi.elf' EXIT

echo "Generating ELF at 0x00000000 via Bazel..."
podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/rvvi.elf --repeat 5000 0x00100093 0x08000073"

# Run simulator via Bazel in Podman with DelayFormatter, explicit output path, and Bazel cache mapping
echo "Running RVVI Backpressure simulator via Bazel in Podman..."
mkdir -p ./tmp_log
time podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run --copt=-DDELAY_FORMATTER //tests/verilator_sim:core_rvvi_sim -- --rvvi_out=\$PWD/trace.rvvi \$PWD/rvvi.elf 2>&1 | tee /tmp/sim.log || (cp /tmp/sim.log ./tmp_log/backpressure_sim.log; find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null; exit 1)"

echo "Checking trace output..."
# Verify 5000 addi and 1 mpause are present
addi_count=$(grep -c "00100093" trace.rvvi || true)
if [ "$addi_count" -lt 5000 ]; then
  echo "Trace file missing expected ADDI instructions: expected at least 5000, found $addi_count"
  exit 1
fi
if ! grep -q "08000073" trace.rvvi; then
  echo "Trace file missing expected instruction (08000073)"
  exit 1
fi

# Verify 'R' packets are present
r_packet_count=$(grep -c "x[0-9]+:[0-9a-fA-F]*" trace.rvvi || true)
if [ "$r_packet_count" -lt 5000 ]; then
  echo "Trace file missing expected 'R' packets (register updates): expected at least 5000, found $r_packet_count"
  exit 1
fi

echo "E2E Backpressure Resilience Test PASSED"
