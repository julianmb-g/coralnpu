#!/usr/bin/env python3
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

"""
Unit tests for apply_doc_deltas.py.
"""

import os
import sys
import unittest
import subprocess
from unittest.mock import patch, mock_open

# Add scripts directory to sys.path to import apply_doc_deltas
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "../../scripts")))
import apply_doc_deltas


class TestApplyDocDeltas(unittest.TestCase):

    def test_validate_patch_valid(self):
        """Verifies that a patch with only doc/ files is accepted."""
        patch_content = (
            "diff --git a/doc/index.md b/doc/index.md\n"
            "--- a/doc/index.md\n"
            "+++ b/doc/index.md\n"
            "@@ -1 +1 @@\n"
            "- Old\n"
            "+ New\n"
        )
        with patch("builtins.open", mock_open(read_data=patch_content)):
            self.assertTrue(apply_doc_deltas.validate_patch("valid.patch"))

    def test_validate_patch_invalid_root(self):
        """Verifies that a patch with files in the root is rejected."""
        patch_content = "diff --git a/README.md b/README.md\n"
        with patch("builtins.open", mock_open(read_data=patch_content)):
            self.assertFalse(apply_doc_deltas.validate_patch("invalid.patch"))

    def test_validate_patch_invalid_subdir(self):
        """Verifies that a patch with files in other subdirectories is rejected."""
        patch_content = "diff --git a/src/main.cc b/src/main.cc\n"
        with patch("builtins.open", mock_open(read_data=patch_content)):
            self.assertFalse(apply_doc_deltas.validate_patch("invalid.patch"))

    def test_validate_patch_mixed(self):
        """Verifies that a mixed patch is rejected."""
        patch_content = (
            "diff --git a/doc/index.md b/doc/index.md\n"
            "diff --git a/hdl/top.v b/hdl/top.v\n"
        )
        with patch("builtins.open", mock_open(read_data=patch_content)):
            self.assertFalse(apply_doc_deltas.validate_patch("mixed.patch"))

    @patch("os.path.isfile", return_value=True)
    @patch("apply_doc_deltas.validate_patch", return_value=True)
    @patch("subprocess.run")
    def test_apply_patch_fallback_checkout_success(self, mock_run, mock_validate, mock_isfile):
        """Verifies fallback via direct checkout succeeds."""
        mock_run.side_effect = [
            subprocess.CalledProcessError(1, "git apply"),
            None,  # git fetch
            None,  # git checkout
        ]
        
        apply_doc_deltas.apply_patch("dummy.patch", "/dummy/ext")
        
        # Assert fallback fetch and final checkout of doc/ were called
        mock_run.assert_any_call(["git", "fetch", "/dummy/ext", "main"], check=True)
        mock_run.assert_any_call(["git", "checkout", "FETCH_HEAD", "--", "doc/"], check=True)


if __name__ == "__main__":
    unittest.main()
