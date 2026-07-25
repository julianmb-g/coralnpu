import subprocess
import os
import sys

# Add scripts directory to path
sys.path.append(os.path.abspath(os.path.dirname(__file__)))

from parse_active_branches import parse_active_branches
from verify_active_branches import verify_active_branches

def sync_active_branches(tracker_path, repo_path):
    missing = verify_active_branches(tracker_path)
    if missing:
        raise RuntimeError(f"Cannot sync. Missing local branches: {missing}")
        
    active_branches = parse_active_branches(tracker_path)
    
    if not active_branches:
        return
    
    # Store current branch
    result = subprocess.run(['git', '-C', repo_path, 'branch', '--show-current'], capture_output=True, text=True, check=True)
    original_branch = result.stdout.strip()
    
    failed_merges = []
    
    try:
        for branch in active_branches:
            # checkout branch
            subprocess.run(['git', '-C', repo_path, 'checkout', branch], check=True)
            
            # merge upstream/main
            merge_result = subprocess.run(['git', '-C', repo_path, 'merge', 'upstream/main', '--no-edit'], capture_output=True, text=True)
            if merge_result.returncode != 0:
                failed_merges.append(branch)
                # abort the merge
                subprocess.run(['git', '-C', repo_path, 'merge', '--abort'])
    finally:
        # restore original branch
        if original_branch:
            subprocess.run(['git', '-C', repo_path, 'checkout', original_branch], check=True)
            
    if failed_merges:
        raise RuntimeError(f"Failed to merge upstream/main into branches: {failed_merges}")

if __name__ == "__main__":
    try:
        sync_active_branches(
            '/google/data/rw/users/ju/julianmb/wiki/projects/coralnpu-rtl-mutations/BRANCH_TRACKER.md',
            '/usr/local/google/home/julianmb/julianmb-g_coralnpu'
        )
        print("Successfully synced all active branches.")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
