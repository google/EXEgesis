# Contains classes, functions and data structures that are platform independent.
# Code in this directory is only allowed to depend on proto/ and util/.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

# Framework for writing procedural clean-ups of an instruction set.
cc_library(
    name = "cleanup_instruction_set",
    srcs = ["cleanup_instruction_set.cc"],
    hdrs = ["cleanup_instruction_set.h"],
    deps = [
        "//base",
        "//cpu_instructions/proto:instructions_cc_proto",
        "//util/gtl:map_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
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
        "//cpu_instructions/proto:instructions_cc_proto",
        "//cpu_instructions/testing:test_util",
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
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# A library to represent known CPU types.
cc_library(
    name = "cpu_type",
    srcs = ["cpu_type.cc"],
    hdrs = ["cpu_type.h"],
    deps = [
        ":port_mask",
        "//cpu_instructions/proto:microarchitecture_cc_proto",
        "//cpu_instructions/util:proto_util",
        "//strings",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
    ],
)

cc_test(
    name = "cpu_type_test",
    srcs = ["cpu_type_test.cc"],
    deps = [
        ":cpu_type",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# A library for getting host CPU info.
cc_library(
    name = "host_cpu",
    srcs = ["host_cpu.cc"],
    hdrs = ["host_cpu.h"],
    deps = [
        "//base",
        "//cpu_instructions/proto:microarchitecture_cc_proto",
        "//strings",
        "//util/gtl:map_util",
        "@com_google_protobuf//:protobuf_lite",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
    ],
)

cc_test(
    name = "host_cpu_test",
    srcs = ["host_cpu_test.cc"],
    deps = [
        ":host_cpu",
        "//base",
        "//cpu_instructions/proto:microarchitecture_cc_proto",
        "//strings",
        "@com_google_protobuf//:protobuf_lite",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
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
        "//cpu_instructions/proto:microarchitecture_cc_proto",
        "//strings",
        "@com_google_protobuf//:protobuf_lite",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
    ],
)

cc_test(
    name = "port_mask_test",
    size = "small",
    srcs = ["port_mask_test.cc"],
    deps = [
        ":port_mask",
        "//base",
        "//cpu_instructions/proto:microarchitecture_cc_proto",
        "//strings",
        "@com_google_protobuf//:protobuf_lite",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
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
        ":cpu_type",
        ":port_mask",
        "//base",
        "//cpu_instructions/proto:instructions_cc_proto",
        "//cpu_instructions/proto:microarchitecture_cc_proto",
        "//strings",
        "@com_google_protobuf//:protobuf_lite",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
    ],
)

cc_test(
    name = "prettyprint_test",
    size = "small",
    srcs = ["prettyprint_test.cc"],
    deps = [
        ":prettyprint",
        "//base",
        "//cpu_instructions/proto:instructions_cc_proto",
        "//cpu_instructions/proto:microarchitecture_cc_proto",
        "//cpu_instructions/util:proto_util",
        "@com_google_protobuf//:protobuf_lite",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
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
        "@com_google_protobuf//:protobuf_lite",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
    ],
)

cc_test(
    name = "transform_factory_test",
    srcs = ["transform_factory_test.cc"],
    deps = [
        ":cleanup_instruction_set",
        ":transform_factory",
        "//base",
        "@com_google_protobuf//:protobuf_lite",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)
