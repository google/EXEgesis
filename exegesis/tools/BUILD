# Description:
#   All public tools for the project.

package(default_visibility = ["//visibility:private"])

licenses(["notice"])  # Apache 2.0

# A library that provides access to instruction sets for all supported architectures.

cc_library(
    name = "instruction_sets",
    srcs = ["instruction_sets.cc"],
    hdrs = ["instruction_sets.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//base",
        "//exegesis/base:architecture_provider",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/base:microarchitecture",
        "//exegesis/base:restrict",
        "//exegesis/base:transform_factory",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/x86:cleanup_instruction_set_all",
        "//strings",
        "//util/gtl:map_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_protobuf//:protobuf_lite",
        "@glog_git//:glog",
    ],
)

# A tool that parses the Intel Software Development Manual.

cc_binary(
    name = "parse_sdm",
    srcs = ["parse_sdm.cc"],
    deps = [
        "//base",
        "//exegesis/base:transform_factory",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:proto_util",
        "//exegesis/x86/pdf:parse_sdm",
        "//strings",
        "//util/task:status",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_protobuf//:protobuf_lite",
        "@glog_git//:glog",
    ],
)

cc_binary(
    name = "pdf2proto",
    srcs = ["pdf2proto.cc"],
    deps = [
        "//base",
        "//exegesis/proto/pdf:pdf_document_cc_proto",
        "//exegesis/util:proto_util",
        "//exegesis/util/pdf:xpdf_util",
        "//strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_protobuf//:protobuf_lite",
        "@glog_git//:glog",
    ],
)

cc_binary(
    name = "proto_patch_helper",
    srcs = ["proto_patch_helper.cc"],
    deps = [
        "//base",
        "//exegesis/proto/pdf:pdf_document_cc_proto",
        "//exegesis/util:proto_util",
        "//exegesis/util/pdf:xpdf_util",
        "//strings",
        "//util/gtl:map_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
        "@glog_git//:glog",
    ],
)

cc_binary(
    name = "proto_patch_migrate",
    srcs = ["proto_patch_migrate.cc"],
    deps = [
        "//base",
        "//exegesis/proto/pdf:pdf_document_cc_proto",
        "//exegesis/util:proto_util",
        "//exegesis/util/pdf:pdf_document_utils",
        "//exegesis/util/pdf:xpdf_util",
        "//strings",
        "//util/gtl:map_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
        "@glog_git//:glog",
    ],
)
