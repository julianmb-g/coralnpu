#!/usr/bin/env python3
"""
Unit tests for verify_doc_metadata.py.
"""

import os
import shutil
import sys
import tempfile
import unittest

# Add scripts directory to sys.path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "../../scripts")))
import verify_doc_metadata


class TestVerifyDocMetadata(unittest.TestCase):

    def setUp(self):
        self.test_dir = tempfile.mkdtemp()
        self.doc_dir = os.path.join(self.test_dir, "doc")
        os.makedirs(self.doc_dir)
        self.sha_file = os.path.join(self.test_dir, "sha.txt")
        self.sha_val = "fedcba0987654321"
        with open(self.sha_file, "w") as f:
            f.write(self.sha_val)

    def tearDown(self):
        shutil.rmtree(self.test_dir)

    def test_verify_success(self):
        """Verifies successful audit when metadata is present."""
        test_file = os.path.join(self.doc_dir, "valid.md")
        with open(test_file, "w") as f:
            f.write(f"# Valid\nDerived from upstream commit {self.sha_val}")

        self.assertTrue(verify_doc_metadata.verify_metadata(self.doc_dir, self.sha_file))

    def test_verify_failure(self):
        """Verifies audit failure when metadata is missing."""
        test_file = os.path.join(self.doc_dir, "invalid.md")
        with open(test_file, "w") as f:
            f.write("# Invalid\nMissing metadata.")

        self.assertFalse(verify_doc_metadata.verify_metadata(self.doc_dir, self.sha_file))

    def test_verify_mixed(self):
        """Verifies audit failure when some files are missing metadata."""
        with open(os.path.join(self.doc_dir, "valid.md"), "w") as f:
            f.write(f"# Valid\nDerived from upstream commit {self.sha_val}")
        with open(os.path.join(self.doc_dir, "invalid.md"), "w") as f:
            f.write("# Invalid")

        self.assertFalse(verify_doc_metadata.verify_metadata(self.doc_dir, self.sha_file))


if __name__ == "__main__":
    unittest.main()
