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

import os
import sys
from audit_rtl_patch import audit_patch


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
  """Applies patch with path translation after auditing."""
  invalid = audit_patch(patch_path)
  if invalid:
    print(f"Error: Audit failed for {patch_path}. Found invalid files:")
    for f in invalid:
      print(f"  {f}")
    return False

  os.system(f'git -C {integration_repo_path} apply -p1 --ignore-space-change --ignore-whitespace {patch_path}')
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
    if not apply_translated_patch(patch, i_repo):
        print(f"Failed to apply patch for branch {b_name}")
        sys.exit(1)
