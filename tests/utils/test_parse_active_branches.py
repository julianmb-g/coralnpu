import unittest
import sys
import os

# Add scripts directory to path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../scripts')))

from parse_active_branches import parse_active_branches

class TestParseActiveBranches(unittest.TestCase):
    def test_parse(self):
        # Create a temporary file
        with open('temp_tracker.md', 'w') as f:
            f.write("""
- [ ] feature/test-branch [STATUS: ACTIVE]
| Branch Name | Source Commit | Purpose | Status | Created |
| :--- | :--- | :--- | :--- | :--- |
| `other-branch` | main | Purpose | Active | 2026-01-01 |
""")
        branches = parse_active_branches('temp_tracker.md')
        self.assertIn('feature/test-branch', branches)
        self.assertIn('other-branch', branches)
        os.remove('temp_tracker.md')

if __name__ == '__main__':
    unittest.main()
