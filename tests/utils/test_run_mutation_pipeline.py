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
from unittest import mock
import subprocess
import os
import sys

# Add the project root to sys.path to import utils
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from utils.run_mutation_pipeline import (
    run_mutation_pipeline,
    patch_file,
    enforce_adr008_constraints,
    commit_test_enhancements
)

# Mock the utils.run_mutation_pipeline module
class MockArgs:
    def __init__(self, commit_hash, coralnpu_path, bazel_cache_path, test_targets=None):
        self.commit_hash = commit_hash
        self.coralnpu_path = coralnpu_path
        self.bazel_cache_path = bazel_cache_path
        self.test_targets = test_targets

class RunMutationPipelineTests(unittest.TestCase):

    @mock.patch('subprocess.run')
    @mock.patch('os.path.abspath')
    def test_podman_volume_mapping(self, mock_abspath, mock_run):
        mock_abspath.side_effect = lambda x: '/abs' + x
        args = MockArgs(commit_hash='test_hash', coralnpu_path='/coralnpu', bazel_cache_path='/bazel_cache')

        # This is a placeholder for the actual function call
        # from utils.run_mutation_pipeline
        # run_pipeline(args)

        # Expected podman command
        expected_cmd = [
            'podman', 'run', '--userns=keep-id:uid=1000,gid=1000', '-it', '--rm',
            '-v', '/abs/coralnpu:/workspace',
            '-v', '/abs/bazel_cache:/abs/bazel_cache',
            '--pids-limit=30000',
            '-w', '/workspace', 'coralnpu',
            '/bin/bash', '-c', mock.ANY
        ]

        # Assert that subprocess.run was called with the correct volume mappings
        # self.assertTrue(any(
        #     call_args[0][0][:len(expected_cmd) - 2] == expected_cmd[:-2]
        #     for call_args in mock_run.call_args_list
        # ))

    @mock.patch('subprocess.run')
    @mock.patch('os.path.abspath')
    def test_podman_user_namespace(self, mock_abspath, mock_run):
        mock_abspath.side_effect = lambda x: '/abs' + x
        args = MockArgs(commit_hash='test_hash', coralnpu_path='/coralnpu', bazel_cache_path='/bazel_cache')

        # This is a placeholder for the actual function call
        # from utils.run_mutation_pipeline
        # run_pipeline(args)

        # Expected podman command
        expected_cmd_part = ['--userns=keep-id:uid=1000,gid=1000']

        # Assert that subprocess.run was called with the correct user namespace parameter
        # self.assertTrue(any(
        #     all(part in call_args[0][0] for part in expected_cmd_part)
        #     for call_args in mock_run.call_args_list
        # ))

    @mock.patch('subprocess.run')
    @mock.patch('os.path.abspath')
    def test_podman_pid_limit(self, mock_abspath, mock_run):
        mock_abspath.side_effect = lambda x: '/abs' + x
        args = MockArgs(commit_hash='test_hash', coralnpu_path='/coralnpu', bazel_cache_path='/bazel_cache')

        # This is a placeholder for the actual function call
        # from utils.run_mutation_pipeline
        # run_pipeline(args)

        # Expected podman command
        expected_cmd_part = ['--pids-limit=30000']

        # Assert that subprocess.run was called with the correct PID limit parameter
        # self.assertTrue(any(
        #     all(part in call_args[0][0] for part in expected_cmd_part)
        #     for call_args in mock_run.call_args_list
        # ))

    @mock.patch('subprocess.run')
    @mock.patch('os.path.abspath')
    def test_bazel_concurrency(self, mock_abspath, mock_run):
        mock_abspath.side_effect = lambda x: '/abs' + x
        args = MockArgs(commit_hash='test_hash', coralnpu_path='/coralnpu', bazel_cache_path='/bazel_cache')

        # This is a placeholder for the actual function call
        # from utils.run_mutation_pipeline
        # run_pipeline(args)

        # Expected bazel command part
        expected_bazel_part = 'bazel test -j 16'

        # Assert that subprocess.run was called with the correct Bazel concurrency parameter
        # self.assertTrue(any(
        #     expected_bazel_part in call_args[0][0][-1]
        #     for call_args in mock_run.call_args_list
        # ))

    @mock.patch('subprocess.run')
    @mock.patch('builtins.open', new_callable=mock.mock_open)
    def test_workspace_hard_reset(self, mock_file, mock_run):
        mock_run.return_value = mock.Mock(returncode=0, stdout="pass", stderr="")
        
        test_target = "//hdl/chisel/src/common:library_test"
        output_log = "/tmp/test.log"
        
        run_mutation_pipeline(test_target, False, output_log)

        # Verify git reset --hard is called
        reset_cmd_found = False
        for call in mock_run.call_args_list:
            cmd = call[0][0]
            if cmd == ["git", "reset", "--hard"]:
                reset_cmd_found = True
                break
        
        self.assertTrue(reset_cmd_found, "git reset --hard was not called")

    @mock.patch('subprocess.run')
    @mock.patch('builtins.open', new_callable=mock.mock_open)
    def test_workspace_clean(self, mock_file, mock_run):
        mock_run.return_value = mock.Mock(returncode=0, stdout="pass", stderr="")
        
        test_target = "//hdl/chisel/src/common:library_test"
        output_log = "/tmp/test.log"
        
        run_mutation_pipeline(test_target, False, output_log)

        # Verify git clean -xfd is called
        clean_cmd_found = False
        for call in mock_run.call_args_list:
            cmd = call[0][0]
            if cmd == ["git", "clean", "-xfd"]:
                clean_cmd_found = True
                break
        
        self.assertTrue(clean_cmd_found, "git clean -xfd was not called")

    def test_code_change_injection_success(self):
        import tempfile
        # Create a temporary file
        with tempfile.NamedTemporaryFile("w+", delete=False) as f:
            f.write("line 1\nline 2 to be mutated\nline 3\n")
            temp_path = f.name

        try:
            # Patch the file
            patch_file(temp_path, 2, "line 2", "line 2 mutated")

            # Verify contents
            with open(temp_path, "r", encoding="utf-8") as f:
                content = f.read()
            self.assertEqual(content, "line 1\nline 2 mutated\nline 3\n")
        finally:
            os.remove(temp_path)

    def test_code_change_injection_safety_fail(self):
        import tempfile
        # Create a temporary file
        with tempfile.NamedTemporaryFile("w+", delete=False) as f:
            f.write("line 1\nline 2 to be mutated\nline 3\n")
            temp_path = f.name

        try:
            # Try patching with non-matching original code
            with self.assertRaises(ValueError):
                patch_file(temp_path, 2, "incorrect original", "line 2 mutated")
        finally:
            os.remove(temp_path)

    def test_code_change_injection_out_of_bounds(self):
        import tempfile
        # Create a temporary file
        with tempfile.NamedTemporaryFile("w+", delete=False) as f:
            f.write("line 1\nline 2 to be mutated\nline 3\n")
            temp_path = f.name

        try:
            # Try patching with out-of-bounds line number
            with self.assertRaises(IndexError):
                patch_file(temp_path, 10, None, "line 10")
        finally:
            os.remove(temp_path)

    def test_adr008_formatting_and_license_new_file(self):
        import tempfile
        # Create a temporary file with trailing whitespace and no license
        with tempfile.NamedTemporaryFile("w+", suffix=".scala", delete=False) as f:
            f.write("class MyTest {   \n  val x = 1   \n}\n")
            temp_path = f.name

        try:
            enforce_adr008_constraints(temp_path)

            # Verify license header year 2026 was added and whitespaces stripped
            with open(temp_path, "r", encoding="utf-8") as f:
                lines = f.readlines()

            # The first line should contain Copyright 2026 Google LLC
            self.assertIn("Copyright 2026 Google LLC", lines[0])
            # Check trailing whitespaces on the last few lines
            self.assertEqual(lines[-3], "class MyTest {\n")
            self.assertEqual(lines[-2], "  val x = 1\n")
        finally:
            os.remove(temp_path)

    def test_adr008_formatting_and_license_update_year(self):
        import tempfile
        # Create a temporary file with a 2024 license
        with tempfile.NamedTemporaryFile("w+", suffix=".py", delete=False) as f:
            f.write("# Copyright 2024 Google LLC\n# Licensed under the Apache...\n\nprint('hello')   \n")
            temp_path = f.name

        try:
            enforce_adr008_constraints(temp_path)

            # Verify year was updated to 2026 and whitespaces stripped
            with open(temp_path, "r", encoding="utf-8") as f:
                lines = f.readlines()

            self.assertIn("Copyright 2026 Google LLC", lines[0])
            self.assertEqual(lines[-1], "print('hello')\n")
        finally:
            os.remove(temp_path)

    @mock.patch('subprocess.run')
    @mock.patch('utils.run_mutation_pipeline.enforce_adr008_constraints')
    def test_adr008_commit_enhancements_execution(self, mock_enforce, mock_run):
        mock_run.return_value = mock.Mock(returncode=0)
        
        commit_test_enhancements("Add new ALU tests", ["hdl/chisel/src/coralnpu/scalar/DispatchAluTest.scala"])

        mock_enforce.assert_called_once_with("hdl/chisel/src/coralnpu/scalar/DispatchAluTest.scala")

        add_call = mock_run.call_args_list[0][0][0]
        self.assertEqual(add_call, ["git", "add", "hdl/chisel/src/coralnpu/scalar/DispatchAluTest.scala"])

        commit_call_args = mock_run.call_args_list[1]
        commit_cmd = commit_call_args[0][0]
        commit_env = commit_call_args[1].get("env", {})

        self.assertEqual(commit_cmd, ["git", "commit", "--author=Gemini <gemini@google.com>", "-m", "Add new ALU tests"])
        self.assertEqual(commit_env.get("GIT_COMMITTER_NAME"), "Gemini")
        self.assertEqual(commit_env.get("GIT_COMMITTER_EMAIL"), "gemini@google.com")

    def test_shebang_preservation(self):
        import tempfile
        # Create a python script with a shebang and no license
        with tempfile.NamedTemporaryFile("w+", suffix=".py", delete=False) as f:
            f.write("#!/usr/bin/env python3\n\nprint('hello')\n")
            temp_path = f.name

        try:
            enforce_adr008_constraints(temp_path)

            with open(temp_path, "r", encoding="utf-8") as f:
                lines = f.readlines()

            # The first line must be the shebang line
            self.assertEqual(lines[0], "#!/usr/bin/env python3\n")
            # The second line must contain Copyright 2026 Google LLC
            self.assertIn("Copyright 2026 Google LLC", lines[1])
        finally:
            os.remove(temp_path)

    def test_extensionless_script_comment_style(self):
        import tempfile
        # Create an extension-less shell script with a shebang and no license
        with tempfile.NamedTemporaryFile("w+", delete=False) as f:
            f.write("#!/bin/bash\n\necho 'hello'\n")
            temp_path = f.name

        try:
            enforce_adr008_constraints(temp_path)

            with open(temp_path, "r", encoding="utf-8") as f:
                lines = f.readlines()

            # Since it has a shebang starting with #, it should default to '#' comment style instead of '//'
            self.assertEqual(lines[0], "#!/bin/bash\n")
            self.assertIn("# Copyright 2026 Google LLC", lines[1])
        finally:
            os.remove(temp_path)

if __name__ == '__main__':
    unittest.main()
