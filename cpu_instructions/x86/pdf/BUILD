# Description:
#   Intel Software Development Manual (SDM) Parser. See README.md.

package(default_visibility = ["//:internal_users"])

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "vendor_syntax",
    srcs = ["vendor_syntax.cc"],
    hdrs = ["vendor_syntax.h"],
    deps = [
        "//base",
        "//cpu_instructions/proto:instructions_cc_proto",
        "//strings",
        "//util/gtl:map_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
        "@glog_git//:glog",
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
        "//cpu_instructions/proto:instructions_cc_proto",
        "//cpu_instructions/proto/pdf:pdf_document_cc_proto",
        "//cpu_instructions/proto/pdf/x86:intel_sdm_cc_proto",
        "//cpu_instructions/util/pdf:pdf_document_utils",
        "//strings",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
        "@glog_git//:glog",
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
        "@com_google_protobuf//:protobuf",
        "@googletest_git//:gtest_main",
    ],
)

# The main entry point.
cc_library(
    name = "parse_sdm",
    srcs = ["parse_sdm.cc"],
    hdrs = ["parse_sdm.h"],
    data = [
        "sdm_patches/sdm_2016_04_vol2a.pbtxt",
        "sdm_patches/sdm_2016_04_vol2b.pbtxt",
        "sdm_patches/sdm_2016_06_vol2.pbtxt",
        "sdm_patches/sdm_2016_09_vol2.pbtxt",
        "sdm_patches/sdm_2016_12_vol123.pbtxt",
        "sdm_patches/sdm_2017_03_vol123.pbtxt",
    ],
    deps = [
        ":intel_sdm_extractor",
        "//base",
        "//cpu_instructions/proto:instructions_cc_proto",
        "//cpu_instructions/util:proto_util",
        "//cpu_instructions/util/pdf:pdf_document_utils",
        "//cpu_instructions/util/pdf:xpdf_util",
        "//cpu_instructions/x86:cleanup_instruction_set_all",
        "//strings",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
        "@glog_git//:glog",
    ],
)
