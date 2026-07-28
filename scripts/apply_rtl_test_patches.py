#!/usr/bin/env python3
import argparse
import subprocess
import sys

def apply_patches(patch_file, external_repo_path=None):
    """
    Attempts to apply a patch file using git apply --index. If it fails,
    it falls back to running git fetch in the external repository.
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
        return True
    except subprocess.CalledProcessError as e:
        print(f"Failed to apply patch: {patch_file}")
        print(f"Error: {e.stderr}")

        if external_repo_path:
            print(f"Attempting native fetch fallback in: {external_repo_path}")
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
    parser = argparse.ArgumentParser(description="Apply RTL patches with native fetch fallback.")
    parser.add_argument("patch_file", help="The patch file to apply.")
    parser.add_argument("--external_repo", default="/usr/local/google/home/julianmb/coralnpu-rtl-mutations", help="Path to external git repository for fallback fetch.")
    args = parser.parse_args()

    if not apply_patches(args.patch_file, args.external_repo):
        sys.exit(1)

if __name__ == "__main__":
    main()
