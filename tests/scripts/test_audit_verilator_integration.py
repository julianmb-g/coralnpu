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

import unittest
from unittest.mock import patch, mock_open, MagicMock
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'scripts')))
import audit_verilator_integration

class TestAuditVerilatorIntegration(unittest.TestCase):

    @patch('subprocess.run')
    def test_audit_legacy_soc_protection_success(self, mock_run):
        mock_run.return_value = MagicMock(stdout="tests/verilator_sim/coralnpu/core_tb.cc\n", returncode=0)
        self.assertTrue(audit_verilator_integration.audit_legacy_soc_protection())

    @patch('subprocess.run')
    def test_audit_legacy_soc_protection_failure(self, mock_run):
        mock_run.return_value = MagicMock(stdout="fpga/rtl/coralnpu_soc.sv\n", returncode=0)
        self.assertFalse(audit_verilator_integration.audit_legacy_soc_protection())

    @patch('os.path.exists')
    @patch('builtins.open', new_callable=mock_open, read_data="class MemoryIf {}")
    def test_audit_adr_013_retracted_success(self, mock_file, mock_exists):
        mock_exists.return_value = True
        self.assertTrue(audit_verilator_integration.audit_adr_013_retracted())

    @patch('os.path.exists')
    @patch('builtins.open', new_callable=mock_open, read_data="class MemoryIf { Status ErrorToStatus(); }")
    def test_audit_adr_013_retracted_failure(self, mock_file, mock_exists):
        mock_exists.return_value = True
        self.assertFalse(audit_verilator_integration.audit_adr_013_retracted())

    @patch('os.path.exists')
    @patch('builtins.open', new_callable=mock_open, read_data="class MemoryIf {}")
    def test_audit_adr_005_retracted_success(self, mock_file, mock_exists):
        mock_exists.return_value = True
        self.assertTrue(audit_verilator_integration.audit_adr_005_retracted())

    @patch('os.path.exists')
    @patch('builtins.open', new_callable=mock_open, read_data="class MemoryIf { MemoryAccessError PropagateError(); }")
    def test_audit_adr_005_retracted_failure(self, mock_file, mock_exists):
        mock_exists.return_value = True
        self.assertFalse(audit_verilator_integration.audit_adr_005_retracted())

    @patch('os.path.exists')
    @patch('builtins.open', new_callable=mock_open, read_data="class ElfLoader {}")
    def test_audit_adr_006_retracted_success(self, mock_file, mock_exists):
        mock_exists.return_value = True
        self.assertTrue(audit_verilator_integration.audit_adr_006_retracted())

    @patch('os.path.exists')
    @patch('builtins.open', new_callable=mock_open, read_data="class CentralElfLoader {}")
    def test_audit_adr_006_retracted_failure(self, mock_file, mock_exists):
        mock_exists.return_value = True
        self.assertFalse(audit_verilator_integration.audit_adr_006_retracted())

    @patch('os.path.exists')
    @patch('builtins.open', new_callable=mock_open, read_data="class L1DCacheTb {}")
    def test_audit_adr_009_success(self, mock_file, mock_exists):
        mock_exists.return_value = True
        self.assertTrue(audit_verilator_integration.audit_adr_009())

    @patch('os.path.exists')
    @patch('builtins.open', new_callable=mock_open, read_data="class L1DCacheTb { assert(true); }")
    def test_audit_adr_009_failure(self, mock_file, mock_exists):
        mock_exists.return_value = True
        self.assertFalse(audit_verilator_integration.audit_adr_009())

if __name__ == "__main__":
    unittest.main()
