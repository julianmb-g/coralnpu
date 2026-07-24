import unittest
from unittest.mock import patch, MagicMock
import sys
import os

# Add scripts directory to path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../scripts')))

from verify_active_branches import verify_active_branches

class TestVerifyActiveBranches(unittest.TestCase):
    @patch('verify_active_branches.parse_active_branches')
    @patch('subprocess.run')
    def test_verify(self, mock_subprocess, mock_parse):
        mock_parse.return_value = ['branch1', 'branch2']
        mock_subprocess.return_value = MagicMock(stdout='branch1\n')
        
        missing = verify_active_branches('dummy_path')
        self.assertEqual(missing, ['branch2'])

if __name__ == '__main__':
    unittest.main()
