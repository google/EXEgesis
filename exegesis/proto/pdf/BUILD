# Description:
#   Pdf Parser protos.

package(default_visibility = ["//:internal_users"])

licenses(["notice"])  # Apache 2.0

proto_library(
    name = "pdf_document_proto",
    srcs = ["pdf_document.proto"],
)

cc_proto_library(
    name = "pdf_document_cc_proto",
    deps = [":pdf_document_proto"],
)
