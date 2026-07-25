#!/usr/bin/env python3
"""
Unit tests for apply_doc_deltas.py.
"""

import os
import sys
import unittest
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


if __name__ == "__main__":
    unittest.main()
