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
from unittest.mock import patch, mock_open
import sys
import os

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'scripts')))
import audit_fallback_formatter

class TestAuditFallbackFormatter(unittest.TestCase):

    @patch('os.path.exists')
    @patch('builtins.open', new_callable=mock_open, read_data="class TraceDaemon {}")
    def test_audit_trace_daemon_success(self, mock_file, mock_exists):
        mock_exists.return_value = True
        self.assertTrue(audit_fallback_formatter.audit_trace_daemon())

    @patch('os.path.exists')
    @patch('builtins.open', new_callable=mock_open, read_data="class TraceDaemon { void ProactiveTraceDaemonRefactor(); }")
    def test_audit_trace_daemon_failure(self, mock_file, mock_exists):
        mock_exists.return_value = True
        self.assertFalse(audit_fallback_formatter.audit_trace_daemon())

    @patch('os.path.exists')
    def test_audit_mpact_formatter_not_exists_success(self, mock_exists):
        mock_exists.return_value = False
        self.assertTrue(audit_fallback_formatter.audit_mpact_formatter())

    @patch('os.path.exists')
    @patch('builtins.open', new_callable=mock_open, read_data="class MpactTraceFormatter {}")
    def test_audit_mpact_formatter_exists_success(self, mock_file, mock_exists):
        mock_exists.return_value = True
        self.assertTrue(audit_fallback_formatter.audit_mpact_formatter())

    @patch('os.path.exists')
    @patch('builtins.open', new_callable=mock_open, read_data="class MpactTraceFormatter { void ProactiveFormatterRefactor(); }")
    def test_audit_mpact_formatter_exists_failure(self, mock_file, mock_exists):
        mock_exists.return_value = True
        self.assertFalse(audit_fallback_formatter.audit_mpact_formatter())

    @patch('os.path.exists')
    def test_audit_fallback_disassembler_not_exists(self, mock_exists):
        mock_exists.return_value = False
        self.assertTrue(audit_fallback_formatter.audit_fallback_disassembler())

    @patch('os.path.exists')
    @patch('builtins.open', new_callable=mock_open, read_data="Disassembler logic")
    def test_audit_fallback_disassembler_success(self, mock_file, mock_exists):
        mock_exists.return_value = True
        self.assertTrue(audit_fallback_formatter.audit_fallback_disassembler())

    @patch('os.path.exists')
    @patch('builtins.open', new_callable=mock_open, read_data="ProactiveDisassemblerEngineering")
    def test_audit_fallback_disassembler_failure(self, mock_file, mock_exists):
        mock_exists.return_value = True
        self.assertFalse(audit_fallback_formatter.audit_fallback_disassembler())

if __name__ == "__main__":
    unittest.main()
