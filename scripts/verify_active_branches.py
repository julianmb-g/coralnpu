import subprocess
import os
import sys

# Add scripts directory to path
sys.path.append(os.path.abspath(os.path.dirname(__file__)))

from parse_active_branches import parse_active_branches

def verify_active_branches(tracker_path):
    active_branches = parse_active_branches(tracker_path)
    
    # Get local branches
    # Need to run git branch in the integration workspace
    result = subprocess.run(['git', '-C', '/usr/local/google/home/julianmb/julianmb-g_coralnpu', 'branch', '--format=%(refname:short)'], capture_output=True, text=True)
    local_branches = result.stdout.splitlines()
    
    missing_branches = []
    for branch in active_branches:
        # Check if the branch is a substring or exactly present? 
        # The BRANCH_TRACKER might list a branch that has a remote prefix.
        if branch not in local_branches:
            missing_branches.append(branch)
    
    return missing_branches

if __name__ == "__main__":
    missing = verify_active_branches('/google/data/rw/users/ju/julianmb/wiki/projects/coralnpu-rtl-mutations/BRANCH_TRACKER.md')
    if missing:
        print(f"Missing branches: {missing}")
        exit(1)
    else:
        print("All active branches are present.")
        exit(0)
