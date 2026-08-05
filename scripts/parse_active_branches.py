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

import re
import os

def parse_active_branches(file_path):
    """Parses BRANCH_TRACKER.md to find active branches."""
    active_branches = []
    if not os.path.exists(file_path):
        return []
        
    with open(file_path, 'r') as f:
        lines = f.readlines()
        for line in lines:
            # Parse bullet points: - [ ] branch_name [STATUS: ACTIVE]
            match = re.search(r'\[ \] (.*) \[STATUS: ACTIVE\]', line)
            if match:
                # Clean up the branch name from the bullet point entry
                branch_name = match.group(1).split('(')[0].strip()
                active_branches.append(branch_name)
            
            # Parse table
            # | Branch Name | Source Commit | Purpose | Status | Created |
            if '|' in line and 'Branch Name' not in line and '---' not in line:
                parts = [p.strip() for p in line.split('|')]
                # Parts: ['', BranchName, SourceCommit, Purpose, Status, Created, '']
                if len(parts) >= 5 and "Active" in parts[4]:
                    branch_name = parts[1].strip('` ')
                    active_branches.append(branch_name)
                    
    return list(set(active_branches))

if __name__ == "__main__":
    # Default to the known branch tracker path if not provided
    tracker_path = os.getenv('BRANCH_TRACKER_PATH', '/google/data/rw/users/ju/julianmb/wiki/projects/coralnpu-rtl-mutations/BRANCH_TRACKER.md')
    branches = parse_active_branches(tracker_path)
    print(branches)
