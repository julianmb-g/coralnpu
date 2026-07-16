#!/usr/bin/python3
import argparse
import os
import subprocess
import sys

def get_absolute_path(path):
    return os.path.abspath(os.path.expanduser(path))

def run_mutation_pipeline(test_target, no_revert, output_log):
    workspace_path = get_absolute_path(".")
    bazel_cache_path = get_absolute_path("~/.cache/bazel")
    container_name = "coralnpu"

    podman_command = [
        "podman", "run", "--userns=keep-id:uid=1000,gid=1000", "--pids-limit=30000", "--rm",
        "-v", f"{workspace_path}:/workspace",
        "-v", f"{bazel_cache_path}:/home/builder/.cache/bazel",
        "-w", "/workspace",
        container_name,
        "/bin/bash", "-c",
        f"set -x; bazel test {test_target}"
    ]

    print("DEBUG: Running command: " + " ".join(podman_command))
    try:
        with open(output_log, "w") as log_file:
            print(f"DEBUG: Opened log file: {output_log}")
            process = subprocess.run(podman_command, capture_output=True, text=True, timeout=900)
            print(f"DEBUG: subprocess.run returned with exit code: {process.returncode}")
            log_file.write("STDOUT:\n" + process.stdout + "\n\n")
            log_file.write("STDERR:\n" + process.stderr + "\n\n")
            log_file.write(f"Exit Code: {process.returncode}\n")
            print(f"DEBUG: Log file written.")

            if process.returncode == 0:
                print(f"Test for {test_target} PASSED.")
            else:
                print(f"Test for {test_target} FAILED with exit code {process.returncode}.")
    except subprocess.TimeoutExpired:
        print(f"Command timed out after 900 seconds.")
    except FileNotFoundError:
        print(f"Error: podman command not found. Ensure Podman is installed and in your PATH.")
        sys.exit(1)
    except Exception as e:
        print(f"An error occurred: {e}")
        sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run mutation testing pipeline.")
    parser.add_argument("--test_target", required=True, help="Bazel test target to run.")
    parser.add_argument("--no_revert", action="store_true", help="Do not revert changes after running the test.")
    parser.add_argument("--output_log", required=True, help="Path to the output log file.")
    args = parser.parse_args()

    temp_dir = "/usr/local/google/home/julianmb/.gemini/tmp/coralnpu-rtl-mutations"
    if not args.output_log.startswith(temp_dir):
        print(f"Error: output_log must be within {temp_dir}")
        sys.exit(1)

    run_mutation_pipeline(args.test_target, args.no_revert, args.output_log)
