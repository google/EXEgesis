# Description:
#   Generic utility libraries.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

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

# Helper functions for wokring with Status object.
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
