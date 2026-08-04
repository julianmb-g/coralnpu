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

import subprocess
import os
import sys

# Add scripts directory to path
sys.path.append(os.path.abspath(os.path.dirname(__file__)))

from parse_active_branches import parse_active_branches
from verify_active_branches import verify_active_branches

def sync_active_branches(tracker_path, repo_path):
    """Syncs active branches with upstream/main using a merge policy."""
    # verify_active_branches returns (missing_locally, missing_both)
    missing_locally, missing_both = verify_active_branches(tracker_path, repo_path)
    if missing_both:
        raise RuntimeError(f"Cannot sync. Missing branches from both repos: {missing_both}")
        
    active_branches = parse_active_branches(tracker_path)
    if not active_branches:
        print("No active branches found to sync.")
        return
    
    # Store current branch
    result = subprocess.run(['git', '-C', repo_path, 'branch', '--show-current'], 
                            capture_output=True, text=True, check=True)
    original_branch = result.stdout.strip()
    
    failed_merges = []
    
    try:
        for branch in active_branches:
            print(f"Syncing branch '{branch}' with upstream/main...")
            subprocess.run(['git', '-C', repo_path, 'checkout', branch], check=True)
            
            # Attempt merge
            merge_result = subprocess.run(['git', '-C', repo_path, 'merge', 'upstream/main', '--no-edit'], 
                                          capture_output=True, text=True)
            
            if merge_result.returncode != 0:
                print(f"Conflict detected in branch '{branch}'. Applying resolution policy (Design 5.2)...")
                # Get list of conflicting files
                status_result = subprocess.run(['git', '-C', repo_path, 'status', '--porcelain'], 
                                               capture_output=True, text=True)
                conflicts = [line[3:] for line in status_result.stdout.splitlines() if line.startswith('UU')]
                
                for f in conflicts:
                    # Policy:
                    # Hardware source files (*.scala, *.v, *.sv, *.svh) -> --ours (preserve bugs)
                    # Test benches (*_tb.cc, *Test.scala, etc.) -> --theirs (accept upstream fixes)
                    # Others -> --ours (conservative)
                    
                    is_hdl = any(f.endswith(ext) for ext in ['.scala', '.v', '.sv', '.svh'])
                    is_test = 'tests/' in f or 'test' in f.lower()
                    
                    if is_hdl and not is_test:
                        print(f"  HDL conflict in {f}: favoring local change (--ours)")
                        subprocess.run(['git', '-C', repo_path, 'checkout', '--ours', f], check=True)
                    elif is_test:
                        print(f"  Test conflict in {f}: favoring upstream change (--theirs)")
                        subprocess.run(['git', '-C', repo_path, 'checkout', '--theirs', f], check=True)
                    else:
                        print(f"  Unknown conflict in {f}: favoring local change (--ours)")
                        subprocess.run(['git', '-C', repo_path, 'checkout', '--ours', f], check=True)
                        
                    subprocess.run(['git', '-C', repo_path, 'add', f], check=True)
                
                # Commit resolution
                commit_result = subprocess.run(['git', '-C', repo_path, 'commit', '--no-edit'], 
                                               capture_output=True, text=True)
                if commit_result.returncode != 0:
                    print(f"Failed to commit merge resolution for '{branch}': {commit_result.stderr}")
                    subprocess.run(['git', '-C', repo_path, 'merge', '--abort'])
                    failed_merges.append(branch)
            else:
                print(f"Successfully merged upstream/main into '{branch}'.")
                
    finally:
        # restore original branch
        if original_branch:
            subprocess.run(['git', '-C', repo_path, 'checkout', original_branch], check=True)
            
    if failed_merges:
        raise RuntimeError(f"Failed to sync branches due to unresolvable conflicts: {failed_merges}")

if __name__ == "__main__":
    tracker_path = os.getenv('BRANCH_TRACKER_PATH', '/google/data/rw/users/ju/julianmb/wiki/projects/coralnpu-rtl-mutations/BRANCH_TRACKER.md')
    repo_path = os.getenv('INTEGRATION_REPO_PATH', os.getcwd())
    
    try:
        sync_active_branches(tracker_path, repo_path)
        print("Successfully synced all active branches.")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
