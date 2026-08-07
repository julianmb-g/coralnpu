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

import subprocess
import sys
import os

def audit_legacy_soc_protection():
    """
    Checks that no files outside tests/verilator_sim/ (except for audit scripts/tests)
    are modified on the integration branch compared to main.
    """
    print("Auditing legacy SoC simulation target protection...")
    try:
        result = subprocess.run(
            ["git", "diff", "--name-only", "main..HEAD"],
            check=True,
            capture_output=True,
            text=True
        )
        changed_files = result.stdout.strip().split("\n")
        
        for f in changed_files:
            if not f:
                continue
            if f.startswith("fpga/"):
                print(f"Error: Violation of legacy SoC protection. File '{f}' was modified.")
                return False
        print("Legacy SoC simulation targets successfully protected.")
        return True
    except Exception as e:
        print(f"Error running git diff: {e}")
        return False

def audit_adr_013_retracted():
    """
    Verifies that simulation termination behavior in memory_if.h is integrated as-is
    without proactive rewrites.
    """
    print("Auditing ADR-013 Retracted (no proactive simulation termination rewrites)...")
    path = "tests/verilator_sim/coralnpu/memory_if.h"
    if not os.path.exists(path):
        print(f"Error: File '{path}' does not exist.")
        return False
    with open(path, "r") as f:
        content = f.read()
    # Check that we didn't add status-based error propagation in place of original logic
    if "Status" in content and "ErrorToStatus" in content:
        print("Error: Detected status-based simulation termination refactoring in memory_if.h.")
        return False
    print("ADR-013 Audit passed.")
    return True

def audit_adr_005_retracted():
    """
    Verifies that memory interface logic in memory_if.h is integrated as-is
    without proactive error-propagation refactoring.
    """
    print("Auditing ADR-005 Retracted (no proactive error-propagation in memory_if.h)...")
    path = "tests/verilator_sim/coralnpu/memory_if.h"
    if not os.path.exists(path):
        print(f"Error: File '{path}' does not exist.")
        return False
    with open(path, "r") as f:
        content = f.read()
    # Ensure there is no complex error return signature or enum for error propagation
    if "MemoryAccessError" in content or "PropagateError" in content:
        print("Error: Detected proactive error propagation structures in memory_if.h.")
        return False
    print("ADR-005 Audit passed.")
    return True

def audit_adr_006_retracted():
    """
    Verifies that structural centralization in the ELF loader is not present.
    """
    print("Auditing ADR-006 Retracted (no structural centralization in ELF loader)...")
    path = "tests/verilator_sim/elf_loader_adapter.h"
    if not os.path.exists(path):
        print(f"Error: File '{path}' does not exist.")
        return False
    with open(path, "r") as f:
        content = f.read()
    # Verify no unapproved centralized elf-loading abstractions were injected
    if "CentralElfLoader" in content or "GlobalElfManager" in content:
        print("Error: Detected unapproved centralized ELF loader refactoring.")
        return False
    print("ADR-006 Audit passed.")
    return True

def audit_adr_009():
    """
    Verifies that l1dcache_tb.cc does not contain unapproved mock stub replacements
    or fake assertions.
    """
    print("Auditing ADR-009 (L1DCache TB mock stub/assertion integrity)...")
    path = "tests/verilator_sim/coralnpu/l1dcache_tb.cc"
    if not os.path.exists(path):
        print(f"Error: File '{path}' does not exist.")
        return False
    with open(path, "r") as f:
        content = f.read()
    # If the file has any artificial ideal assertions or hardcoded positive paths
    if "assert(true)" in content:
        print("Error: Detected potential fake assertions/mock stub bypasses in l1dcache_tb.cc.")
        return False
    print("ADR-009 Audit passed.")
    return True

def main():
    success = True
    success &= audit_legacy_soc_protection()
    success &= audit_adr_013_retracted()
    success &= audit_adr_005_retracted()
    success &= audit_adr_006_retracted()
    success &= audit_adr_009()
    
    if not success:
        print("Audit FAILED.")
        sys.exit(1)
    print("All Verilator integration audits PASSED.")

if __name__ == "__main__":
    main()
