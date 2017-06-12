# Contains classes, functions and data structures that are platform independent.
# Code in this directory is only allowed to depend on proto/ and util/.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "architecture_provider",
    srcs = ["architecture_provider.cc"],
    hdrs = ["architecture_provider.h"],
    deps = [
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:proto_util",
        "//strings",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "architecture_provider_test",
    size = "small",
    srcs = ["architecture_provider_test.cc"],
    deps = [
        ":architecture_provider",
        "//base",
        "//exegesis/testing:test_util",
        "//exegesis/util:proto_util",
        "//strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# A library of functions for working with categories.

cc_library(
    name = "category_utils",
    srcs = [],
    hdrs = ["category_utils.h"],
    deps = [
        "//base",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "category_utils_test",
    size = "small",
    srcs = ["category_utils_test.cc"],
    deps = [
        ":category_utils",
        "//base",
        "//strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# Framework for writing procedural clean-ups of an instruction set.
cc_library(
    name = "cleanup_instruction_set",
    srcs = ["cleanup_instruction_set.cc"],
    hdrs = ["cleanup_instruction_set.h"],
    deps = [
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//util/gtl:map_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
    ],
    alwayslink = 1,
)

cc_library(
    name = "cleanup_instruction_set_test_utils",
    testonly = 1,
    srcs = ["cleanup_instruction_set_test_utils.cc"],
    hdrs = ["cleanup_instruction_set_test_utils.h"],
    deps = [
        ":cleanup_instruction_set",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/testing:test_util",
        "@com_google_protobuf//:protobuf",
        "@googletest_git//:gtest",
    ],
)

cc_test(
    name = "cleanup_instruction_set_test",
    size = "small",
    srcs = ["cleanup_instruction_set_test.cc"],
    deps = [
        ":cleanup_instruction_set",
        ":cleanup_instruction_set_test_utils",
        "//base",
        "//util/task:status",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# A library to represent known CPU microarchitectures and models.
cc_library(
    name = "cpu_model",
    srcs = ["cpu_model.cc"],
    hdrs = ["cpu_model.h"],
    deps = [
        ":port_mask",
        "//base",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

# Utilities for working with CPUID data.
cc_library(
    name = "cpuid",
    srcs = [
        "cpuid.cc",
        "cpuid_x86.cc",
    ],
    hdrs = [
        "cpuid.h",
        "cpuid_x86.h",
    ],
    deps = [
        ":cpu_info",
        "//base",
        "//exegesis/proto:cpuid_cc_proto",
        "//exegesis/proto/x86:cpuid_cc_proto",
        "//exegesis/util:structured_register",
        "//strings",
        "//util/gtl:map_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_test(
    name = "cpuid_test",
    size = "small",
    srcs = [
        "cpuid_test.cc",
        "cpuid_x86_test.cc",
    ],
    deps = [
        ":cpu_info",
        ":cpuid",
        "//exegesis/testing:test_util",
        "//exegesis/util:proto_util",
        "//util/task:status",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# A library that represents the host CPU info.
cc_library(
    name = "cpu_info",
    srcs = ["cpu_info.cc"],
    hdrs = ["cpu_info.h"],
    deps = [
        "//base",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//strings",
        "//util/gtl:map_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "cpu_info_test",
    srcs = ["cpu_info_test.cc"],
    deps = [
        ":cpu_info",
        ":cpuid",
        "//base",
        "//exegesis/proto:cpuid_cc_proto",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//exegesis/proto/x86:cpuid_cc_proto",
        "//exegesis/util:proto_util",
        "//strings",
        "//util/task:status",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

cc_library(
    name = "microarchitecture",
    srcs = ["microarchitecture.cc"],
    hdrs = ["microarchitecture.h"],
    deps = [
        "//exegesis/base:cpu_model",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//strings",
        "//util/task:status",
        "//util/task:statusor",
    ],
)

cc_test(
    name = "microarchitecture_test",
    srcs = ["microarchitecture_test.cc"],
    deps = [
        ":microarchitecture",
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:proto_util",
        "//exegesis/x86:microarchitectures",
        "//strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# A library that provides information about the host CPU. Note that this library
# compiles only on platform where reading the host CPU is supported.
cc_library(
    name = "host_cpu",
    srcs = ["host_cpu.cc"],
    hdrs = ["host_cpu.h"],
    deps = [
        ":cpu_info",
        ":cpuid",
        "//base",
        "//exegesis/proto:cpuid_cc_proto",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "host_cpu_test",
    size = "small",
    srcs = ["host_cpu_test.cc"],
    deps = [
        ":cpu_info",
        ":host_cpu",
        "//exegesis/proto:cpuid_cc_proto",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# An efficient representation of the execution unit port mask.
cc_library(
    name = "port_mask",
    srcs = ["port_mask.cc"],
    hdrs = ["port_mask.h"],
    deps = [
        "//base",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "port_mask_test",
    size = "small",
    srcs = ["port_mask_test.cc"],
    deps = [
        ":port_mask",
        "//base",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# Utilities to prettyprint protos.
cc_library(
    name = "prettyprint",
    srcs = ["prettyprint.cc"],
    hdrs = ["prettyprint.h"],
    deps = [
        ":cpu_info",
        ":cpu_model",
        ":port_mask",
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "prettyprint_test",
    size = "small",
    srcs = ["prettyprint_test.cc"],
    deps = [
        ":prettyprint",
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//exegesis/util:proto_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# Utilities for restrict instruction set to a certain range.

cc_library(
    name = "restrict",
    srcs = ["restrict.cc"],
    hdrs = ["restrict.h"],
    deps = [
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//net/proto2/util/public:repeated_field_util",
        "//strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "restrict_test",
    size = "small",
    srcs = ["restrict_test.cc"],
    deps = [
        ":restrict",
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/testing:test_util",
        "//exegesis/util:proto_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# Factory functions for obtaining the list of instruction set transforms.
cc_library(
    name = "transform_factory",
    srcs = ["transform_factory.cc"],
    hdrs = ["transform_factory.h"],
    deps = [
        ":cleanup_instruction_set",
        "//base",
        "//strings",
        "//util/gtl:map_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "transform_factory_test",
    srcs = ["transform_factory_test.cc"],
    deps = [
        ":cleanup_instruction_set",
        ":transform_factory",
        "//base",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)
