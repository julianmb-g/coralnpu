import re

def parse_active_branches(file_path):
    active_branches = []
    with open(file_path, 'r') as f:
        lines = f.readlines()
        for line in lines:
            # Parse bullet points
            match = re.search(r'\[ \] (.*) \[STATUS: ACTIVE\]', line)
            if match:
                # Clean up the branch name from the bullet point entry
                branch_name = match.group(1).split('(')[0].strip()
                active_branches.append(branch_name)
            
            # Parse table
            # Branch Name | Source Commit | Purpose | Status | Created
            if '|' in line and 'Branch Name' not in line and '---' not in line:
                parts = [p.strip() for p in line.split('|')]
                # Parts: ['', BranchName, SourceCommit, Purpose, Status, Created, '']
                if len(parts) >= 5 and "Active" in parts[4]:
                    branch_name = parts[1].strip('` ')
                    active_branches.append(branch_name)
                    
    return list(set(active_branches))

if __name__ == "__main__":
    branches = parse_active_branches('/google/data/rw/users/ju/julianmb/wiki/projects/coralnpu-rtl-mutations/BRANCH_TRACKER.md')
    print(branches)
