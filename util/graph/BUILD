# Description:
#   Graph utilities.

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "connected_components",
    srcs = ["connected_components.cc"],
    hdrs = ["connected_components.h"],
    deps = [
        "//base",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
        "@gflags_git//:gflags",
        "@glog_git//:glog",
        "@protobuf_git//:protobuf_lite",
    ],
)
