#!/usr/bin/python3
import argparse
import os
import subprocess
import sys

def get_absolute_path(path):
    return os.path.abspath(os.path.expanduser(path))

def patch_file(file_path, line_number, original_code, mutated_code):
    """
    Patches file_path at line_number (1-based index).
    Optionally verifies that the existing line contains original_code.
    """
    file_path = os.path.abspath(file_path)
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"Target file for mutation not found: {file_path}")
        
    with open(file_path, "r", encoding="utf-8") as f:
        lines = f.readlines()
        
    if line_number < 1 or line_number > len(lines):
        raise IndexError(f"Line number {line_number} is out of bounds for file {file_path} (total lines: {len(lines)})")
        
    actual_line = lines[line_number - 1]
    
    if original_code and original_code not in actual_line:
        raise ValueError(f"Safety check failed: Line {line_number} in {file_path} does not contain '{original_code}'. Actual: '{actual_line.strip()}'")
        
    has_newline = actual_line.endswith("\n")
    
    new_line = mutated_code
    if not new_line.endswith("\n") and has_newline:
        new_line += "\n"
        
    lines[line_number - 1] = new_line
    
    with open(file_path, "w", encoding="utf-8") as f:
        f.writelines(lines)
    
    print(f"Injected mutation at {file_path}:{line_number}")

def enforce_adr008_constraints(file_path):
    """
    Enforces ADR-008 constraints on file_path:
    - Removes trailing whitespaces.
    - Ensures correct license header with copyright year 2026.
    - Runs formatting commands if available.
    """
    file_path = os.path.abspath(file_path)
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"File not found: {file_path}")

    # Read lines
    with open(file_path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    # Detect and preserve shebang line
    has_shebang = False
    shebang_line = ""
    if lines and lines[0].startswith("#!"):
        has_shebang = True
        shebang_line = lines[0]
        lines = lines[1:]

    # 1. Remove trailing whitespaces
    updated_lines = []
    for line in lines:
        has_newline = line.endswith("\n")
        content = line[:-1] if has_newline else line
        stripped = content.rstrip(" \t\r")
        if has_newline:
            stripped += "\n"
        updated_lines.append(stripped)

    # 2. Check and enforce license header with year 2026
    ext = os.path.splitext(file_path)[1]
    is_hash_style = ext in [".py", ".sh", ".bzl", "BUILD"] or os.path.basename(file_path) == "BUILD"
    if has_shebang and shebang_line.startswith("#!"):
        is_hash_style = True
    comment_char = "#" if is_hash_style else "//"

    full_license_header = [
        f"{comment_char} Copyright 2026 Google LLC\n",
        f"{comment_char}\n",
        f"{comment_char} Licensed under the Apache License, Version 2.0 (the \"License\");\n",
        f"{comment_char} you may not use this file except in compliance with the License.\n",
        f"{comment_char} You may obtain a copy of the License at\n",
        f"{comment_char}\n",
        f"{comment_char}     http://www.apache.org/licenses/LICENSE-2.0\n",
        f"{comment_char}\n",
        f"{comment_char} Unless required by applicable law or agreed to in writing, software\n",
        f"{comment_char} distributed under the License is distributed on an \"AS IS\" BASIS,\n",
        f"{comment_char} WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n",
        f"{comment_char} See the License for the specific language governing permissions and\n",
        f"{comment_char} limitations under the License.\n",
        "\n"
    ]

    # Try to find existing copyright/license
    has_license = False
    copyright_index = -1
    for idx, line in enumerate(updated_lines[:15]):
        if "Copyright" in line and "Google" in line:
            copyright_index = idx
            has_license = True
            break
        elif "Licensed under the Apache" in line:
            has_license = True
            break

    if has_license:
        if copyright_index != -1:
            line = updated_lines[copyright_index]
            import re
            updated_line = re.sub(r"Copyright\s+\d{4}(-\d{4})?", "Copyright 2026", line)
            updated_lines[copyright_index] = updated_line
    else:
        updated_lines = full_license_header + updated_lines

    # Prepend shebang line if existed
    if has_shebang:
        updated_lines.insert(0, shebang_line)

    # Write back the changes
    with open(file_path, "w", encoding="utf-8") as f:
        f.writelines(updated_lines)

    # 3. Run formatters
    try:
        if ext == ".py":
            process = subprocess.run(["yapf3", "-i", file_path], capture_output=True, text=True)
            if process.returncode != 0:
                print(f"Warning: yapf3 failed on {file_path} with exit code {process.returncode}:\n{process.stderr}")
        elif ext == ".scala":
            process = subprocess.run(["scalafmt", "--config", ".scalafmt", file_path], capture_output=True, text=True)
            if process.returncode != 0:
                print(f"Warning: scalafmt failed on {file_path} with exit code {process.returncode}:\n{process.stderr}")
    except Exception as e:
        print(f"Warning: automatic formatter failed for {file_path}: {e}")

def commit_test_enhancements(commit_msg, files):
    """
    Applies constraints and commits the target files with Gemini identity.
    """
    if not files:
        print("No files specified for commit.")
        return

    for file_path in files:
        enforce_adr008_constraints(file_path)
        subprocess.run(["git", "add", file_path], check=True)

    env = os.environ.copy()
    env["GIT_COMMITTER_NAME"] = "Gemini"
    env["GIT_COMMITTER_EMAIL"] = "gemini@google.com"

    commit_cmd = [
        "git", "commit",
        "--author=Gemini <gemini@google.com>",
        "-m", commit_msg
    ]
    subprocess.run(commit_cmd, env=env, check=True)
    print(f"Successfully committed {len(files)} files with Gemini identity.")

def run_mutation_pipeline(test_target, no_revert, output_log, mutation_file=None, mutation_line=None, original_code=None, mutated_code=None):
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

        try:
            clean_process = subprocess.run(["git", "clean", "-xfd"], capture_output=True, text=True)
            if clean_process.returncode != 0:
                print(f"Error: git clean failed with code {clean_process.returncode}\n{clean_process.stderr}")
                sys.exit(1)
        except Exception as e:
            print(f"An error occurred during git clean: {e}")
            sys.exit(1)

    # Injected code mutation/patching before running tests
    if mutation_file and mutation_line is not None and mutated_code is not None:
        try:
            patch_file(mutation_file, mutation_line, original_code, mutated_code)
        except Exception as e:
            print(f"Error injecting mutation: {e}")
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
    parser.add_argument("--test_target", help="Bazel test target to run.")
    parser.add_argument("--no_revert", action="store_true", help="Do not revert changes after running the test.")
    parser.add_argument("--output_log", help="Path to the output log file.")
    
    # New options for mutation patching
    parser.add_argument("--mutation_file", help="Target file path to inject mutation.")
    parser.add_argument("--mutation_line", type=int, help="1-based line number to mutate.")
    parser.add_argument("--original_code", help="Expected original line of code (safety check).")
    parser.add_argument("--mutated_code", help="New line of code to inject.")

    # New options for committing test enhancements (ADR-008)
    parser.add_argument("--commit_msg", help="Commit message for test enhancements.")
    parser.add_argument("--commit_files", help="Comma-separated list of files to format and commit.")
    
    args = parser.parse_args()

    # Validate mutation arguments to prevent silent fail-through (BUG-03)
    mutation_args = [args.mutation_file, args.mutation_line, args.mutated_code]
    provided_count = sum(1 for x in mutation_args if x is not None)
    if 0 < provided_count < 3:
        parser.error("All of --mutation_file, --mutation_line, and --mutated_code must be specified together.")

    if args.commit_msg:
        if not args.commit_files:
            parser.error("--commit_files is required when --commit_msg is specified")
        files = [f.strip() for f in args.commit_files.split(",") if f.strip()]
        commit_test_enhancements(args.commit_msg, files)
        sys.exit(0)

    if not args.test_target or not args.output_log:
        parser.error("--test_target and --output_log are required unless --commit_msg is specified")

    temp_dir = "/usr/local/google/home/julianmb/.gemini/tmp/coralnpu-rtl-mutations"
    if not args.output_log.startswith(temp_dir):
        print(f"Error: output_log must be within {temp_dir}")
        sys.exit(1)

    run_mutation_pipeline(
        test_target=args.test_target,
        no_revert=args.no_revert,
        output_log=args.output_log,
        mutation_file=args.mutation_file,
        mutation_line=args.mutation_line,
        original_code=args.original_code,
        mutated_code=args.mutated_code
    )
