import os


def parse_branches(file_path):
  """Parses BRANCH_TRACKER.md to find finalized branches."""
  finalized_branches = []
  if not os.path.exists(file_path):
    return []

  with open(file_path, 'r', encoding='utf-8') as f:
    lines = f.readlines()

    for line in lines:
      if '|' in line and 'Branch Name' not in line and '---' not in line:
        parts = [p.strip() for p in line.split('|')]
        if len(parts) >= 5:
          branch_name = parts[1].replace('`', '').strip()
          status = parts[3].strip()
          if 'Finalized (Mutant Killed, Test Enhanced, Restored)' in status:
            finalized_branches.append(branch_name)

  return finalized_branches


def generate_test_patch(finalized_branch, branch_repo_path):
  """Generates test patch from finalized branch."""
  # Use unique patch file names to prevent overwriting
  patch_name = finalized_branch.replace('/', '_')
  patch_path = f'{patch_name}_test.patch'
  # Shorten command line for linting
  cmd = (f'git -C {branch_repo_path} diff origin/main {finalized_branch} -- '
         f'tests/ tb/ **/BUILD **/BUILD.bazel > {patch_path}')
  os.system(cmd)
  return patch_path


def apply_translated_patch(patch_path, integration_repo_path):
  """Applies patch without path translation."""
  os.system(f'git -C {integration_repo_path} apply {patch_path}')
  # Clean up the patch file after application
  os.remove(patch_path)
  return True


if __name__ == '__main__':
  tracker = ('/google/data/rw/users/ju/julianmb/wiki/projects/'
             'coralnpu-rtl-mutations/BRANCH_TRACKER.md')
  r_path = '/usr/local/google/home/julianmb/coralnpu-rtl-mutations'
  i_repo = '/usr/local/google/home/julianmb/julianmb-g_coralnpu'

  f_branches = parse_branches(tracker)
  for b_name in f_branches:
    patch = generate_test_patch(b_name, r_path)
    apply_translated_patch(patch, i_repo)
