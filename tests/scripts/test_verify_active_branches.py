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
from unittest.mock import patch, MagicMock, mock_open
import os
import sys

# Add scripts directory to path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../scripts')))

from verify_active_branches import verify_active_branches

class TestVerifyActiveBranches(unittest.TestCase):
    """Unit tests for verify_active_branches.py"""
    
    @patch('verify_active_branches.parse_active_branches')
    @patch('subprocess.run')
    def test_verify_active_branches_all_present(self, mock_run, mock_parse):
        # Mock active branches from tracker
        mock_parse.return_value = ['branch_active_1']
        
        # Mock git branch output
        mock_run.return_value = MagicMock(returncode=0, stdout='main\nbranch_active_1\n')
        
        missing_locally, missing_both = verify_active_branches('dummy_tracker.md', '/dummy/repo')
        
        self.assertEqual(missing_locally, [])
        self.assertEqual(missing_both, [])

    @patch('verify_active_branches.parse_active_branches')
    @patch('subprocess.run')
    @patch('builtins.open', new_callable=mock_open)
    def test_verify_active_branches_fetch_success(self, mock_file, mock_run, mock_parse):
        # Mock active branches from tracker
        mock_parse.return_value = ['branch_missing']
        
        # Mock git branch (missing) and git fetch (success)
        mock_run.side_effect = [
            MagicMock(returncode=0, stdout='main\n'), # git branch
            MagicMock(returncode=0, stdout='')        # git fetch
        ]
        
        missing_locally, missing_both = verify_active_branches('dummy_tracker.md', '/repo', '/external')
        
        self.assertEqual(missing_locally, ['branch_missing'])
        self.assertEqual(missing_both, [])
        # Verify fetch was attempted
        mock_run.assert_any_call(['git', '-C', '/repo', 'fetch', '/external', 'branch_missing:branch_missing'], 
                                  capture_output=True, text=True)
        # Verify log entry was written
        mock_file.assert_called_with('/google/data/rw/users/ju/julianmb/wiki/projects/coralnpu-integration/CONFLICT_LOG.md', 'a')

    @patch('verify_active_branches.parse_active_branches')
    @patch('subprocess.run')
    def test_verify_active_branches_fetch_failure(self, mock_run, mock_parse):
        # Mock active branches from tracker
        mock_parse.return_value = ['branch_missing']
        
        # Mock git branch (missing) and git fetch (failure)
        mock_run.side_effect = [
            MagicMock(returncode=0, stdout='main\n'), # git branch
            MagicMock(returncode=1, stderr='Branch not found on remote') # git fetch
        ]
        
        missing_locally, missing_both = verify_active_branches('dummy_tracker.md', '/repo', '/external')
        
        self.assertEqual(missing_locally, [])
        self.assertEqual(missing_both, ['branch_missing'])

if __name__ == '__main__':
    unittest.main()
