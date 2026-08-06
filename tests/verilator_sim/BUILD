# Copyright 2023 Google LLC
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

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "sim_libs",
    visibility = ["//visibility:public"],
    deps = [
        "@accellera_systemc//:systemc",
        "@verilator//:verilator_lib",
    ],
)

cc_library(
    name = "util",
    hdrs = [
        "fifo.h",
        "sysc_module.h",
        "sysc_tb.h",
        "util.h",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "coralnpu_if",
    hdrs = [
        "coralnpu/coralnpu_cfg.h",
        "coralnpu/core_if.h",
        "coralnpu/debug_if.h",
        "coralnpu/memory_if.h",
    ],
    defines = [
        "CORALNPU_SIMD=128",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":sim_libs",
        ":util",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "spsc_ring_buffer",
    hdrs = ["rvvi/spsc_ring_buffer.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "register_value",
    hdrs = ["rvvi/register_value.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "trace_packet",
    hdrs = ["rvvi/trace_packet.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "trace_daemon",
    hdrs = [
        "rvvi/register_value.h",
        "rvvi/trace_daemon.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":spsc_ring_buffer",
        ":trace_packet",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:span",
        "@com_google_mpact-sim//mpact/sim/util/program_loader:elf_loader",
    ],
)

cc_library(
    name = "core_mini_axi_tb",
    srcs = [
        "coralnpu/core_mini_axi_tb.cc",
        "//hdl/chisel/src/coralnpu:VRvvCoreMiniAxi_parameters.h",
    ],
    hdrs = ["coralnpu/core_mini_axi_tb.h"],
    defines = [
        "VERILATOR_MODEL=VRvvCoreMiniAxi",
    ],
    deps = [
        ":coralnpu_if",
        ":elf_adapter",
        ":sim_libs",
        ":util",
        "//hdl/chisel/src/coralnpu:rvv_core_mini_axi_cc_library",
        "//hdl/verilog:sram_backdoor",
        "//tests/systemc:Xbar",
        "//tests/systemc:instruction_trace",
        "@accellera_systemc//:systemc",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/log:check",
        "@com_google_absl//absl/log:initialize",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/strings:str_format",
        "@libsystemctlm_soc",
    ],
)

cc_binary(
    name = "core_mini_axi_sim",
    srcs = ["coralnpu/core_mini_axi_sim.cc"],
    defines = [
        "VERILATOR_MODEL=VRvvCoreMiniAxi",
    ],
    deps = [
        ":core_mini_axi_tb",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/log:initialize",
    ],
)

cc_test(
    name = "core_barebones_tb_test",
    srcs = [
        "coralnpu/core_barebones_tb_test.cc",
        "//hdl/chisel/src/coralnpu:VCoreBarebones_parameters.h",
    ],
    defines = [
        "VERILATOR_MODEL=VCoreBarebones",
    ],
    deps = [
        ":coralnpu_if",
        ":elf_adapter",
        ":sim_libs",
        ":util",
        "//hdl/chisel/src/coralnpu:core_barebones_cc_library",
        "@com_google_googletest//:gtest",
    ],
)

cc_test(
    name = "core_rvvi_tb_test",
    srcs = [
        "coralnpu/core_rvvi_tb_test.cc",
    ],
    deps = [
        ":spsc_ring_buffer",
        ":trace_daemon",
        "@com_google_googletest//:gtest_main",
        "@com_google_mpact-sim//mpact/sim/util/program_loader:elf_loader",
    ],
)

cc_binary(
    name = "core_scalar_sim",
    srcs = [
        "coralnpu/core_tb.cc",
        "@coralnpu_hw//hdl/chisel/src/coralnpu:VCoreScalar_parameters.h",
    ],
    defines = [
        "VERILATOR_MODEL=VCoreScalar",
    ],
    deps = [
        ":coralnpu_if",
        ":sim_libs",
        ":util",
        "//hdl/chisel/src/coralnpu:core_scalar_cc_library",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/flags:usage",
    ],
)

cc_binary(
    name = "core_barebones_sim",
    srcs = [
        "coralnpu/core_barebones_tb.cc",
        "//hdl/chisel/src/coralnpu:VCoreBarebones_parameters.h",
    ],
    defines = [
        "VERILATOR_MODEL=VCoreBarebones",
    ],
    deps = [
        ":coralnpu_if",
        ":elf_adapter",
        ":sim_libs",
        ":util",
        "//hdl/chisel/src/coralnpu:core_barebones_cc_library",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/flags:usage",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_binary(
    name = "core_rvvi_traced_sim",
    srcs = [
        "coralnpu/core_rvvi_traced_sim.cc",
        "//hdl/chisel/src/coralnpu:VCoreVerification_parameters.h",
    ],
    defines = [
        "VERILATOR_MODEL=VCoreVerification",
    ],
    deps = [
        ":coralnpu_if",
        ":elf_adapter",
        ":sim_libs",
        ":spsc_ring_buffer",
        ":trace_daemon",
        ":util",
        "//hdl/chisel/src/coralnpu:core_verification_cc_library",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/flags:usage",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_test(
    name = "spsc_ring_buffer_test",
    srcs = ["rvvi/spsc_ring_buffer_test.cc"],
    deps = [
        ":spsc_ring_buffer",
        ":trace_packet",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "trace_daemon_test",
    srcs = ["rvvi/trace_daemon_test.cc"],
    deps = [
        ":trace_daemon",
        "@com_google_googletest//:gtest_main",
        "@com_google_mpact-sim//mpact/sim/util/program_loader:elf_loader",
    ],
)

cc_test(
    name = "dbus2axi_test",
    srcs = ["coralnpu/dbus2axi_tb.cc"],
    deps = [
        ":sim_libs",
        ":util",
        "//hdl/chisel/src/coralnpu:dbus2axi_cc_library",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "custom_fallback_formatter_test",
    srcs = ["coralnpu/custom_fallback_formatter_test.cc"],
    deps = [
        "@com_google_absl//absl/strings:str_format",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_binary(
    name = "gen_elf",
    srcs = [
        "utils/gen_elf.cc",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "elf_adapter",
    hdrs = ["elf_loader_adapter.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":coralnpu_if",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_mpact-sim//mpact/sim/util/program_loader:elf_loader",
    ],
)
