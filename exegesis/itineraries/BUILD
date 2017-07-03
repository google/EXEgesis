# Description:
#   Libraries for computing itineraries.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

# A library to compute itineraries for an instruction set.
cc_library(
    name = "compute_itineraries",
    srcs = ["compute_itineraries.cc"],
    hdrs = ["compute_itineraries.h"],
    deps = [
        "//base",
        "//exegesis/base:category_utils",
        "//exegesis/base:cpu_info",
        "//exegesis/base:host_cpu",
        "//exegesis/base:microarchitecture",
        "//exegesis/base:prettyprint",
        "//exegesis/itineraries:decomposition",
        "//exegesis/itineraries:jit_perf_evaluator",
        "//exegesis/itineraries:perf_subsystem",
        "//exegesis/llvm:inline_asm",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//exegesis/x86:cpu_state",
        "//exegesis/x86:microarchitectures",
        "//exegesis/x86:operand_translator",
        "//net/proto2/util/public:repeated_field_util",
        "//strings",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
        "@llvm_git//:ir",
        "@llvm_git//:support",
    ],
)

cc_test(
    name = "compute_itineraries_test",
    srcs = ["compute_itineraries_test.cc"],
    tags = ["notap"],
    deps = [
        "//exegesis/base:host_cpu",
        "//exegesis/base:microarchitecture",
        "//exegesis/itineraries:compute_itineraries",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/testing:test_util",
        "//exegesis/util:proto_util",
        "//exegesis/util:system",
        "//util/task:status",
        "@com_google_protobuf//:protobuf",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# A library that decomposes instructions into micro-operations based on measurements made
# using the performance counters.
cc_library(
    name = "decomposition",
    srcs = ["decomposition.cc"],
    hdrs = ["decomposition.h"],
    deps = [
        "//base",
        "//exegesis/base:microarchitecture",
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
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
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
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
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
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
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
        "//exegesis/base:host_cpu",
        "//exegesis/base:microarchitecture",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//exegesis/x86:microarchitectures",
        "//strings",
        "//util/gtl:map_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
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
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@googletest_git//:gtest_main",
    ],
)
