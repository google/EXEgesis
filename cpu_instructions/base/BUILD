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
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib",
        "//external:protobuf_clib_for_base",
        "//util/gtl:map_util",
        "//util/task:status",
        "//util/task:statusor",
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
        "//external:googletest",
        "//external:protobuf_clib",
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
        "//external:gflags",
        "//external:glog",
        "//external:googletest",
        "//external:googletest_main",
        "//external:protobuf_clib",
        "//external:protobuf_clib_for_base",
        "//util/task:status",
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
        "//external:googletest",
        "//external:googletest_main",
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
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib_for_base",
        "//strings",
        "//util/gtl:map_util",
    ],
)

cc_test(
    name = "host_cpu_test",
    srcs = ["host_cpu_test.cc"],
    deps = [
        ":host_cpu",
        "//base",
        "//cpu_instructions/proto:cpu_type_proto",
        "//external:gflags",
        "//external:glog",
        "//external:googletest",
        "//external:googletest_main",
        "//external:protobuf_clib_for_base",
        "//strings",
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
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib_for_base",
        "//strings",
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
        "//external:gflags",
        "//external:glog",
        "//external:googletest",
        "//external:googletest_main",
        "//external:protobuf_clib_for_base",
        "//strings",
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
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib_for_base",
        "//strings",
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
        "//external:gflags",
        "//external:glog",
        "//external:googletest",
        "//external:googletest_main",
        "//external:protobuf_clib_for_base",
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
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib_for_base",
        "//strings",
        "//util/gtl:map_util",
    ],
)

cc_test(
    name = "transform_factory_test",
    srcs = ["transform_factory_test.cc"],
    deps = [
        ":cleanup_instruction_set",
        ":transform_factory",
        "//base",
        "//external:gflags",
        "//external:glog",
        "//external:googletest",
        "//external:googletest_main",
        "//external:protobuf_clib_for_base",
    ],
)
