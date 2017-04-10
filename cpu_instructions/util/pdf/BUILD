# Description:
#   Pdf Parser. See README.md.

package(default_visibility = ["//:internal_users"])

load("//cpu_instructions:proto_library.bzl", "cpu_instructions_proto_library")

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "geometry",
    srcs = ["geometry.cc"],
    hdrs = ["geometry.h"],
    deps = [
        "//base",
        "//cpu_instructions/proto/pdf:pdf_document_proto",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@protobuf_git//:protobuf_lite",
    ],
)

cc_test(
    name = "geometry_test",
    srcs = ["geometry_test.cc"],
    deps = [
        ":geometry",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

cc_library(
    name = "pdf_document_parser",
    srcs = ["pdf_document_parser.cc"],
    hdrs = ["pdf_document_parser.h"],
    deps = [
        ":geometry",
        "//base",
        "//cpu_instructions/proto/pdf:pdf_document_proto",
        "//strings",
        "//util/graph:connected_components",
        "//util/gtl:map_util",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@protobuf_git//:protobuf_lite",
    ],
)

cc_test(
    name = "pdf_document_parser_test",
    srcs = ["pdf_document_parser_test.cc"],
    deps = [
        ":pdf_document_parser",
        "//cpu_instructions/util:proto_util",
        "@googletest_git//:gtest_main",
        "@protobuf_git//:protobuf",
    ],
)

cc_library(
    name = "pdf_document_utils",
    srcs = ["pdf_document_utils.cc"],
    hdrs = ["pdf_document_utils.h"],
    deps = [
        "//base",
        "//cpu_instructions/proto/pdf:pdf_document_proto",
        "//strings",
        "//util/gtl:map_util",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@protobuf_git//:protobuf_lite",
    ],
)

cc_test(
    name = "pdf_document_utils_test",
    srcs = ["pdf_document_utils_test.cc"],
    deps = [
        ":pdf_document_utils",
        "//base",
        "//cpu_instructions/util:proto_util",
        "//strings",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
        "@protobuf_git//:protobuf",
        "@protobuf_git//:protobuf_lite",
    ],
)

cc_library(
    name = "xpdf_util",
    srcs = ["xpdf_util.cc"],
    hdrs = ["xpdf_util.h"],
    deps = [
        ":geometry",
        ":pdf_document_parser",
        ":pdf_document_utils",
        "//base",
        "//cpu_instructions/proto/pdf:pdf_document_proto",
        "//strings",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
        "@com_googlesource_code_re2//:re2",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@protobuf_git//:protobuf_lite",
        "@utf_archive//:utf",
        "@xpdf_archive//:xpdf",
    ],
)

cc_test(
    name = "xpdf_util_test",
    srcs = ["xpdf_util_test.cc"],
    data = [
        "testdata/simple.pdf",
    ],
    deps = [
        ":xpdf_util",
        "//base",
        "//cpu_instructions/testing:test_util",
        "//strings",
        "//util/gtl:ptr_util",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@googletest_git//:gtest_main",
        "@protobuf_git//:protobuf",
        "@protobuf_git//:protobuf_lite",
    ],
)
