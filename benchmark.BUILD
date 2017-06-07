cc_library(
    name = "benchmark",
    srcs = glob([
        "src/*.cc",
        "src/*.h",
        "include/**/*.h",
    ]),
    hdrs = glob([
        "include/benchmark.h",
    ]),
    includes = [
        "src",
        "include",
    ],
    visibility = ["//visibility:public"],
    copts= [
        "-DHAVE_POSIX_REGEX",
    ],
)
