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

def get_podman_command(cmd, repo_path):
    user = os.environ.get('USER', 'julianmb')
    return [
        'podman', 'run', '--userns=keep-id:uid=1000,gid=1000', '--pids-limit=-1', '-it', '--rm', 
        '-v', f"{repo_path}:{repo_path}", '-w', repo_path,
        'localhost/coralnpu-validation', 'bash', '-c', 
        f"git config --global --add safe.directory {repo_path} && {cmd}"
    ]

def verify_active_branches_build(tracker_path, repo_path=None):
    """Verifies that all active branches build and pass tests inside Podman."""
    active_branches = parse_active_branches(tracker_path)
    
    if repo_path is None:
        repo_path = os.getenv('INTEGRATION_REPO_PATH', os.getcwd())
    
    # Save current branch
    result = subprocess.run(['git', '-C', repo_path, 'branch', '--show-current'], capture_output=True, text=True)
    original_branch = result.stdout.strip()
    
    failures = []
    
    try:
        for branch in active_branches:
            print(f"Verifying build/test on branch: {branch}")
            checkout_res = subprocess.run(['git', '-C', repo_path, 'checkout', branch], capture_output=True, text=True)
            if checkout_res.returncode != 0:
                print(f"Failed to checkout {branch}: {checkout_res.stderr}")
                failures.append((branch, 'checkout'))
                continue
            
            # 1. Bazel Build
            print(f"  Running bazel build //... on {branch}...")
            build_cmd = get_podman_command('bazel build //...', repo_path)
            if subprocess.run(build_cmd).returncode != 0:
                print(f"  Build failed on {branch}")
                failures.append((branch, 'build'))
                continue
                
            # 2. Bazel Test
            print(f"  Running bazel test //... on {branch}...")
            test_cmd = get_podman_command('bazel test //...', repo_path)
            if subprocess.run(test_cmd).returncode != 0:
                print(f"  Tests failed on {branch}")
                failures.append((branch, 'test'))
                continue
                
            # 3. Legacy SoC Test (ADR-004)
            print(f"  Running bazel test //fpga:coralnpu_soc_test on {branch}...")
            soc_cmd = get_podman_command('bazel test //fpga:coralnpu_soc_test', repo_path)
            if subprocess.run(soc_cmd).returncode != 0:
                print(f"  Legacy SoC test failed on {branch}")
                failures.append((branch, 'soc_test'))
    finally:
        # Restore original branch
        if original_branch:
            print(f"Restoring original branch: {original_branch}")
            subprocess.run(['git', '-C', repo_path, 'checkout', original_branch], capture_output=True, text=True)
        
    return failures

if __name__ == "__main__":
    tracker_path = os.getenv('BRANCH_TRACKER_PATH', '/google/data/rw/users/ju/julianmb/wiki/projects/coralnpu-rtl-mutations/BRANCH_TRACKER.md')
    repo_path = os.getenv('INTEGRATION_REPO_PATH', os.getcwd())
    
    failures = verify_active_branches_build(tracker_path, repo_path)
    if failures:
        print(f"Build/Test failures detected: {failures}")
        sys.exit(1)
    else:
        print("All active branches passed verification.")
        sys.exit(0)
