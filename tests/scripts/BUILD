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

load("@rules_python//python:defs.bzl", "py_test")

py_test(
    name = "test_branch_tracker",
    srcs = ["test_branch_tracker.py"],
)

py_test(
    name = "test_verify_active_branches",
    srcs = ["test_verify_active_branches.py"],
    deps = ["//scripts:verify_active_branches"],
)

py_test(
    name = "test_sync_active_branches",
    srcs = ["test_sync_active_branches.py"],
    deps = ["//scripts:sync_active_branches"],
)

py_test(
    name = "test_verify_active_branches_build",
    srcs = ["test_verify_active_branches_build.py"],
    deps = ["//scripts:verify_active_branches_build"],
)

py_test(
    name = "test_audit_rtl_patch",
    srcs = ["test_audit_rtl_patch.py"],
    deps = ["//scripts:audit_rtl_patch"],
)

py_test(
    name = "test_integrate_rtl_tests",
    srcs = ["test_integrate_rtl_tests.py"],
    deps = ["//scripts:integrate_rtl_tests"],
)
