import unittest
import os
import sys
import tempfile

# Add scripts directory to path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../scripts')))

from parse_active_branches import parse_active_branches

class TestParseActiveBranches(unittest.TestCase):
    def test_parse_table_starting_with_pipe(self):
        # Create a temporary file
        with tempfile.NamedTemporaryFile(mode='w', delete=False) as f:
            f.write("""
| Branch Name | Source Commit | Purpose | Status | Created |
| :--- | :--- | :--- | :--- | :--- |
| `other-branch` | main | Purpose | Active | 2026-01-01 |
""")
            temp_path = f.name
        
        try:
            branches = parse_active_branches(temp_path)
            self.assertIn('other-branch', branches)
        finally:
            os.remove(temp_path)

    def test_parse_table_without_starting_pipe(self):
        # Create a temporary file
        with tempfile.NamedTemporaryFile(mode='w', delete=False) as f:
            f.write("""
Branch Name | Source Commit | Purpose | Status | Created
:--- | :--- | :--- | :--- | :---
`feature/test-branch` | main | Purpose | Active | 2026-01-01
""")
            temp_path = f.name
        
        try:
            branches = parse_active_branches(temp_path)
            self.assertIn('feature/test-branch', branches)
        finally:
            os.remove(temp_path)

if __name__ == '__main__':
    unittest.main()
