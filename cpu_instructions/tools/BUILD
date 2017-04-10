# Description:
#   All public tools for the project.

package(default_visibility = ["//visibility:private"])

licenses(["notice"])  # Apache 2.0

# A tool that parses the Intel Software Development Manual.

cc_binary(
    name = "parse_sdm",
    srcs = ["parse_sdm.cc"],
    deps = [
        "//base",
        "//cpu_instructions/base:transform_factory",
        "//cpu_instructions/proto:instructions_proto",
        "//cpu_instructions/util:proto_util",
        "//cpu_instructions/x86/pdf:parse_sdm",
        "//strings",
        "//util/task:status",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@protobuf_git//:protobuf_lite",
    ],
)

cc_binary(
    name = "pdf2proto",
    srcs = ["pdf2proto.cc"],
    deps = [
        "//base",
        "//cpu_instructions/proto/pdf:pdf_document_proto",
        "//cpu_instructions/util:proto_util",
        "//cpu_instructions/util/pdf:xpdf_util",
        "//strings",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@protobuf_git//:protobuf_lite",
    ],
)

cc_binary(
    name = "proto_patch_helper",
    srcs = ["proto_patch_helper.cc"],
    deps = [
        "//base",
        "//cpu_instructions/proto/pdf:pdf_document_proto",
        "//cpu_instructions/util:proto_util",
        "//cpu_instructions/util/pdf:xpdf_util",
        "//strings",
        "//util/gtl:map_util",
        "@com_googlesource_code_re2//:re2",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@protobuf_git//:protobuf_lite",
    ],
)
