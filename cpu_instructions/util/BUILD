# Description:
#   Generic utility libraries.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

# A library for working with bits in an unsigned integer.
cc_library(
    name = "bits",
    hdrs = ["bits.h"],
    deps = [
        "//base",
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib_for_base",
    ],
)

cc_test(
    name = "bits_test",
    srcs = ["bits_test.cc"],
    deps = [
        ":bits",
        "//base",
        "//external:gflags",
        "//external:glog",
        "//external:googletest",
        "//external:googletest_main",
        "//external:protobuf_clib_for_base",
        "//strings",
    ],
)

# Helper functions for working with instruction syntax.
cc_library(
    name = "instruction_syntax",
    srcs = ["instruction_syntax.cc"],
    hdrs = ["instruction_syntax.h"],
    deps = [
        "//base",
        "//cpu_instructions/proto:instructions_proto",
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib_for_base",
        "//strings",
    ],
)

cc_test(
    name = "instruction_syntax_test",
    size = "small",
    srcs = ["instruction_syntax_test.cc"],
    deps = [
        ":instruction_syntax",
        "//base",
        "//cpu_instructions/proto:instructions_proto",
        "//cpu_instructions/testing:test_util",
        "//external:gflags",
        "//external:glog",
        "//external:googletest",
        "//external:googletest_main",
        "//external:protobuf_clib_for_base",
        "//strings",
    ],
)

# Utilities to read and write binary and text protos from files and strings.
cc_library(
    name = "proto_util",
    srcs = ["proto_util.cc"],
    hdrs = ["proto_util.h"],
    deps = [
        "//base",
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib",
        "//external:protobuf_clib_for_base",
        "//strings",
    ],
)

cc_test(
    name = "proto_util_test",
    srcs = ["proto_util_test.cc"],
    deps = [
        ":proto_util",
        "//base",
        "//cpu_instructions/proto:instructions_proto",
        "//cpu_instructions/testing:test_util",
        "//external:gflags",
        "//external:glog",
        "//external:googletest",
        "//external:googletest_main",
        "//external:protobuf_clib",
        "//external:protobuf_clib_for_base",
        "//strings",
    ],
)

# Helper functions for working with Status object.
cc_library(
    name = "status_util",
    srcs = ["status_util.cc"],
    hdrs = ["status_util.h"],
    deps = [
        "//base",
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib_for_base",
        "//util/task:status",
    ],
)

cc_test(
    name = "status_util_test",
    size = "small",
    srcs = ["status_util_test.cc"],
    deps = [
        ":status_util",
        "//external:googletest",
        "//external:googletest_main",
        "//util/task:status",
    ],
)
