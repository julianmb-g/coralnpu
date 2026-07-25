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
import json

# Add the project root to sys.path to import utils
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from utils.run_mutation_pipeline import (
    run_mutation_pipeline,
    patch_file,
    enforce_adr008_constraints,
    commit_test_enhancements,
    evaluate_status
)

class RunMutationPipelineTests(unittest.TestCase):

    @mock.patch('subprocess.run')
    @mock.patch('os.path.abspath')
    @mock.patch('builtins.open', new_callable=mock.mock_open)
    def test_podman_bazel_test_execution(self, mock_file, mock_abspath, mock_run):
        mock_abspath.side_effect = lambda x: '/abs/workspace' if x == '.' else '/abs' + x if not x.startswith('/abs') else x
        mock_run.return_value = mock.Mock(returncode=0, stdout="pass", stderr="")
        
        test_target = "//hdl/chisel/src/common:library_test"
        output_log = "/tmp/test.log"
        
        run_mutation_pipeline(test_target, False, output_log)

        # Expected podman command part
        expected_cmd_start = ['podman', 'run', '--userns=keep-id:uid=1000,gid=1000']
        
        podman_cmd_found = False
        for call in mock_run.call_args_list:
            cmd = call[0][0]
            if cmd[:3] == expected_cmd_start:
                podman_cmd_found = True
                self.assertIn('-v', cmd)
                self.assertTrue(any('/abs/workspace' in arg for arg in cmd))
                self.assertTrue(any('bazel --output_user_root' in arg for arg in cmd))
                break
                
        self.assertTrue(podman_cmd_found, "Podman Bazel test execution was not called")

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

    def test_mapping_bazel_test_pass_to_survived_state(self):
        self.assertEqual(evaluate_status(0), "SURVIVED")

    def test_mapping_bazel_test_fail_to_killed_state(self):
        self.assertEqual(evaluate_status(3), "KILLED")
        self.assertEqual(evaluate_status(4), "KILLED")

    def test_mapping_build_error_to_invalid_state(self):
        self.assertEqual(evaluate_status(1), "INVALID")
        self.assertEqual(evaluate_status(8), "INVALID")

    @mock.patch('utils.run_mutation_pipeline.patch_file')
    @mock.patch('subprocess.run')
    @mock.patch('builtins.open', new_callable=mock.mock_open)
    def test_file_reversion_via_git_checkout(self, mock_file, mock_run, mock_patch_file):
        mock_run.return_value = mock.Mock(returncode=0, stdout="pass", stderr="")
        
        test_target = "//hdl/chisel/src/common:library_test"
        output_log = "/tmp/test.log"
        mutation_file = "src/my_file.scala"
        
        # Call with no_revert=False to ensure file reversion happens
        run_mutation_pipeline(
            test_target=test_target, 
            no_revert=False, 
            output_log=output_log, 
            mutation_file=mutation_file,
            mutation_line=1,
            original_code="old",
            mutated_code="new"
        )
        
        checkout_cmd_found = False
        for call in mock_run.call_args_list:
            cmd = call[0][0]
            if cmd == ["git", "checkout", "--", mutation_file]:
                checkout_cmd_found = True
                break
                
        self.assertTrue(checkout_cmd_found, "git checkout -- file was not called")

    @mock.patch('subprocess.run')
    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('builtins.open', new_callable=mock.mock_open)
    @mock.patch('json.load')
    def test_sequential_flow_orchestration_of_main_pipeline_loop(self, mock_json_load, mock_file, mock_exists, mock_run):
        mock_run.return_value = mock.Mock(returncode=0, stdout="pass", stderr="")
        
        targets = ["//target1:test", "//target2:test"]
        mock_json_load.return_value = targets
        
        output_log = "/tmp/test.log"
        
        # Call with no test_target to trigger the main loop over pending_mutations.json
        run_mutation_pipeline(
            test_target=None,
            no_revert=True,
            output_log=output_log
        )
        
        # Verify podman is called for each target sequentially
        podman_calls = []
        for call in mock_run.call_args_list:
            cmd = call[0][0]
            if cmd[0] == "podman":
                podman_calls.append(cmd[-1]) # the shell command
                
        self.assertEqual(len(podman_calls), 2)
        self.assertTrue("target1:test" in podman_calls[0])
        self.assertTrue("target2:test" in podman_calls[1])

if __name__ == '__main__':
    unittest.main()
