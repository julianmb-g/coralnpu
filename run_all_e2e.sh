#!/bin/bash
set -e

echo "Running all E2E tests sequentially to avoid Bazel lock contention..."

for script in run_e2e_*.sh; do
    if [ "$script" != "run_all_e2e.sh" ]; then
        echo "======================================"
        echo "Executing $script"
        echo "======================================"
        ./"$script"
    fi
done

echo "All E2E tests completed successfully."
