licenses(["notice"])  # Apache 2.0

package(default_visibility = ["//visibility:private"])

cc_library(
    name = "component",
    srcs = ["component.cc"],
    hdrs = ["component.h"],
    visibility = [
        "//llvm_sim/components:__pkg__",
        "//llvm_sim/x86:__pkg__",
    ],
    deps = [":context"],
)

cc_test(
    name = "component_test",
    srcs = ["component_test.cc"],
    deps = [
        ":component",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "context",
    srcs = ["context.cc"],
    hdrs = ["context.h"],
    visibility = [
        "//llvm_sim/analysis:__pkg__",
        "//llvm_sim/components:__pkg__",
        "//llvm_sim/x86:__pkg__",
    ],
    deps = [
        "@llvm_git//:codegen",
        "@llvm_git//:machine_code",
        "@llvm_git//:support",
    ],
)

cc_test(
    name = "context_test",
    srcs = ["context_test.cc"],
    deps = [
        ":context",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "log",
    srcs = ["log.cc"],
    hdrs = ["log.h"],
    visibility = [
        "//llvm_sim/analysis:__pkg__",
        "//llvm_sim/x86:__pkg__",
    ],
    deps = [
        ":component",
    ],
)

cc_library(
    name = "log_levels",
    srcs = ["log_levels.cc"],
    hdrs = ["log_levels.h"],
    visibility = [
        "//llvm_sim/analysis:__pkg__",
        "//llvm_sim/components:__pkg__",
        "//llvm_sim/x86:__pkg__",
    ],
)

cc_library(
    name = "simulator",
    srcs = ["simulator.cc"],
    hdrs = ["simulator.h"],
    visibility = ["//llvm_sim/x86:__pkg__"],
    deps = [
        ":component",
        ":context",
        ":log",
        "@llvm_git//:machine_code",
        "@llvm_git//:support",
    ],
)

cc_test(
    name = "simulator_test",
    srcs = ["simulator_test.cc"],
    deps = [
        ":component",
        ":simulator",
        "@com_google_googletest//:gtest_main",
    ],
)
