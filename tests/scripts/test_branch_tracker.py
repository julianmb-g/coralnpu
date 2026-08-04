
import unittest
import os
import sys
# Need to import the function, maybe I should put it in a module?
# Or just keep it simple.

def parse_branch_tracker(file_path):
    active_branches = []
    finalized_branches = []
    if not os.path.exists(file_path):
        return [], []
    with open(file_path, 'r') as f:
        lines = f.readlines()
    for line in lines:
        if "STATUS: ACTIVE" in line:
            active_branches.append(line.strip())
        elif "Finalized" in line:
            finalized_branches.append(line.strip())
    return active_branches, finalized_branches

class TestBranchTrackerParser(unittest.TestCase):
    def test_parse_tracker(self):
        # Create a dummy tracker file
        with open('dummy_tracker.md', 'w') as f:
            f.write("- [ ] branch1 [STATUS: ACTIVE]\n")
            f.write("- [ ] branch2 [Finalized]\n")
        
        active, finalized = parse_branch_tracker('dummy_tracker.md')
        self.assertEqual(len(active), 1)
        self.assertEqual(len(finalized), 1)
        os.remove('dummy_tracker.md')

if __name__ == '__main__':
    unittest.main()
