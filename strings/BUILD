# Description:
#   String manipulation functions.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "strings",
    hdrs = [
        "ascii_ctype.h",
        "case.h",
        "str_cat.h",
        "str_join.h",
        "str_replace.h",
        "str_split.h",
        "string_view.h",
        "string_view_utils.h",
        "strip.h",
    ],
    deps = [
        "@com_google_protobuf//:protobuf_lite",
    ],
)
