# Description:
#   Classes for status values.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "status",
    srcs = ["canonical_errors.cc"],
    hdrs = [
        "canonical_errors.h",
        "status.h",
        "status_macros.h",
    ],
    deps = [
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_library(
    name = "statusor",
    hdrs = ["statusor.h"],
    deps = [
        "@com_google_protobuf//:protobuf_lite",
    ],
)
