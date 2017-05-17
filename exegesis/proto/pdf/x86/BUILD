# Description:
#   Pdf Parser protos for x86.

package(default_visibility = ["//:internal_users"])

licenses(["notice"])  # Apache 2.0

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
