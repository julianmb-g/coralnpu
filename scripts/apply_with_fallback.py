#!/usr/bin/env python3
"""Applies a git patch with fallback mechanisms.

This script attempts to apply a git patch. If the initial application fails,
it implements fallback strategies, including fetching from a remote and
attempting merges or cherry-picks.
"""

import argparse
import subprocess
import sys

def run_command(command, cwd=None):
    """Runs a shell command and returns the output and success status."""
    try:
        result = subprocess.run(
            command,
            cwd=cwd,
            capture_output=True,
            text=True,
            check=False
        )
        if result.returncode != 0:
            print(f"Command failed: {' '.join(command)}", file=sys.stderr)
            print(f"Stdout:
{result.stdout}", file=sys.stderr)
            print(f"Stderr:
{result.stderr}", file=sys.stderr)
        return result.returncode == 0, result.stdout, result.stderr
    except FileNotFoundError:
        print(f"Error: Command not found: {command[0]}", file=sys.stderr)
        return False, "", f"Command not found: {command[0]}"

def apply_patch(patch_file, cwd):
    """Attempts to apply a git patch."""
    print(f"Attempting to apply patch: {patch_file} in {cwd}")
    success, stdout, stderr = run_command(["git", "apply", "--index", patch_file], cwd=cwd)
    return success, stdout, stderr

def fallback_with_cherry_pick(remote, branch, cwd):
    """Attempts to fetch from a remote and cherry-pick the changes."""
    print(f"Attempting fallback: git fetch {remote} {branch} && git cherry-pick {remote}/{branch}")
    success, _, _ = run_command(["git", "fetch", remote, branch], cwd=cwd)
    if not success:
        return False, "", "Fetch failed"
    success, stdout, stderr = run_command(["git", "cherry-pick", f"{remote}/{branch}"], cwd=cwd)
    return success, stdout, stderr

def main():
    parser = argparse.ArgumentParser(description="Apply a git patch with fallback.")
    parser.add_argument("patch_file", help="The patch file to apply.")
    parser.add_argument("--cwd", default=".", help="The current working directory.")
    parser.add_argument("--remote", help="The remote to fetch from for fallback.")
    parser.add_argument("--branch", help="The branch to fetch from for fallback.")
    args = parser.parse_args()

    patch_file = args.patch_file
    cwd = args.cwd
    remote = args.remote
    branch = args.branch

    # Attempt to apply the patch
    print(f"Trying initial git apply for {patch_file}")
    success, _, _ = apply_patch(patch_file, cwd)
    if success:
        print(f"Successfully applied {patch_file}.")
    elif remote and branch:
        print(f"Initial apply of {patch_file} failed. Attempting fallback.")
        success, _, _ = fallback_with_cherry_pick(remote, branch, cwd)
        if success:
            print(f"Successfully applied fallback.")
        else:
            print(f"Fallback failed.")
            sys.exit(1)
    else:
        print(f"Initial apply of {patch_file} failed and no fallback provided.")
        sys.exit(1)

if __name__ == "__main__":
    main()
