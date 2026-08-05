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
from unittest.mock import patch, MagicMock, call
import os
import sys

# Add scripts directory to path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../scripts')))

from verify_active_branches_build import verify_active_branches_build

class TestVerifyActiveBranchesBuild(unittest.TestCase):
    """Unit tests for verify_active_branches_build.py"""
    
    @patch('verify_active_branches_build.parse_active_branches')
    @patch('subprocess.run')
    def test_verify_build_success(self, mock_run, mock_parse):
        mock_parse.return_value = ['branch_active_1']
        
        # Mock git and podman/bazel commands
        def side_effect(cmd, **kwargs):
            if 'branch' in cmd and '--show-current' in cmd:
                return MagicMock(returncode=0, stdout='main\n')
            return MagicMock(returncode=0, stdout='')
            
        mock_run.side_effect = side_effect
        
        failures = verify_active_branches_build('dummy_tracker.md', '/repo')
        
        self.assertEqual(failures, [])
        # Verify podman bazel build was called
        mock_run.assert_any_call(['podman', 'run', '--userns=keep-id:uid=1000,gid=1000', '--pids-limit=-1', '-it', '--rm', 
                                  '-v', '/repo:/repo', '-w', '/repo', 
                                  'localhost/coralnpu-validation', 'bash', '-c', 
                                  'git config --global --add safe.directory /repo && bazel build //...'])

    @patch('verify_active_branches_build.parse_active_branches')
    @patch('subprocess.run')
    def test_verify_build_failure(self, mock_run, mock_parse):
        mock_parse.return_value = ['branch_active_1']
        
        # Mock git show-current, checkout, and bazel build (fail)
        mock_run.side_effect = [
            MagicMock(returncode=0, stdout='main\n'), # show-current
            MagicMock(returncode=0),                  # checkout branch
            MagicMock(returncode=1),                  # bazel build (FAIL)
            MagicMock(returncode=0),                  # restore branch
        ]
        
        failures = verify_active_branches_build('dummy_tracker.md', '/repo')
        
        self.assertEqual(failures, [('branch_active_1', 'build')])

if __name__ == '__main__':
    unittest.main()
