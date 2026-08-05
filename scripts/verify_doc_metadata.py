#!/usr/bin/env python3
"""
Audits all markdown files in doc/ for the presence of the correct traceability footer.
"""

import argparse
import os
import sys


def verify_metadata(doc_dir, sha_file):
    """Verifies the presence of the traceability footer in all .md files."""
    if not os.path.isfile(sha_file):
        print(f"Error: SHA file '{sha_file}' not found.")
        return False

    with open(sha_file, "r") as f:
        sha = f.read().strip()

    missing = []
    for root, _, files in os.walk(doc_dir):
        for file in files:
            if file.endswith(".md"):
                file_path = os.path.join(root, file)
                with open(file_path, "r") as f:
                    content = f.read()

                if f"Derived from upstream commit {sha}" not in content:
                    missing.append(file_path)

    if missing:
        print(f"Verification FAILED: {len(missing)} files missing correct metadata.")
        for m in missing:
            print(f"  {m}")
        return False

    print("Verification PASSED: All files contain correct metadata.")
    return True


def main():
    parser = argparse.ArgumentParser(description="Verify documentation metadata.")
    parser.add_argument("--doc-dir", default="doc", help="Directory containing documentation")
    parser.add_argument("--sha-file", default="/tmp/doc_upstream_sha_julianmb.txt", help="File containing the upstream SHA")
    args = parser.parse_args()

    if not verify_metadata(args.doc_dir, args.sha_file):
        sys.exit(1)


if __name__ == "__main__":
    main()
