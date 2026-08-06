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

import unittest
import sys
import os
import tempfile

# Add scripts directory to path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../scripts')))

from integrate_rtl_tests import parse_branches

class TestIntegrateRtlTests(unittest.TestCase):

    def test_parse_branches_finalized_only(self):
        # Create a temporary file with markdown table format
        with tempfile.NamedTemporaryFile(mode='w', delete=False) as f:
            f.write("""| Branch Name | Author | Status | Notes |
| :--- | :--- | :--- | :--- |
| `feature/fix-a` | dev1 | Finalized (Mutant Killed, Test Enhanced, Restored) | fix a |
| `feature/active-b` | dev2 | Active | active b |
| `feature/fix-c` | dev3 | In Progress | wip |
""")
            temp_path = f.name
        
        try:
            result = parse_branches(temp_path)
            expected = ['feature/fix-a']
            self.assertEqual(result, expected)
        finally:
            os.remove(temp_path)

if __name__ == '__main__':
    unittest.main()
