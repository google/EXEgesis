# Description:
#   Classes for status values.

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "status",
    srcs = ["canonical_errors.cc"],
    hdrs = [
        "canonical_errors.h",
        "status.h",
        "status_macros.h",
    ],
    deps = [
        "//external:protobuf_clib_for_status",
        "//strings",
    ],
)

cc_library(
    name = "statusor",
    hdrs = ["statusor.h"],
    deps = [
        "//external:protobuf_clib_for_status",
    ],
)
