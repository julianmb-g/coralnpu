#!/usr/bin/python3
import argparse
import os
import subprocess
import sys
import json
import re
from enum import Enum

class MutationStatus(Enum):
    SURVIVED = "SURVIVED"
    KILLED = "KILLED"
    INVALID = "INVALID"

    def __str__(self):
        return self.value

def get_absolute_path(path):
    return os.path.abspath(os.path.expanduser(path))

def patch_file(file_path, line_number, original_code, mutated_code):
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

def _remove_trailing_whitespace(lines):
    updated_lines = []
    for line in lines:
        has_newline = line.endswith("\n")
        content = line[:-1] if has_newline else line
        stripped = content.rstrip(" \t\r")
        if has_newline:
            stripped += "\n"
        updated_lines.append(stripped)
    return updated_lines

def _get_comment_character(file_path, has_shebang, shebang_line):
    ext = os.path.splitext(file_path)[1]
    is_hash_style = ext in [".py", ".sh", ".bzl", "BUILD"] or os.path.basename(file_path) == "BUILD"
    if has_shebang and shebang_line.startswith("#!"):
        is_hash_style = True
    return "#" if is_hash_style else "//"

def _generate_license_header(comment_char):
    return [
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

def _update_license_year(lines):
    copyright_index = -1
    has_license = False
    for idx, line in enumerate(lines[:15]):
        if "Copyright" in line and "Google" in line:
            copyright_index = idx
            has_license = True
            break
        elif "Licensed under the Apache" in line:
            has_license = True
            break

    if has_license:
        if copyright_index != -1:
            line = lines[copyright_index]
            updated_line = re.sub(r"Copyright\s+\d{4}(-\d{4})?", "Copyright 2026", line)
            lines[copyright_index] = updated_line
        return lines, True
    return lines, False

def enforce_adr008_constraints(file_path):
    file_path = os.path.abspath(file_path)
    if not os.path.exists(file_path):
        raise FileNotFoundError(f"File not found: {file_path}")

    with open(file_path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    has_shebang = False
    shebang_line = ""
    if lines and lines[0].startswith("#!"):
        has_shebang = True
        shebang_line = lines[0]
        lines = lines[1:]

    lines = _remove_trailing_whitespace(lines)
    lines, has_license = _update_license_year(lines)

    if not has_license:
        comment_char = _get_comment_character(file_path, has_shebang, shebang_line)
        lines = _generate_license_header(comment_char) + lines

    if has_shebang:
        lines.insert(0, shebang_line)

    with open(file_path, "w", encoding="utf-8") as f:
        f.writelines(lines)

    try:
        ext = os.path.splitext(file_path)[1]
        if ext == ".py":
            subprocess.run(["yapf3", "-i", file_path], capture_output=True, text=True)
        elif ext == ".scala":
            subprocess.run(["scalafmt", "--config", ".scalafmt", file_path], capture_output=True, text=True)
    except Exception as e:
        print(f"Warning: automatic formatter failed for {file_path}: {e}")

def commit_test_enhancements(commit_msg, files):
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

def evaluate_status(returncode):
    """
    Evaluates the Bazel test returncode and maps it to a mutation status.
    Pass (0) means the test suite passed, so the mutant SURVIVED.
    Fail (3 or 4) means tests failed, so the mutant was KILLED.
    Anything else (build error, etc.) is INVALID.
    """
    if returncode == 0:
        return MutationStatus.SURVIVED
    elif returncode in [3, 4]:
        return MutationStatus.KILLED
    else:
        return MutationStatus.INVALID

def _build_podman_command(workspace_path, bazel_cache_path, container_name, target):
    return [
        "podman", "run", "--userns=keep-id:uid=1000,gid=1000", "--pids-limit=30000", "--rm",
        "-v", f"{workspace_path}:/workspace",
        "-v", f"{bazel_cache_path}:{bazel_cache_path}",
        "-w", "/workspace",
        container_name,
        "/bin/bash", "-c",
        f"set -x; bazel --output_user_root={bazel_cache_path} --output_base={bazel_cache_path}/container_base test -j 16 {target}"
    ]

def _get_test_targets(test_target):
    if test_target:
        return [test_target]
    
    pending_path = os.path.join(os.path.dirname(__file__), "pending_mutations.json")
    if os.path.exists(pending_path):
        with open(pending_path, "r") as f:
            return json.load(f)
    
    print("No test_target specified and pending_mutations.json not found.")
    sys.exit(1)

def run_mutation_pipeline(test_target, no_revert, output_log, mutation_file=None, mutation_line=None, original_code=None, mutated_code=None):
    workspace_path = get_absolute_path(".")
    bazel_cache_path = get_absolute_path("~/.cache/bazel")
    container_name = "coralnpu"

    if not no_revert:
        try:
            subprocess.run(["git", "reset", "--hard"], capture_output=True, text=True, check=True)
            subprocess.run(["git", "clean", "-xfd"], capture_output=True, text=True, check=True)
        except subprocess.CalledProcessError as e:
            print(f"An error occurred during git reset/clean: {e}")
            sys.exit(1)

    if mutation_file and mutation_line is not None and mutated_code is not None:
        try:
            patch_file(mutation_file, mutation_line, original_code, mutated_code)
        except Exception as e:
            print(f"Error injecting mutation: {e}")
            sys.exit(1)

    targets = _get_test_targets(test_target)

    with open(output_log, "w") as log_file:
        for current_target in targets:
            podman_command = _build_podman_command(workspace_path, bazel_cache_path, container_name, current_target)

            try:
                process = subprocess.run(podman_command, capture_output=True, text=True, timeout=900)
                log_file.write(f"--- Target: {current_target} ---\n")
                log_file.write("STDOUT:\n" + process.stdout + "\n\n")
                log_file.write("STDERR:\n" + process.stderr + "\n\n")
                log_file.write(f"Exit Code: {process.returncode}\n")
                
                status = evaluate_status(process.returncode)
                print(f"Mutation Status for {current_target}: {status}")
                log_file.write(f"Mutation Status: {status}\n\n")

            except subprocess.TimeoutExpired:
                print(f"Command timed out after 900 seconds for {current_target}.")
                log_file.write(f"Command timed out after 900 seconds for {current_target}.\n\n")
            except Exception as e:
                print(f"An error occurred: {e}")
                sys.exit(1)

    if mutation_file and not no_revert:
        try:
            subprocess.run(["git", "checkout", "--", mutation_file], check=True)
            print(f"Reverted file {mutation_file}")
        except subprocess.CalledProcessError as e:
            print(f"Failed to revert {mutation_file}: {e}")
            sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run mutation testing pipeline.")
    parser.add_argument("--test_target", help="Bazel test target to run. If omitted, runs all targets in pending_mutations.json.")
    parser.add_argument("--no_revert", action="store_true", help="Do not revert changes after running the test.")
    parser.add_argument("--output_log", help="Path to the output log file.")
    
    parser.add_argument("--mutation_file", help="Target file path to inject mutation.")
    parser.add_argument("--mutation_line", type=int, help="1-based line number to mutate.")
    parser.add_argument("--original_code", help="Expected original line of code (safety check).")
    parser.add_argument("--mutated_code", help="New line of code to inject.")

    parser.add_argument("--commit_msg", help="Commit message for test enhancements.")
    parser.add_argument("--commit_files", help="Comma-separated list of files to format and commit.")
    
    args = parser.parse_args()

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

    if not args.output_log:
        parser.error("--output_log is required unless --commit_msg is specified")

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
