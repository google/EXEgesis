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
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "bits_test",
    srcs = ["bits_test.cc"],
    deps = [
        ":bits",
        "//base",
        "//strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# Helper functions for working with instruction syntax.
cc_library(
    name = "instruction_syntax",
    srcs = ["instruction_syntax.cc"],
    hdrs = ["instruction_syntax.h"],
    deps = [
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "instruction_syntax_test",
    size = "small",
    srcs = ["instruction_syntax_test.cc"],
    deps = [
        ":instruction_syntax",
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/testing:test_util",
        "//strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# Utilities to read and write binary and text protos from files and strings.
cc_library(
    name = "proto_util",
    srcs = ["proto_util.cc"],
    hdrs = ["proto_util.h"],
    deps = [
        "//base",
        "//strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "proto_util_test",
    srcs = ["proto_util_test.cc"],
    deps = [
        ":proto_util",
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/testing:test_util",
        "//strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# Helper functions for working with Status object.
cc_library(
    name = "status_util",
    srcs = ["status_util.cc"],
    hdrs = ["status_util.h"],
    deps = [
        "//base",
        "//util/task:status",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "status_util_test",
    size = "small",
    srcs = ["status_util_test.cc"],
    deps = [
        ":status_util",
        "//util/task:status",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# A library of string functions.
cc_library(
    name = "strings",
    srcs = ["strings.cc"],
    hdrs = ["strings.h"],
    deps = [
        "//base",
        "//strings",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_test(
    name = "strings_test",
    size = "small",
    srcs = ["strings_test.cc"],
    deps = [
        ":strings",
        "//strings",
        "//util/task:status",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# Utilities for interacting with the host and system.

cc_library(
    name = "system",
    srcs = ["system.cc"],
    hdrs = ["system.h"],
    deps = [
        "//base",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)
