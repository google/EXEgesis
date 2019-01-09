licenses(["notice"])  # Apache 2.0

package(
    default_visibility = ["//visibility:private"],
)

cc_library(
    name = "inverse_throughput",
    srcs = ["inverse_throughput.cc"],
    hdrs = ["inverse_throughput.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//llvm_sim/framework:context",
        "//llvm_sim/framework:log",
    ],
)

cc_test(
    name = "inverse_throughput_test",
    srcs = ["inverse_throughput_test.cc"],
    deps = [
        ":inverse_throughput",
        "//llvm_sim/framework:context",
        "//llvm_sim/framework:log",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "port_pressure",
    srcs = ["port_pressure.cc"],
    hdrs = ["port_pressure.h"],
    visibility = [
        "//llvm_sim/x86:__pkg__",
    ],
    deps = [
        "//llvm_sim/framework:context",
        "//llvm_sim/framework:log",
        "@llvm_git//:support",
    ],
)

cc_test(
    name = "port_pressure_test",
    srcs = ["port_pressure_test.cc"],
    deps = [
        ":port_pressure",
        "//llvm_sim/framework:context",
        "//llvm_sim/framework:log",
        "@com_google_googletest//:gtest_main",
    ],
)
