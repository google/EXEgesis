# Descripting:
#   Test utilities.

package(
    default_visibility = ["//visibility:public"],
)

load("//cpu_instructions:proto_library.bzl", "cpu_instructions_proto_library")

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "test_util",
    testonly = 1,
    hdrs = ["test_util.h"],
    deps = [
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//external:protobuf_clib",
        
        "//strings",
        "//external:googletest",
    ],
)

cpu_instructions_proto_library(
    name = "test_proto",
    srcs = ["test.proto"],
    cc_api_version = 2,
)

cc_test(
    name = "test_util_test",
    size = "small",
    srcs = ["test_util_test.cc"],
    deps = [
        ":test_proto",
        ":test_util",
        "//external:googletest",
        "//external:googletest_main",
    ],
)

cc_library(
    name = "googletest",
    testonly = 1,
    deps = [
        ":test_util",
        "//external:googletest",
    ],
)

cc_library(
    name = "googletest_main",
    testonly = 1,
    deps = [
        ":googletest",
        "//external:googletest_main",
    ],
)
