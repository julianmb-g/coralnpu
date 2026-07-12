# BUILD file for RVVI libraries

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "rvvi_libs",
    srcs = [
        "trace_daemon.cc",
    ],
    hdrs = [
        "mpact_trace_formatter.h",
        "spsc_ring_buffer.h",
        "trace_daemon.h",
        "trace_formatter_interface.h",
        "trace_packet.h",
    ],
    deps = [
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/log",
        "@com_google_absl//absl/strings",
    ],
)
cc_test(
    name = "spsc_ring_buffer_test",
    srcs = ["spsc_ring_buffer_test.cc"],
    deps = [
        ":rvvi_libs",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "trace_daemon_test",
    srcs = ["trace_daemon_test.cc"],
    deps = [
        ":rvvi_libs",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "mpact_trace_formatter_test",
    srcs = ["mpact_trace_formatter_test.cc"],
    deps = [
        ":rvvi_libs",
        "@com_google_googletest//:gtest_main",
    ],
)
