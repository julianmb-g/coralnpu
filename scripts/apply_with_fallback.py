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

import argparse
import subprocess
import sys

def apply_patch_with_fallback(patch_file, external_repo_path=None):
    """
    Attempts to apply a patch file using git apply. If it fails, logs the error
    and suggests a manual fallback. If external_repo_path is provided,
    it also attempts a git fetch in that repository.
    """
    try:
        print(f"Attempting to apply patch: {patch_file}")
        result = subprocess.run(
            ["git", "apply", "--index", "--whitespace=nowarn", patch_file],
            check=True,
            capture_output=True,
            text=True
        )
        print(f"Successfully applied patch: {patch_file}")
        print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Failed to apply patch: {patch_file}")
        print(f"Error: {e.stderr}")
        print("\n--- Fallback Required ---")
        print(f"Manual intervention is needed to apply {patch_file}.")
        print("Please resolve conflicts manually or use an alternative method.")

        if external_repo_path:
            print(f"Attempting git fetch in external repo: {external_repo_path}")
            try:
                subprocess.run(
                    ["git", "-C", external_repo_path, "fetch", "origin"],
                    check=True,
                    capture_output=True,
                    text=True
                )
                print(f"Successfully ran git fetch origin in {external_repo_path}")
            except subprocess.CalledProcessError as fetch_e:
                print(f"Failed to run git fetch origin in {external_repo_path}")
                print(f"Error: {fetch_e.stderr}")
        return False

def main():
    parser = argparse.ArgumentParser(description="Apply a patch with a fallback mechanism.")
    parser.add_argument("patch_file", help="The patch file to apply.")
    parser.add_argument("--external_repo", help="Optional: Path to an external git repository for fallback fetch.")
    args = parser.parse_args()

    if not apply_patch_with_fallback(args.patch_file, args.external_repo):
        sys.exit(1)

if __name__ == "__main__":
    main()
