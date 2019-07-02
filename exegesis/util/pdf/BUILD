# Description:
#   Pdf Parser. See README.md.

package(default_visibility = ["//:internal_users"])

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "geometry",
    srcs = ["geometry.cc"],
    hdrs = ["geometry.h"],
    deps = [
        "//base",
        "//exegesis/proto/pdf:pdf_document_cc_proto",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "geometry_test",
    srcs = ["geometry_test.cc"],
    deps = [
        ":geometry",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "pdf_document_parser",
    srcs = ["pdf_document_parser.cc"],
    hdrs = ["pdf_document_parser.h"],
    deps = [
        ":geometry",
        "//exegesis/proto/pdf:pdf_document_cc_proto",
        "//util/graph:connected_components",
        "//util/gtl:map_util",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "pdf_document_parser_test",
    srcs = ["pdf_document_parser_test.cc"],
    deps = [
        ":pdf_document_parser",
        "//exegesis/util:proto_util",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "pdf_document_utils",
    srcs = ["pdf_document_utils.cc"],
    hdrs = ["pdf_document_utils.h"],
    deps = [
        "//base",
        "//exegesis/proto/pdf:pdf_document_cc_proto",
        "//exegesis/util:proto_util",
        "//util/gtl:map_util",
        "//util/task:status",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "pdf_document_utils_test",
    srcs = ["pdf_document_utils_test.cc"],
    deps = [
        ":pdf_document_utils",
        "//base",
        "//exegesis/testing:test_util",
        "//exegesis/util:proto_util",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
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
        "//exegesis/proto/pdf:pdf_document_cc_proto",
        "//util/gtl:map_util",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/memory",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
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
        "//exegesis/testing:test_util",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
    ],
)
