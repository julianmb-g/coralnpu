import re
import subprocess
import os
import sys

def parse_finalized_branches(file_path):
    finalized_branches = []
    with open(file_path, 'r') as f:
        lines = f.readlines()
        for line in lines:
            # Parse table
            # Branch Name | Source Commit | Purpose | Status | Created
            if '|' in line and 'Branch Name' not in line and '---' not in line:
                parts = [p.strip() for p in line.split('|')]
                # Handle both leading/trailing pipes and no leading/trailing pipes
                if parts[0] == '':
                    # Leading pipe present: ['', BranchName, SourceCommit, Purpose, Status, Created, '']
                    if len(parts) >= 5 and "Finalized (Mutant Killed, Test Enhanced, Restored)" in parts[4]:
                        branch_name = parts[1].strip('` ')
                        finalized_branches.append(branch_name)
                else:
                    # No leading pipe: [BranchName, SourceCommit, Purpose, Status, Created]
                    if len(parts) >= 4 and "Finalized (Mutant Killed, Test Enhanced, Restored)" in parts[3]:
                        branch_name = parts[0].strip('` ')
                        finalized_branches.append(branch_name)
                    
    return list(set(finalized_branches))

def generate_patch(branches, repo_path, output_patch_path):
    with open(output_patch_path, 'w') as patch_file:
        for branch in branches:
            # Check if branch exists
            check_result = subprocess.run([
                'git', '-C', repo_path, 'rev-parse', '--verify', branch
            ], capture_output=True, text=True)
            
            if check_result.returncode != 0:
                print(f"Skipping branch {branch} (not found locally)")
                continue
                
            # Find merge base
            base_result = subprocess.run([
                'git', '-C', repo_path, 'merge-base', 'origin/main', branch
            ], capture_output=True, text=True, check=True)
            merge_base = base_result.stdout.strip()
            
            # Run git diff for each branch using merge base
            result = subprocess.run([
                'git', '-C', repo_path, 'diff', merge_base, branch, '--', 'tests/', '**/*Test.scala'
            ], capture_output=True, text=True, check=True)
            
            if result.stdout:
                patch_file.write(result.stdout)
                patch_file.write("\n") # Add newline between patches

if __name__ == "__main__":
    tracker_path = '/google/data/rw/users/ju/julianmb/wiki/projects/coralnpu-rtl-mutations/BRANCH_TRACKER.md'
    repo_path = '/usr/local/google/home/julianmb/coralnpu-rtl-mutations'
    output_patch = 'test_enhancements.patch'
    
    try:
        branches = parse_finalized_branches(tracker_path)
        if not branches:
            print("No finalized branches found.")
            sys.exit(0)
            
        print(f"Found finalized branches: {branches}")
        generate_patch(branches, repo_path, output_patch)
        print(f"Successfully generated patch: {output_patch}")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
