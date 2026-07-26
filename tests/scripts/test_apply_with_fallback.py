import unittest
import subprocess
import os
from unittest.mock import patch

class TestApplyWithFallback(unittest.TestCase):
    def test_apply_patch_success(self):
        # Mock subprocess.run to simulate a successful git apply
        with patch('subprocess.run') as mock_run:
            mock_run.return_value = subprocess.CompletedProcess(args=[], returncode=0, stdout="Patch applied\\n", stderr="")
            from scripts import apply_with_fallback
            success, stdout, stderr = apply_with_fallback.apply_patch("test.patch", ".")
            self.assertTrue(success)
            mock_run.assert_called_once_with(["git", "apply", "--index", "test.patch"], cwd=".", capture_output=True, text=True, check=False)

    def test_apply_patch_failure(self):
        # Mock subprocess.run to simulate a failed git apply
        with patch('subprocess.run') as mock_run:
            mock_run.return_value = subprocess.CompletedProcess(args=[], returncode=1, stdout="", stderr="Patch failed
")
            from scripts import apply_with_fallback
            success, stdout, stderr = apply_with_fallback.apply_patch("test.patch", ".")
            self.assertFalse(success)
            mock_run.assert_called_once_with(["git", "apply", "--index", "test.patch"], cwd=".", capture_output=True, text=True, check=False)

if __name__ == "__main__":
    unittest.main()
