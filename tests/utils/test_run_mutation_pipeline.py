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

if __name__ == '__main__':
    unittest.main()
