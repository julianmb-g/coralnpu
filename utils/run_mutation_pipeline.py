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

    if not no_revert:
        try:
            reset_process = subprocess.run(["git", "reset", "--hard"], capture_output=True, text=True)
            if reset_process.returncode != 0:
                print(f"Error: git reset failed with code {reset_process.returncode}\n{reset_process.stderr}")
                sys.exit(1)
        except FileNotFoundError:
            print("Error: git command not found. Ensure Git is installed and in your PATH.")
            sys.exit(1)
        except Exception as e:
            print(f"An error occurred during git reset: {e}")
            sys.exit(1)

    podman_command = [
        "podman", "run", "--userns=keep-id:uid=1000,gid=1000", "--pids-limit=30000", "--rm",
        "-v", f"{workspace_path}:/workspace",
        "-v", f"{bazel_cache_path}:{bazel_cache_path}",
        "-w", "/workspace",
        container_name,
        "/bin/bash", "-c",
        f"set -x; bazel --output_user_root={bazel_cache_path} --output_base={bazel_cache_path}/container_base test -j 16 {test_target}"
    ]

    try:
        with open(output_log, "w") as log_file:
            process = subprocess.run(podman_command, capture_output=True, text=True, timeout=900)
            log_file.write("STDOUT:\n" + process.stdout + "\n\n")
            log_file.write("STDERR:\n" + process.stderr + "\n\n")
            log_file.write(f"Exit Code: {process.returncode}\n")

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
