#!/bin/bash
# Subsystem SafeMux Integration Test

set -e

# Run the Chisel unit test to verify SafeMuxUpTo1H handles out-of-bounds
bazel test //tests/chisel:safe_mux_test

# If it succeeds, the test boundary correctly identifies safe casting behavior
echo "Integration Verification Passed: SafeMuxUpTo1H successfully validates out-of-bounds."
