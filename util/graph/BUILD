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
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/hash",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/meta:type_traits",
        "@com_google_protobuf//:protobuf_lite",
    ],
)
