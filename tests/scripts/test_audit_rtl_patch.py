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

# Add scripts directory to path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../scripts')))

from audit_rtl_patch import audit_patch

class TestAuditRtlPatch(unittest.TestCase):
    def test_audit_patch_valid(self):
        temp_patch = 'temp_valid.patch'
        with open(temp_patch, 'w') as f:
            f.write("""diff --git a/tests/test.py b/tests/test.py
index 123..456 100644
--- a/tests/test.py
+++ b/tests/test.py
@@ -1 +1 @@
-a
+b
diff --git a/src/deep/dir/MyTest.scala b/src/deep/dir/MyTest.scala
index 123..456 100644
--- a/src/deep/dir/MyTest.scala
+++ b/src/deep/dir/MyTest.scala
diff --git a/tb/my_tb.sv b/tb/my_tb.sv
index 123..456 100644
--- a/tb/my_tb.sv
+++ b/tb/my_tb.sv
diff --git a/hdl/BUILD b/hdl/BUILD
index 123..456 100644
--- a/hdl/BUILD
+++ b/hdl/BUILD
diff --git a/hdl/BUILD.bazel b/hdl/BUILD.bazel
index 123..456 100644
--- a/hdl/BUILD.bazel
+++ b/hdl/BUILD.bazel
""")
        
        invalid_files = audit_patch(temp_patch)
        self.assertEqual(len(invalid_files), 0)
        os.remove(temp_patch)

    def test_audit_patch_invalid(self):
        temp_patch = 'temp_invalid.patch'
        with open(temp_patch, 'w') as f:
            f.write("""diff --git a/src/main.py b/src/main.py
index 123..456 100644
--- a/src/main.py
+++ b/src/main.py
diff --git a/hdl/top.sv b/hdl/top.sv
index 123..456 100644
--- a/hdl/top.sv
+++ b/hdl/top.sv
""")
        
        invalid_files = audit_patch(temp_patch)
        self.assertIn('src/main.py', invalid_files)
        self.assertIn('hdl/top.sv', invalid_files)
        os.remove(temp_patch)

if __name__ == '__main__':
    unittest.main()
