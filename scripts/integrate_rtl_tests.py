import re
import os

def parse_branches(file_path):
    """Parses BRANCH_TRACKER.md to find finalized branches from Markdown table."""
    finalized_branches = []
    if not os.path.exists(file_path):
        return []

    with open(file_path, 'r') as f:
        lines = f.readlines()

        for line in lines:
            if '|' in line and 'Branch Name' not in line and '---' not in line:
                parts = [p.strip() for p in line.split('|')]
                if len(parts) >= 5:
                    branch = parts[1].replace('`', '').strip()
                    status = parts[3].strip()
                    if 'Finalized (Mutant Killed, Test Enhanced, Restored)' in status:
                        finalized_branches.append(branch)

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

def generate_test_patch(finalized_branch, repo_path):
    """Generates test patch from finalized branch."""
    patch_path = "rtl_mutation_test.patch"
    # Use git diff to generate patch, filtering for test files as per PLAN.md
    cmd = f"git -C {repo_path} diff origin/main {finalized_branch} -- 'tests/' 'tb/' '**/BUILD' '**/BUILD.bazel' > {patch_path}"
    os.system(cmd)
    return patch_path

def apply_translated_patch(patch_path, repo_path):
    """Applies patch with translated paths."""
    # Translate paths from tests/ to src/
    with open(patch_path, 'r') as f:
        content = f.read()
    
    # 1:1 path translation from tests/ to src/
    content = content.replace('tests/', 'src/')
    
    translated_patch = "translated_" + patch_path
    with open(translated_patch, 'w') as f:
        f.write(content)
        
    os.system(f"git -C {repo_path} apply {translated_patch}")
    return True

if __name__ == "__main__":
    # Integration workflow
    tracker_path = "/google/data/rw/users/ju/julianmb/wiki/projects/coralnpu-rtl-mutations/BRANCH_TRACKER.md"
    repo_path = "/usr/local/google/home/julianmb/coralnpu-rtl-mutations"
    integration_repo = "/usr/local/google/home/julianmb/julianmb-g_coralnpu"
    
    finalized_branches = parse_branches(tracker_path)
    for branch in finalized_branches:
        patch = generate_test_patch(branch, repo_path)
        apply_translated_patch(patch, integration_repo)
