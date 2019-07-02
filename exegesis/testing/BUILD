# Descripting:
#   Test utilities.

package(default_visibility = ["//:internal_users"])

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "test_util",
    testonly = 1,
    srcs = ["test_util.cc"],
    hdrs = ["test_util.h"],
    deps = [
        "//base",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

proto_library(
    name = "test_proto",
    srcs = ["test.proto"],
)

cc_proto_library(
    name = "test_cc_proto",
    deps = [
        ":test_proto",
    ],
)

cc_test(
    name = "test_util_test",
    size = "small",
    srcs = ["test_util_test.cc"],
    deps = [
        ":test_cc_proto",
        ":test_util",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
