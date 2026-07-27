#!/usr/bin/env python3
import unittest
import subprocess
from unittest.mock import patch, MagicMock
import os
import sys

# Add the scripts directory to the system path to allow importing apply_with_fallback
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'scripts')))
import apply_with_fallback

class TestApplyWithFallback(unittest.TestCase):

    @patch('subprocess.run')
    def test_apply_patch_success(self, mock_run):
        # Simulate successful git apply
        mock_run.return_value = MagicMock(stdout="Patch applied successfully", stderr="", returncode=0)
        self.assertTrue(apply_with_fallback.apply_patch_with_fallback("test.patch"))
        mock_run.assert_called_once_with(
            ["git", "apply", "--reject", "--whitespace=nowarn", "test.patch"],
            check=True,
            capture_output=True,
            text=True
        )

    @patch('subprocess.run')
    def test_apply_patch_failure_and_fallback(self, mock_run):
        # Simulate git apply failure
        mock_run.side_effect = subprocess.CalledProcessError(1, "git apply", stderr="Patch failed to apply")
        self.assertFalse(apply_with_fallback.apply_patch_with_fallback("test.patch"))
        mock_run.assert_called_once_with(
            ["git", "apply", "--reject", "--whitespace=nowarn", "test.patch"],
            check=True,
            capture_output=True,
            text=True
        )
        # Verify that fallback message is printed (we can't directly test print, but we can verify the function returns False)

    @patch('subprocess.run')
    def test_apply_patch_failure_with_fetch_fallback(self, mock_run):
        # Simulate git apply failure
        mock_run.side_effect = [
            subprocess.CalledProcessError(1, "git apply", stderr="Patch failed to apply"),
            MagicMock(stdout="Fetch successful", stderr="", returncode=0)
        ]
        external_path = "/tmp/external_repo"
        self.assertFalse(apply_with_fallback.apply_patch_with_fallback("test.patch", external_repo_path=external_path))

        # Verify git apply was called
        self.assertEqual(mock_run.call_args_list[0].args[0], ["git", "apply", "--reject", "--whitespace=nowarn", "test.patch"])
        self.assertEqual(mock_run.call_args_list[0].kwargs, {'check': True, 'capture_output': True, 'text': True})

        # Verify git fetch was called as fallback
        self.assertEqual(mock_run.call_args_list[1].args[0], ["git", "-C", external_path, "fetch", "origin"])
        self.assertEqual(mock_run.call_args_list[1].kwargs, {'check': True, 'capture_output': True, 'text': True})

if __name__ == "__main__":
    unittest.main()
