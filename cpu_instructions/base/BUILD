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
        "//cpu_instructions/proto:instructions_proto",
        "//util/gtl:map_util",
        "//util/task:status",
        "//util/task:statusor",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@protobuf_git//:protobuf",
        "@protobuf_git//:protobuf_lite",
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
        "//cpu_instructions/proto:instructions_proto",
        "//cpu_instructions/testing:test_util",
        "@googletest_git//:gtest",
        "@protobuf_git//:protobuf",
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
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
        "@protobuf_git//:protobuf",
        "@protobuf_git//:protobuf_lite",
    ],
)

# A library to represent known CPU types.
cc_library(
    name = "cpu_type",
    srcs = ["cpu_type.cc"],
    hdrs = ["cpu_type.h"],
    deps = [
        ":port_mask",
        "//cpu_instructions/proto:cpu_type_proto",
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
        "//cpu_instructions/proto:cpu_type_proto",
        "//strings",
        "//util/gtl:map_util",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@protobuf_git//:protobuf_lite",
    ],
)

cc_test(
    name = "host_cpu_test",
    srcs = ["host_cpu_test.cc"],
    deps = [
        ":host_cpu",
        "//base",
        "//cpu_instructions/proto:cpu_type_proto",
        "//strings",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
        "@protobuf_git//:protobuf_lite",
    ],
)

# An efficient representation of the execution unit port mask.
cc_library(
    name = "port_mask",
    srcs = ["port_mask.cc"],
    hdrs = ["port_mask.h"],
    deps = [
        "//base",
        "//cpu_instructions/proto:cpu_type_proto",
        "//strings",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@protobuf_git//:protobuf_lite",
    ],
)

cc_test(
    name = "port_mask_test",
    size = "small",
    srcs = ["port_mask_test.cc"],
    deps = [
        ":port_mask",
        "//base",
        "//cpu_instructions/proto:cpu_type_proto",
        "//strings",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
        "@protobuf_git//:protobuf_lite",
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
        "//cpu_instructions/proto:cpu_type_proto",
        "//cpu_instructions/proto:instructions_proto",
        "//strings",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@protobuf_git//:protobuf_lite",
    ],
)

cc_test(
    name = "prettyprint_test",
    size = "small",
    srcs = ["prettyprint_test.cc"],
    deps = [
        ":prettyprint",
        "//base",
        "//cpu_instructions/proto:cpu_type_proto",
        "//cpu_instructions/proto:instructions_proto",
        "//cpu_instructions/util:proto_util",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
        "@protobuf_git//:protobuf_lite",
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
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@protobuf_git//:protobuf_lite",
    ],
)

cc_test(
    name = "transform_factory_test",
    srcs = ["transform_factory_test.cc"],
    deps = [
        ":cleanup_instruction_set",
        ":transform_factory",
        "//base",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
        "@protobuf_git//:protobuf_lite",
    ],
)
