# Description:
#   General utility functions.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "container_algorithm",
    hdrs = ["container_algorithm.h"],
)

cc_library(
    name = "map_util",
    hdrs = ["map_util.h"],
    deps = [
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_library(
    name = "ptr_util",
    hdrs = ["ptr_util.h"],
)
