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
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

# A library of functions for working with categories.

cc_library(
    name = "category_util",
    srcs = [],
    hdrs = ["category_util.h"],
    deps = [
        "//base",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "category_util_test",
    size = "small",
    srcs = ["category_util_test.cc"],
    deps = [
        ":category_util",
        "//base",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

# Helper functions for working with files.
cc_library(
    name = "file_util",
    srcs = ["file_util.cc"],
    hdrs = ["file_util.h"],
    deps = [
        "//base",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "file_util_test",
    size = "small",
    srcs = ["file_util_test.cc"],
    deps = [
        ":file_util",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# A type for indices with strong type checks.
cc_library(
    name = "index_type",
    hdrs = ["index_type.h"],
)

cc_test(
    name = "index_type_test",
    size = "small",
    srcs = ["index_type_test.cc"],
    deps = [
        ":index_type",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
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
        "//util/gtl:map_util",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_test(
    name = "instruction_syntax_test",
    size = "small",
    srcs = ["instruction_syntax_test.cc"],
    deps = [
        ":instruction_syntax",
        ":proto_util",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/testing:test_util",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# Utilities to read and write binary and text protos from files and strings.
cc_library(
    name = "proto_util",
    srcs = ["proto_util.cc"],
    hdrs = ["proto_util.h"],
    deps = [
        "//base",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
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
        "//file/base:path",
        "//util/task:status",
        "@com_github_glog_glog//:glog",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
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
        "//exegesis/testing:test_util",
        "//util/task:status",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# A library of string functions.
cc_library(
    name = "strings",
    srcs = ["strings.cc"],
    hdrs = ["strings.h"],
    deps = [
        "//base",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
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
        "//exegesis/testing:test_util",
        "//util/task:status",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# A library for working with structured registers.
cc_library(
    name = "structured_register",
    hdrs = ["structured_register.h"],
)

cc_test(
    name = "structured_register_test",
    size = "small",
    srcs = ["structured_register_test.cc"],
    deps = [
        ":structured_register",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# A library of string functions.
cc_library(
    name = "text_processing",
    srcs = ["text_processing.cc"],
    hdrs = ["text_processing.h"],
    deps = [
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_test(
    name = "text_processing_test",
    size = "small",
    srcs = ["text_processing_test.cc"],
    deps = [
        ":text_processing",
        "@com_google_googletest//:gtest_main",
    ],
)

# Utilities for interacting with the host and system.
cc_library(
    name = "system",
    srcs = ["system.cc"],
    hdrs = ["system.h"],
    deps = [
        "//base",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)
