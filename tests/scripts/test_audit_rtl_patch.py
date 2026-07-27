import unittest
import sys
import os

# Add scripts directory to path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../scripts')))

try:
    from audit_rtl_patch import audit_patch
except ImportError:
    pass

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
""")
        
        from audit_rtl_patch import audit_patch
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
diff --git a/tests/test.py b/tests/test.py
""")
        
        from audit_rtl_patch import audit_patch
        invalid_files = audit_patch(temp_patch)
        self.assertIn('src/main.py', invalid_files)
        os.remove(temp_patch)

if __name__ == '__main__':
    unittest.main()
