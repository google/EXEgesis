# Description:
#   Intel Software Development Manual (SDM) Parser. See README.md.

load("//cpu_instructions:proto_library.bzl", "cpu_instructions_proto_library")


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
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
    ],
)

cc_test(
    name = "geometry_test",
    srcs = ["geometry_test.cc"],
    deps = [
        ":geometry",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
    ],
)

cc_library(
    name = "vendor_syntax",
    srcs = ["vendor_syntax.cc"],
    hdrs = ["vendor_syntax.h"],
    deps = [
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//strings",
        "//util/gtl:map_util",
        "//cpu_instructions/proto:instructions_proto",
        "//external:re2",
    ],
)

cc_test(
    name = "vendor_syntax_test",
    srcs = ["vendor_syntax_test.cc"],
    deps = [
        ":vendor_syntax",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
    ],
)

cc_library(
    name = "proto_util",
    srcs = ["proto_util.cc"],
    hdrs = ["proto_util.h"],
    visibility = [
        "//cpu_instructions/tools:__subpackages__",
    ],
    deps = [
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        
        "//external:protobuf_clib",
        "//strings",
    ],
)

cc_test(
    name = "proto_util_test",
    srcs = ["proto_util_test.cc"],
    deps = [
        ":pdf_document_proto",
        ":proto_util",
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//external:protobuf_clib",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
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
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
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
        "//external:protobuf_clib",
        "//cpu_instructions/testing:googletest_main",
    ],
)

cc_library(
    name = "pdf_document_utils",
    srcs = ["pdf_document_utils.cc"],
    hdrs = ["pdf_document_utils.h"],
    deps = [
        ":pdf_document_proto",
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
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
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//external:protobuf_clib",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
        "//strings",
    ],
)

cpu_instructions_proto_library(
    name = "intel_sdm_proto",
    srcs = ["intel_sdm.proto"],
    cc_api_version = 2,
    deps = [
        ":pdf_document_proto",
        "//cpu_instructions/proto:cpu_info_proto",
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
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//external:protobuf_clib",
        "//strings",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
        "//cpu_instructions/proto:instructions_proto",
        "//external:re2",
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
        "//external:protobuf_clib",
        "//cpu_instructions/testing:googletest_main",
        "//strings",
    ],
)

# The main entry point.
cc_library(
    name = "parse_sdm",
    srcs = ["parse_sdm.cc"],
    hdrs = ["parse_sdm.h"],
    data = [":pdf_document_patches.pbtxt"],
    visibility = [
        "//cpu_instructions/tools:__subpackages__",
    ],
    deps = [
        ":intel_sdm_extractor",
        ":pdf_document_utils",
        ":proto_util",
        ":xpdf_util",
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//external:protobuf_clib",
        "//strings",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
        "//cpu_instructions/proto:cpu_info_proto",
        "//cpu_instructions/proto:instructions_proto",
        "//external:re2",
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
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//strings",
        "//external:utf",
        "//external:xpdf",
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
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//external:protobuf_clib",
        "//cpu_instructions/testing:googletest_main",
        "//strings",
        "//util/gtl:ptr_util",
    ],
)
