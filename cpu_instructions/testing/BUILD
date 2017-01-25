# Descripting:
#   Test utilities.

package(default_visibility = ["//:internal_users"])

load("//cpu_instructions:proto_library.bzl", "cpu_instructions_proto_library")

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "test_util",
    testonly = 1,
    hdrs = ["test_util.h"],
    deps = [
        "//base",
        "//external:gflags",
        "//external:glog",
        "//external:googletest",
        "//external:protobuf_clib",
        "//external:protobuf_clib_for_base",
        "//strings",
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
