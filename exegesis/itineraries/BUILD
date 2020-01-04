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
        ":decomposition",
        ":jit_perf_evaluator",
        ":perf_subsystem",
        "//base",
        "//exegesis/base:cpu_info",
        "//exegesis/base:host_cpu",
        "//exegesis/base:microarchitecture",
        "//exegesis/base:prettyprint",
        "//exegesis/llvm:inline_asm",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:category_util",
        "//exegesis/util:instruction_syntax",
        "//exegesis/x86:cpu_state",
        "//exegesis/x86:microarchitectures",
        "//exegesis/x86:operand_translator",
        "//net/proto2/util/public:repeated_field_util",
        "//util/gtl:map_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
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
        ":compute_itineraries",
        "//exegesis/base:host_cpu",
        "//exegesis/base:microarchitecture",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/testing:test_util",
        "//exegesis/util:proto_util",
        "//exegesis/util:system",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
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
        "//util/gtl:map_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/algorithm:container",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
        "@or_tools_git//ortools/linear_solver",
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
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

# A library using perf on JIT-generated assembly code.
cc_library(
    name = "jit_perf_evaluator",
    srcs = ["jit_perf_evaluator.cc"],
    hdrs = ["jit_perf_evaluator.h"],
    deps = [
        ":perf_subsystem",
        "//base",
        "//exegesis/llvm:inline_asm",
        "//exegesis/util:strings",
        "//exegesis/x86:cpu_state",
        "//util/gtl:map_util",
        "//util/task:status",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_protobuf//:protobuf_lite",
        "@llvm_git//:ir",
    ],
)

cc_test(
    name = "jit_perf_evaluator_test",
    srcs = ["jit_perf_evaluator_test.cc"],
    deps = [
        ":jit_perf_evaluator",
        ":perf_subsystem",
        "//base",
        "//exegesis/testing:test_util",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf_lite",
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
        "//util/gtl:map_util",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/synchronization",
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
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf_lite",
    ],
)
