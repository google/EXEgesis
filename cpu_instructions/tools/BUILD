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
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib_for_base",
        "//strings",
        "//util/task:status",
    ],
)

cc_binary(
    name = "pdf2proto",
    srcs = ["pdf2proto.cc"],
    deps = [
        "//base",
        "//cpu_instructions/base:transform_factory",
        "//cpu_instructions/proto/pdf:pdf_document_proto",
        "//cpu_instructions/util:proto_util",
        "//cpu_instructions/util/pdf:xpdf_util",
        "//external:gflags",
        "//external:glog",
        "//external:protobuf_clib_for_base",
        "//strings",
        "//util/task:status",
    ],
)
