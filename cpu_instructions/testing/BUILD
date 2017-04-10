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
        "//strings",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@googletest_git//:gtest",
        "@protobuf_git//:protobuf",
        "@protobuf_git//:protobuf_lite",
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
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)
