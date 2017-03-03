# Description:
#   Intel Software Development Manual (SDM) Parser. See README.md.

package(default_visibility = ["//:internal_users"])

load("//cpu_instructions:proto_library.bzl", "cpu_instructions_proto_library")

licenses(["notice"])  # Apache 2.0

cpu_instructions_proto_library(
    name = "pdf_document_proto",
    srcs = ["pdf_document.proto"],
    cc_api_version = 2,
)

cc_library(
    name = "geometry",
    srcs = ["geometry.cc"],
    hdrs = ["geometry.h"],
    deps = [
        ":pdf_document_proto",
        "//base",
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
    name = "vendor_syntax",
    srcs = ["vendor_syntax.cc"],
    hdrs = ["vendor_syntax.h"],
    deps = [
        "//base",
        "//cpu_instructions/proto:instructions_proto",
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib_for_base",
        "//external:re2",
        "//strings",
        "//util/gtl:map_util",
    ],
)

cc_test(
    name = "vendor_syntax_test",
    srcs = ["vendor_syntax_test.cc"],
    deps = [
        ":vendor_syntax",
        "//cpu_instructions/testing:test_util",
        "//external:googletest",
        "//external:googletest_main",
    ],
)

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
        ":pdf_document_proto",
        ":proto_util",
        "//base",
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

cc_library(
    name = "pdf_document_parser",
    srcs = ["pdf_document_parser.cc"],
    hdrs = ["pdf_document_parser.h"],
    deps = [
        ":geometry",
        ":pdf_document_proto",
        "//base",
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
        ":proto_util",
        "//external:googletest_main",
        "//external:protobuf_clib",
    ],
)

cc_library(
    name = "pdf_document_utils",
    srcs = ["pdf_document_utils.cc"],
    hdrs = ["pdf_document_utils.h"],
    deps = [
        ":pdf_document_proto",
        "//base",
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
        ":proto_util",
        "//base",
        "//external:gflags",
        "//external:glog",
        "//external:googletest",
        "//external:googletest_main",
        "//external:protobuf_clib",
        "//external:protobuf_clib_for_base",
        "//strings",
    ],
)

cpu_instructions_proto_library(
    name = "intel_sdm_proto",
    srcs = ["intel_sdm.proto"],
    cc_api_version = 2,
    deps = [
        ":pdf_document_proto",
        "//cpu_instructions/proto:cpu_type_proto",
        "//cpu_instructions/proto:instructions_proto",
    ],
)

cc_library(
    name = "intel_sdm_extractor",
    srcs = ["intel_sdm_extractor.cc"],
    hdrs = ["intel_sdm_extractor.h"],
    deps = [
        ":intel_sdm_proto",
        ":pdf_document_proto",
        ":pdf_document_utils",
        ":vendor_syntax",
        "//base",
        "//cpu_instructions/proto:instructions_proto",
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib",
        "//external:protobuf_clib_for_base",
        "//external:re2",
        "//strings",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
    ],
)

cc_test(
    name = "intel_sdm_extractor_test",
    srcs = ["intel_sdm_extractor_test.cc"],
    data = [
        "testdata/253666_p170_p171_instructionset.pbtxt",
        "testdata/253666_p170_p171_pdfdoc.pbtxt",
        "testdata/253666_p170_p171_sdmdoc.pbtxt",
    ],
    deps = [
        ":intel_sdm_extractor",
        ":pdf_document_parser",
        ":proto_util",
        "//cpu_instructions/testing:test_util",
        "//external:googletest_main",
        "//external:protobuf_clib",
        "//strings",
    ],
)

# The main entry point.
cc_library(
    name = "parse_sdm",
    srcs = ["parse_sdm.cc"],
    hdrs = ["parse_sdm.h"],
    data = [":sdm_patches.pbtxt"],
    deps = [
        ":intel_sdm_extractor",
        ":pdf_document_utils",
        ":proto_util",
        ":xpdf_util",
        "//base",
        "//cpu_instructions/proto:instructions_proto",
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib",
        "//external:protobuf_clib_for_base",
        "//external:re2",
        "//strings",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
    ],
)

cc_library(
    name = "xpdf_util",
    srcs = ["xpdf_util.cc"],
    hdrs = ["xpdf_util.h"],
    deps = [
        ":geometry",
        ":pdf_document_parser",
        ":pdf_document_proto",
        ":pdf_document_utils",
        "//base",
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib_for_base",
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
