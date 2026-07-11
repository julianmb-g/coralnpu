# BUILD file for RVVI libraries

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "rvvi_libs",
    srcs = [
        "trace_daemon.cc",
    ],
    hdrs = [
        "custom_fallback_formatter.h",
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