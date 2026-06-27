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
  podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "bazel run //tests/verilator_sim:gen_elf -- \$PWD/rvvi.elf --repeat 5000 0x00100093 0x08000073"
else
  bazel run //tests/verilator_sim:gen_elf -- "$PWD/rvvi.elf" --repeat 5000 0x00100093 0x08000073
fi

# Run simulator via Bazel with DelayFormatter, explicit output path, and Bazel cache mapping
mkdir -p ./tmp_log
if [ "$USE_PODMAN" -eq 1 ]; then
  echo "Running RVVI Backpressure simulator via Bazel in Podman..."
  time podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -v $HOME/.cache/bazel:/home/builder/.cache/bazel -w $PWD localhost/coralnpu bash -c "set -o pipefail; bazel run --copt=-DBUFFER_SIZE=2 //tests/verilator_sim:core_rvvi_traced_sim -- --memory_profile=all --rvvi_out=\$PWD/trace.rvvi \$PWD/rvvi.elf 2>&1 | tee /tmp/sim.log || (cp /tmp/sim.log ./tmp_log/backpressure_sim.log; find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null; exit 1)"
else
  echo "Running RVVI Backpressure simulator via Bazel natively..."
  set -o pipefail
  time bazel run --copt=-DBUFFER_SIZE=2 //tests/verilator_sim:core_rvvi_traced_sim -- --memory_profile=all --rvvi_out="$PWD/trace.rvvi" "$PWD/rvvi.elf" 2>&1 | tee /tmp/sim.log || (cp /tmp/sim.log ./tmp_log/backpressure_sim.log; find bazel-bin -name '*.log' -exec cp {} ./tmp_log/ \; 2>/dev/null; exit 1)
fi

echo "Checking trace output..."
# Verify 5000 addi and 1 mpause are present
addi_count=$(grep -c "00100093" trace.rvvi || true)
if [ "$addi_count" -ne 5000 ]; then
  echo "Trace file has unexpected ADDI instruction count: expected exactly 5000, found $addi_count"
  exit 1
fi
if ! grep -q "08000073" trace.rvvi; then
  echo "Trace file missing expected instruction (08000073)"
  exit 1
fi

# Verify 'R' packets are present
r_packet_count=$(grep -E -o "x[0-9]+:[0-9a-fA-F]*" trace.rvvi | wc -l || true)
# Trim whitespace from wc -l output
r_packet_count=$(echo $r_packet_count | xargs)
if [ "$r_packet_count" -ne 5000 ]; then
  echo "Trace file has unexpected 'R' packet count (register updates): expected exactly 5000, found $r_packet_count"
  exit 1
fi

echo "E2E Backpressure Resilience Test PASSED"
