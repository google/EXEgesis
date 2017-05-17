# Description:
#   Libraries for computing itineraries.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

# A library that decomposes instructions into micro-operations based on measurements made
# using the performance counters.
cc_library(
    name = "decomposition",
    srcs = ["decomposition.cc"],
    hdrs = ["decomposition.h"],
    deps = [
        "//base",
        "//exegesis/base:cpu_model",
        "//exegesis/base:port_mask",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//strings",
        "//util/gtl:container_algorithm",
        "//util/gtl:map_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
        "@glog_git//:glog",
        "@or_tools_git//ortools/linear_solver:linear_solver",
    ],
)

cc_test(
    name = "decomposition_test",
    size = "medium",
    timeout = "long",
    srcs = ["decomposition_test.cc"],
    deps = [
        ":decomposition",
        "//exegesis/testing:test_util",
        "//exegesis/x86:microarchitectures",
        "//strings",
        "//util/task:status",
        "@com_google_protobuf//:protobuf",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# A library using perf on JIT-generated assembly code.
cc_library(
    name = "jit_perf_evaluator",
    srcs = ["jit_perf_evaluator.cc"],
    hdrs = ["jit_perf_evaluator.h"],
    deps = [
        "//base",
        "//exegesis/itineraries:perf_subsystem",
        "//exegesis/llvm:inline_asm",
        "//exegesis/x86:cpu_state",
        "//strings",
        "//util/gtl:map_util",
        "//util/task:status",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_protobuf//:protobuf_lite",
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
        "//exegesis/itineraries:perf_subsystem",
        "//exegesis/testing:test_util",
        "//strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_protobuf//:protobuf_lite",
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
        "//exegesis/base:cpu_model",
        "//exegesis/base:host_cpu",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//exegesis/x86:microarchitectures",
        "//strings",
        "//util/gtl:map_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
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
        "@com_github_gflags_gflags//:gflags",
        "@com_google_protobuf//:protobuf_lite",
        "@glog_git//:glog",
        "@googletest_git//:gtest_main",
    ],
)
