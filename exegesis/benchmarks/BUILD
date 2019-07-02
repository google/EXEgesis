# Description:
#   Benchmarks.

licenses(["notice"])  # Apache 2.0

cc_test(
    name = "mergeicmps_bench",
    srcs = ["mergeicmps_bench.cc"],
    copts = [
        "$(STACK_FRAME_UNLIMITED)",
    ],
    linkstatic = 1,
    deps = [
        "//base",
        "@com_github_glog_glog//:glog",
        "@com_google_benchmark//:benchmark",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf_lite",
    ],
)
