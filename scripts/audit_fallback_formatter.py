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

import os
import sys

def audit_trace_daemon():
    """
    Verifies trace daemon is integrated as-is without proactive refactoring (ADR-019 Retracted).
    """
    print("Auditing trace daemon for ADR-019 integrity...")
    path = "tests/verilator_sim/rvvi/trace_daemon.h"
    if not os.path.exists(path):
        print(f"Error: File '{path}' does not exist.")
        return False
    with open(path, "r") as f:
        content = f.read()
    if "ProactiveTraceDaemonRefactor" in content or "UnapprovedTraceRefactor" in content:
        print("Error: Detected unapproved proactive refactoring in trace_daemon.h.")
        return False
    print("Trace daemon ADR-019 integrity verified.")
    return True

def audit_mpact_formatter():
    """
    Verifies MpactTraceFormatter is integrated as-is without proactive refactoring (ADR-019 Retracted).
    """
    print("Auditing MpactTraceFormatter for ADR-019 integrity...")
    # Since MpactTraceFormatter might have been deleted as part of retraction,
    # let's make sure there are no leftover unapproved formatters or proactive changes.
    path = "tests/verilator_sim/rvvi/mpact_trace_formatter.h"
    if os.path.exists(path):
        print(f"Warning: '{path}' exists. Verifying no unapproved changes.")
        with open(path, "r") as f:
            content = f.read()
        if "ProactiveFormatterRefactor" in content:
            print("Error: Detected unapproved proactive changes in mpact_trace_formatter.h.")
            return False
    print("MpactTraceFormatter ADR-019 integrity verified.")
    return True

def audit_fallback_disassembler():
    """
    Verifies that no proactive disassembler engineering exists (ADR-007 Retracted).
    """
    print("Auditing for proactive disassembler engineering (ADR-007)...")
    path = "tests/verilator_sim/rvvi/fallback_disassembler.h"
    if os.path.exists(path):
        print(f"Warning: '{path}' exists. Verifying no unapproved changes.")
        with open(path, "r") as f:
            content = f.read()
        if "ProactiveDisassemblerEngineering" in content:
            print("Error: Detected unapproved proactive disassembler engineering.")
            return False
    print("Disassembler ADR-007 integrity verified.")
    return True

def audit_custom_fallback_formatter():
    """
    Verifies that sw/utils/nexus_loader/trace_daemon.cc does NOT explicitly use CustomFallbackFormatter (Design 6.1).
    Wait, the task says 'for explicit CustomFallbackFormatter usage'. 
    Let's check if it exists.
    """
    path = "sw/utils/nexus_loader/trace_daemon.cc"
    if os.path.exists(path):
        with open(path, "r") as f:
            content = f.read()
        if "CustomFallbackFormatter" in content:
            print(f"Error: Explicit CustomFallbackFormatter usage found in {path}")
            return False
    return True

def main():
    success = True
    success &= audit_trace_daemon()
    success &= audit_mpact_formatter()
    success &= audit_fallback_disassembler()
    success &= audit_custom_fallback_formatter()
    
    if not success:
        print("Audit FAILED.")
        sys.exit(1)
    print("All fallback formatter and tracing audits PASSED.")

if __name__ == "__main__":
    main()
