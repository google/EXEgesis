# Description:
#   Pdf Parser protos for x86.

package(default_visibility = ["//:internal_users"])

load("//cpu_instructions:proto_library.bzl", "cpu_instructions_proto_library")

licenses(["notice"])  # Apache 2.0

cpu_instructions_proto_library(
    name = "intel_sdm_proto",
    srcs = ["intel_sdm.proto"],
    cc_api_version = 2,
    deps = [
        "//cpu_instructions/proto:instructions_proto",
        "//cpu_instructions/proto/pdf:pdf_document_proto",
    ],
)
