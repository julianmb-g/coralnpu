<<<<<<< HEAD
import unittest
from unittest.mock import patch, call
import sys
import os
import subprocess

=======
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
>>>>>>> main
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../scripts')))

from sync_active_branches import sync_active_branches

class TestSyncActiveBranches(unittest.TestCase):
<<<<<<< HEAD
=======
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
>>>>>>> main

    @patch('sync_active_branches.verify_active_branches')
    @patch('sync_active_branches.parse_active_branches')
    @patch('subprocess.run')
<<<<<<< HEAD
    def test_sync_active_branches_success(self, mock_run, mock_parse, mock_verify):
        mock_verify.return_value = []
        mock_parse.return_value = ['branch1', 'branch2']
        
        class MockResult:
            def __init__(self, stdout, returncode=0):
                self.stdout = stdout
                self.returncode = returncode
                
        def side_effect(*args, **kwargs):
            if 'branch' in args[0] and '--show-current' in args[0]:
                return MockResult('main')
            return MockResult('', 0)
            
        mock_run.side_effect = side_effect
        
        sync_active_branches('dummy_tracker', 'dummy_repo')
        
        calls = [
            call(['git', '-C', 'dummy_repo', 'checkout', 'branch1'], check=True),
            call(['git', '-C', 'dummy_repo', 'merge', 'upstream/main', '--no-edit'], capture_output=True, text=True),
            call(['git', '-C', 'dummy_repo', 'checkout', 'branch2'], check=True),
            call(['git', '-C', 'dummy_repo', 'merge', 'upstream/main', '--no-edit'], capture_output=True, text=True),
            call(['git', '-C', 'dummy_repo', 'checkout', 'main'], check=True)
        ]
        
        for c in calls:
            self.assertIn(c, mock_run.mock_calls)

    @patch('sync_active_branches.verify_active_branches')
    @patch('sync_active_branches.parse_active_branches')
    @patch('subprocess.run')
    def test_sync_active_branches_merge_failure(self, mock_run, mock_parse, mock_verify):
        mock_verify.return_value = []
        mock_parse.return_value = ['branch1']
        
        class MockResult:
            def __init__(self, stdout, returncode=0):
                self.stdout = stdout
                self.returncode = returncode
                
        def side_effect(*args, **kwargs):
            if 'branch' in args[0] and '--show-current' in args[0]:
                return MockResult('main')
            if 'merge' in args[0] and 'upstream/main' in args[0]:
                return MockResult('conflict', 1)
            return MockResult('', 0)
            
        mock_run.side_effect = side_effect
        
        with self.assertRaises(RuntimeError) as context:
            sync_active_branches('dummy_tracker', 'dummy_repo')
            
        self.assertIn("Failed to merge upstream/main into branches: ['branch1']", str(context.exception))
        
        mock_run.assert_any_call(['git', '-C', 'dummy_repo', 'merge', '--abort'])
        mock_run.assert_any_call(['git', '-C', 'dummy_repo', 'checkout', 'main'], check=True)

    @patch('sync_active_branches.verify_active_branches')
    def test_sync_active_branches_missing(self, mock_verify):
        mock_verify.return_value = ['missing_branch']
        
        with self.assertRaises(RuntimeError) as context:
            sync_active_branches('dummy_tracker', 'dummy_repo')
            
        self.assertIn("Cannot sync. Missing local branches: ['missing_branch']", str(context.exception))
=======
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
>>>>>>> main

if __name__ == '__main__':
    unittest.main()
