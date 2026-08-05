import re
import os

def parse_branches(file_path):
    """Parses BRANCH_TRACKER.md to find finalized branches."""
    finalized_branches = []
    if not os.path.exists(file_path):
        return []
        
    with open(file_path, 'r') as f:
        lines = f.readlines()
        
        current_branch = None
        for line in lines:
            line = line.strip()
            if line.startswith('Branch:'):
                current_branch = line.split(':', 1)[1].strip()
            elif line.startswith('Status:') and current_branch:
                status = line.split(':', 1)[1].strip()
                if 'Finalized (Mutant Killed, Test Enhanced, Restored)' in status:
                    finalized_branches.append(current_branch)
                current_branch = None
                    
    return finalized_branches

def filter_test_paths(paths):
    """Filters paths to include only test-related files."""
    test_extensions = ('.scala', '.py', '.cc', '.v', '.sv', 'BUILD', 'BUILD.bazel')
    # Filter for tests/, tb/, BUILD, BUILD.bazel and relevant extensions
    filtered = []
    for path in paths:
        # Include if in tests/ or tb/ OR if it is a BUILD file
        if path.startswith('tests/') or path.startswith('tb/') or path == 'BUILD' or path == 'BUILD.bazel':
            # Check extensions, but also keep BUILD files
            if path.endswith(test_extensions) or path == 'BUILD' or path == 'BUILD.bazel':
                filtered.append(path)
    return filtered

def preserve_subdir_structure(input_path):
    """Translates path from tests/ to src/ while preserving structure."""
    if input_path.startswith('tests/'):
        return 'src/' + input_path[len('tests/'):]
    return input_path
