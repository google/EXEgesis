# Description:
#   Graph utilities.

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "connected_components",
    srcs = ["connected_components.cc"],
    hdrs = ["connected_components.h"],
    deps = [
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
    ],
)
