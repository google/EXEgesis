# Description:
#   Libraries for computing itineraries.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

# A library using perf on JIT-generated assembly code.
cc_library(
    name = "jit_perf_evaluator",
    srcs = ["jit_perf_evaluator.cc"],
    hdrs = ["jit_perf_evaluator.h"],
    deps = [
        "//base",
        "//cpu_instructions/itineraries:perf_subsystem",
        "//cpu_instructions/llvm:inline_asm",
        "//cpu_instructions/x86:cpu_state",
        "//strings",
        "//util/gtl:map_util",
        "//util/task:status",
        "@com_google_protobuf//:protobuf_lite",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@llvm_git//:ir",
        "@llvm_git//:support",
    ],
)

cc_test(
    name = "jit_perf_evaluator_test",
    srcs = ["jit_perf_evaluator_test.cc"],
    deps = [
        ":jit_perf_evaluator",
        "//base",
        "//cpu_instructions/itineraries:perf_subsystem",
        "//strings",
        "@com_google_protobuf//:protobuf_lite",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# A library interfacing to libpfm4.
cc_library(
    name = "perf_subsystem",
    srcs = ["perf_subsystem.cc"],
    hdrs = ["perf_subsystem.h"],
    deps = [
        "//base",
        "//cpu_instructions/base:cpu_type",
        "//cpu_instructions/base:host_cpu",
        "//cpu_instructions/proto:cpu_type_cc_proto",
        "//strings",
        "//util/gtl:map_util",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@libpfm4_git//:pfm4",
    ],
)

cc_test(
    name = "perf_subsystem_test",
    size = "small",
    srcs = ["perf_subsystem_test.cc"],
    copts = ["-O2"],  # because of clang
    deps = [
        ":perf_subsystem",
        "//base",
        "//strings",
        "@com_google_protobuf//:protobuf_lite",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@googletest_git//:gtest_main",
    ],
)
