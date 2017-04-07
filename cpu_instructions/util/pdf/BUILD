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
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib_for_base",
    ],
)

cc_test(
    name = "geometry_test",
    srcs = ["geometry_test.cc"],
    deps = [
        ":geometry",
        "//external:googletest",
        "//external:googletest_main",
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
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib_for_base",
        "//strings",
        "//util/graph:connected_components",
        "//util/gtl:map_util",
    ],
)

cc_test(
    name = "pdf_document_parser_test",
    srcs = ["pdf_document_parser_test.cc"],
    deps = [
        ":pdf_document_parser",
        "//cpu_instructions/util:proto_util",
        "//external:googletest_main",
        "//external:protobuf_clib",
    ],
)

cc_library(
    name = "pdf_document_utils",
    srcs = ["pdf_document_utils.cc"],
    hdrs = ["pdf_document_utils.h"],
    deps = [
        "//base",
        "//cpu_instructions/proto/pdf:pdf_document_proto",
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib_for_base",
        "//strings",
        "//util/gtl:map_util",
    ],
)

cc_test(
    name = "pdf_document_utils_test",
    srcs = ["pdf_document_utils_test.cc"],
    deps = [
        ":pdf_document_utils",
        "//base",
        "//cpu_instructions/util:proto_util",
        "//external:gflags",
        "//external:glog",
        "//external:googletest",
        "//external:googletest_main",
        "//external:protobuf_clib",
        "//external:protobuf_clib_for_base",
        "//strings",
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
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib_for_base",
        "//external:re2",
        "//external:utf",
        "//external:xpdf",
        "//strings",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
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
        "//external:gflags",
        "//external:glog",
        "//external:googletest_main",
        "//external:protobuf_clib",
        "//external:protobuf_clib_for_base",
        "//strings",
        "//util/gtl:ptr_util",
    ],
)
