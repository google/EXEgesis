# Descripting:
#   Test utilities.

package(
    default_visibility = ["//visibility:public"],
)

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "test_util",
    testonly = 1,
    hdrs = ["test_util.h"],
    deps = [
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//external:protobuf_clib",
        
        "//external:googletest",
    ],
)

cc_library(
    name = "googletest",
    testonly = 1,
    deps = [
        ":test_util",
        "//external:googletest",
    ],
)

cc_library(
    name = "googletest_main",
    testonly = 1,
    deps = [
        ":googletest",
        "//external:googletest_main",
    ],
)
