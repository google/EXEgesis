licenses(["notice"])  # Apache 2.0

package(default_visibility = ["//:internal_users"])

cc_library(
    name = "component",
    srcs = ["component.cc"],
    hdrs = ["component.h"],
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
    deps = [
        "@com_google_absl//absl/container:flat_hash_map",
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
    deps = [
        ":component",
    ],
)

cc_library(
    name = "log_levels",
    srcs = ["log_levels.cc"],
    hdrs = ["log_levels.h"],
)

cc_library(
    name = "simulator",
    srcs = ["simulator.cc"],
    hdrs = ["simulator.h"],
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
