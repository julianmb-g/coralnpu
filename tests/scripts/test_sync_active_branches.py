import unittest
from unittest.mock import patch, call
import sys
import os
import subprocess

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../scripts')))

from sync_active_branches import sync_active_branches

class TestSyncActiveBranches(unittest.TestCase):

    @patch('sync_active_branches.verify_active_branches')
    @patch('sync_active_branches.parse_active_branches')
    @patch('subprocess.run')
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

if __name__ == '__main__':
    unittest.main()
