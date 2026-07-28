#!/usr/bin/env python3
import unittest
import subprocess
from unittest.mock import patch, MagicMock
import os
import sys

# Add the scripts directory to the system path to allow importing apply_rtl_test_patches
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'scripts')))

try:
    import apply_rtl_test_patches
except ImportError:
    apply_rtl_test_patches = None

class TestApplyRtlTestPatches(unittest.TestCase):

    def setUp(self):
        self.assertIsNotNone(apply_rtl_test_patches, "apply_rtl_test_patches module not found (TDD)")

    @patch('subprocess.run')
    def test_apply_patch_success(self, mock_run):
        mock_run.return_value = MagicMock(stdout="Patch applied successfully", stderr="", returncode=0)
        self.assertTrue(apply_rtl_test_patches.apply_patches("test.patch"))
        mock_run.assert_called_once_with(
            ["git", "apply", "--index", "--whitespace=nowarn", "test.patch"],
            check=True,
            capture_output=True,
            text=True
        )

    @patch('subprocess.run')
    def test_apply_patch_failure(self, mock_run):
        mock_run.side_effect = subprocess.CalledProcessError(1, "git apply", stderr="Patch failed to apply")
        self.assertFalse(apply_rtl_test_patches.apply_patches("test.patch"))
        mock_run.assert_called_once_with(
            ["git", "apply", "--index", "--whitespace=nowarn", "test.patch"],
            check=True,
            capture_output=True,
            text=True
        )

    @patch('subprocess.run')
    def test_apply_patch_failure_with_fetch_fallback(self, mock_run):
        mock_run.side_effect = [
            subprocess.CalledProcessError(1, "git apply", stderr="Patch failed to apply"),
            MagicMock(stdout="Fetch successful", stderr="", returncode=0)
        ]
        external_path = "/usr/local/google/home/julianmb/coralnpu-rtl-mutations"
        self.assertFalse(apply_rtl_test_patches.apply_patches("test.patch", external_repo_path=external_path))
        
        self.assertEqual(mock_run.call_args_list[0].args[0], ["git", "apply", "--index", "--whitespace=nowarn", "test.patch"])
        self.assertEqual(mock_run.call_args_list[1].args[0], ["git", "-C", external_path, "fetch", "origin"])

if __name__ == "__main__":
    unittest.main()
