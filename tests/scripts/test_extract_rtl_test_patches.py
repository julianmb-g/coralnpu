import unittest
import sys
import os

# Add scripts directory to path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../scripts')))

# We expect to import this, but it might not exist yet or be a stub
try:
    from extract_rtl_test_patches import parse_finalized_branches
except ImportError:
    # Allow test to be written before implementation
    pass

class TestExtractRtlTestPatches(unittest.TestCase):
    def test_parse_finalized_branches(self):
        # Create a temporary file
        temp_file = 'temp_tracker_finalized.md'
        with open(temp_file, 'w') as f:
            f.write("""
| Branch Name | Source Commit | Purpose | Status | Created |
| :--- | :--- | :--- | :--- | :--- |
| `mutation/1` | main | Purpose | Finalized (Audit Resolved) | 2026-01-01 |
| `mutation/2` | main | Purpose | Active | 2026-01-01 |
| `mutation/3` | main | Purpose | Finalized (Mutant Killed, Restored) | 2026-01-01 |
| `mutation/4` | main | Purpose | Closed | 2026-01-01 |
| `mutation/5` | main | Purpose | Finalized (Mutant Killed, Test Enhanced, Restored) | 2026-01-01 |
""")
        
        # We need to import it here if it failed before
        from extract_rtl_test_patches import parse_finalized_branches
        
        branches = parse_finalized_branches(temp_file)
        
        self.assertIn('mutation/1', branches)
        self.assertNotIn('mutation/2', branches)
        self.assertIn('mutation/3', branches)
        self.assertNotIn('mutation/4', branches)
        self.assertIn('mutation/5', branches)
        
        os.remove(temp_file)

if __name__ == '__main__':
    unittest.main()
