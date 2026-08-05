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

from sync_active_branches import sync_active_branches

class TestSyncActiveBranches(unittest.TestCase):
    """Unit tests for sync_active_branches.py"""
    
    @patch('sync_active_branches.verify_active_branches')
    @patch('sync_active_branches.parse_active_branches')
    @patch('subprocess.run')
    def test_sync_success(self, mock_run, mock_parse, mock_verify):
        mock_verify.return_value = ([], []) # (missing_locally, missing_both)
        mock_parse.return_value = ['branch_active_1']
        
        # Mock git commands
        def side_effect(cmd, **kwargs):
            if '--show-current' in cmd:
                return MagicMock(returncode=0, stdout='main\n')
            return MagicMock(returncode=0, stdout='')
            
        mock_run.side_effect = side_effect
        
        sync_active_branches('dummy_tracker.md', '/repo')
        
        # Verify no-rebase merge was called
        mock_run.assert_any_call(['git', '-C', '/repo', 'merge', 'upstream/main', '--no-edit'], 
                                  capture_output=True, text=True)

    @patch('sync_active_branches.verify_active_branches')
    @patch('sync_active_branches.parse_active_branches')
    @patch('subprocess.run')
    def test_sync_conflict_resolution(self, mock_run, mock_parse, mock_verify):
        mock_verify.return_value = ([], [])
        mock_parse.return_value = ['branch_active_1']
        
        # Mock sequence of git commands:
        # 1. show-current -> main
        # 2. checkout branch_active_1 -> success
        # 3. merge -> conflict (returncode 1)
        # 4. status --porcelain -> conflicting files
        # 5. checkout --ours/--theirs -> success
        # 6. add -> success
        # 7. commit -> success
        # 8. checkout main -> success
        
        mock_run.side_effect = [
            MagicMock(returncode=0, stdout='main\n'), # show-current
            MagicMock(returncode=0),                  # checkout branch
            MagicMock(returncode=1, stderr='Conflict'), # merge (fail)
            MagicMock(returncode=0, stdout='UU src/Core.scala\nUU tests/tb.cc\n'), # status
            MagicMock(returncode=0), # checkout --ours Core.scala
            MagicMock(returncode=0), # add Core.scala
            MagicMock(returncode=0), # checkout --theirs tb.cc
            MagicMock(returncode=0), # add tb.cc
            MagicMock(returncode=0), # commit
            MagicMock(returncode=0), # checkout main
        ]
        
        sync_active_branches('dummy_tracker.md', '/repo')
        
        # Verify resolution policy
        mock_run.assert_any_call(['git', '-C', '/repo', 'checkout', '--ours', 'src/Core.scala'], check=True)
        mock_run.assert_any_call(['git', '-C', '/repo', 'checkout', '--theirs', 'tests/tb.cc'], check=True)
        mock_run.assert_any_call(['git', '-C', '/repo', 'commit', '--no-edit'], capture_output=True, text=True)

if __name__ == '__main__':
    unittest.main()
