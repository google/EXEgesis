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
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:strings",
        "//util/gtl:map_util",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_test(
    name = "vendor_syntax_test",
    srcs = ["vendor_syntax_test.cc"],
    deps = [
        ":vendor_syntax",
        "//exegesis/testing:test_util",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

proto_library(
    name = "intel_sdm_proto",
    srcs = ["intel_sdm.proto"],
    deps = [
        "//exegesis/proto:instructions_proto",
        "//exegesis/proto/pdf:pdf_document_proto",
    ],
)

cc_proto_library(
    name = "intel_sdm_cc_proto",
    deps = [
        ":intel_sdm_proto",
    ],
)

cc_library(
    name = "intel_sdm_extractor",
    srcs = ["intel_sdm_extractor.cc"],
    hdrs = ["intel_sdm_extractor.h"],
    deps = [
        ":intel_sdm_cc_proto",
        ":vendor_syntax",
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto/pdf:pdf_document_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//exegesis/util:strings",
        "//exegesis/util:text_processing",
        "//exegesis/util/pdf:pdf_document_utils",
        "//net/proto2/util/public:repeated_field_util",
        "//util/gtl:map_util",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
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
        "//exegesis/testing:test_util",
        "//exegesis/util:proto_util",
        "//exegesis/util/pdf:pdf_document_parser",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

# The main entry point.
filegroup(
    name = "sdm_patches",
    srcs = glob(["sdm_patches/*"]),
)

cc_library(
    name = "parse_sdm",
    srcs = ["parse_sdm.cc"],
    hdrs = ["parse_sdm.h"],
    data = [":sdm_patches"],
    deps = [
        ":intel_sdm_extractor",
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:proto_util",
        "//exegesis/util/pdf:pdf_document_utils",
        "//exegesis/util/pdf:xpdf_util",
        "//exegesis/x86:cleanup_instruction_set_all",
        "//exegesis/x86:registers",
        "//net/proto2/util/public:repeated_field_util",
        "//util/gtl:map_util",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
    ],
)
