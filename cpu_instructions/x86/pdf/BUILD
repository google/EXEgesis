# Description:
#   Intel Software Development Manual (SDM) Parser. See README.md.

package(default_visibility = ["//:internal_users"])

load("//cpu_instructions:proto_library.bzl", "cpu_instructions_proto_library")

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "vendor_syntax",
    srcs = ["vendor_syntax.cc"],
    hdrs = ["vendor_syntax.h"],
    deps = [
        "//base",
        "//cpu_instructions/proto:instructions_proto",
        "//strings",
        "//util/gtl:map_util",
        "@com_googlesource_code_re2//:re2",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@protobuf_git//:protobuf_lite",
    ],
)

cc_test(
    name = "vendor_syntax_test",
    srcs = ["vendor_syntax_test.cc"],
    deps = [
        ":vendor_syntax",
        "//cpu_instructions/testing:test_util",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

cc_library(
    name = "intel_sdm_extractor",
    srcs = ["intel_sdm_extractor.cc"],
    hdrs = ["intel_sdm_extractor.h"],
    deps = [
        ":vendor_syntax",
        "//base",
        "//cpu_instructions/proto:instructions_proto",
        "//cpu_instructions/proto/pdf:pdf_document_proto",
        "//cpu_instructions/proto/pdf/x86:intel_sdm_proto",
        "//cpu_instructions/util/pdf:pdf_document_utils",
        "//strings",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
        "@com_googlesource_code_re2//:re2",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@protobuf_git//:protobuf",
        "@protobuf_git//:protobuf_lite",
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
        "//cpu_instructions/testing:test_util",
        "//cpu_instructions/util:proto_util",
        "//cpu_instructions/util/pdf:pdf_document_parser",
        "//strings",
        "@googletest_git//:gtest_main",
        "@protobuf_git//:protobuf",
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
        "//base",
        "//cpu_instructions/proto:instructions_proto",
        "//cpu_instructions/util:proto_util",
        "//cpu_instructions/util/pdf:pdf_document_utils",
        "//cpu_instructions/util/pdf:xpdf_util",
        "//strings",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
        "@com_googlesource_code_re2//:re2",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@protobuf_git//:protobuf",
        "@protobuf_git//:protobuf_lite",
    ],
)
