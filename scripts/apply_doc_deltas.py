#!/usr/bin/env python3
# Copyright 2026 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Applies documentation deltas patch ensuring all files are mapped under doc/.
"""

import argparse
import os
import subprocess
import sys


def validate_patch(patch_path):
    """Verifies that all files in the patch are located under the doc/ directory."""
    with open(patch_path, "r") as f:
        for line in f:
            if line.startswith("diff --git"):
                parts = line.split()
                if len(parts) >= 4:
                    # b/path is the target path in the patch
                    target_path = parts[3]
                    # git diff format usually starts with a/ or b/
                    if target_path.startswith("b/"):
                        rel_path = target_path[2:]
                    else:
                        rel_path = target_path

                    if not rel_path.startswith("doc/"):
                        print(f"Error: Patch contains file outside doc/: {rel_path}")
                        return False
    return True


def apply_native_fallback(external_repo_path):
    """Fallback using git fetch and direct checkout."""
    print("Attempting native fetch fallback...")
    try:
        # 1. Fetch from external repo path
        subprocess.run(["git", "fetch", external_repo_path, "main"], check=True)
        
        # Direct checkout of doc/
        print("Trying direct checkout of doc/...")
        subprocess.run(["git", "checkout", "FETCH_HEAD", "--", "doc/"], check=True)
            
        print("Successfully applied documentation changes using native fetch fallback.")
    except subprocess.CalledProcessError as e:
        print(f"Native fetch fallback failed: {e}")
        sys.exit(1)


def apply_patch(patch_path, external_repo_path):
    """Applies the patch using git apply --index, with native fetch fallback."""
    if not os.path.isfile(patch_path):
        print(f"Error: Patch file '{patch_path}' not found.")
        sys.exit(1)

    if not validate_patch(patch_path):
        sys.exit(1)

    try:
        # Use --index to stage the changes immediately
        subprocess.run(["git", "apply", "--index", patch_path], check=True)
        print(f"Successfully applied patch: {patch_path}")
    except subprocess.CalledProcessError as e:
        print(f"Failed to apply patch: {e}")
        apply_native_fallback(external_repo_path)


def main():
    parser = argparse.ArgumentParser(description="Apply documentation deltas patch.")
    parser.add_argument("patch_file", help="Path to the .patch file")
    parser.add_argument("--external-repo", default="/usr/local/google/home/julianmb/coralnpu-hw-arch-document",
                        help="Path to the external documentation repository")
    args = parser.parse_args()

    apply_patch(args.patch_file, args.external_repo)


if __name__ == "__main__":
    main()
