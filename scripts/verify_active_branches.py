<<<<<<< HEAD
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

>>>>>>> main
import subprocess
import os
import sys

# Add scripts directory to path
sys.path.append(os.path.abspath(os.path.dirname(__file__)))

from parse_active_branches import parse_active_branches

<<<<<<< HEAD
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
=======
def verify_active_branches(tracker_path, repo_path=None, external_repo_path=None):
    """Verifies and fetches active branches listed in the tracker."""
    active_branches = parse_active_branches(tracker_path)
    
    if repo_path is None:
        repo_path = os.getenv('INTEGRATION_REPO_PATH', os.getcwd())
    if external_repo_path is None:
        external_repo_path = os.getenv('EXTERNAL_REPO_PATH', '/usr/local/google/home/julianmb/coralnpu-rtl-mutations')
    
    # Get local branches
    result = subprocess.run(['git', '-C', repo_path, 'branch', '--format=%(refname:short)'], 
                            capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Error running git branch: {result.stderr}")
        return [], active_branches # Assume all missing from both if git fails locally
        
    local_branches = result.stdout.splitlines()
    
    missing_locally = []
    missing_from_both = []

    for branch in active_branches:
        if branch not in local_branches:
            print(f"Branch '{branch}' missing locally. Attempting to fetch from {external_repo_path}...")
            # Try to fetch from external repo
            fetch_cmd = ['git', '-C', repo_path, 'fetch', external_repo_path, f'{branch}:{branch}']
            fetch_result = subprocess.run(fetch_cmd, capture_output=True, text=True)
            
            if fetch_result.returncode == 0:
                missing_locally.append(branch)
                # Log to CONFLICT_LOG.md
                log_path = '/google/data/rw/users/ju/julianmb/wiki/projects/coralnpu-integration/CONFLICT_LOG.md'
                try:
                    with open(log_path, 'a') as f:
                        f.write(f"\n## Desynchronization Event\nFetched missing active branch '{branch}' from external repository at {external_repo_path}.\n")
                except Exception as e:
                    print(f"Warning: Could not write to CONFLICT_LOG.md: {e}")
            else:
                print(f"Failed to fetch branch '{branch}': {fetch_result.stderr}")
                missing_from_both.append(branch)
                
    return missing_locally, missing_from_both

if __name__ == "__main__":
    tracker_path = os.getenv('BRANCH_TRACKER_PATH', '/google/data/rw/users/ju/julianmb/wiki/projects/coralnpu-rtl-mutations/BRANCH_TRACKER.md')
    repo_path = os.getenv('INTEGRATION_REPO_PATH', os.getcwd())
    external_repo_path = os.getenv('EXTERNAL_REPO_PATH', '/usr/local/google/home/julianmb/coralnpu-rtl-mutations')
    
    missing_locally, missing_both = verify_active_branches(tracker_path, repo_path, external_repo_path)
    
    if missing_both:
        print(f"ALERT: The following active branches are missing from both local and external repositories: {missing_both}. Halting upkeep cycle.")
        sys.exit(1)
    
    if missing_locally:
        print(f"Desynchronization resolved: Fetched missing branches from external repository: {missing_locally}")
        
    print("All active branches are present or have been synchronized.")
    sys.exit(0)
>>>>>>> main
