licenses(["notice"])  # Apache 2.0

package(default_visibility = ["//:internal_users"])

cc_library(
    name = "buffer",
    hdrs = ["buffer.h"],
    deps = [
        ":common",
        "//llvm_sim/framework:component",
        "//llvm_sim/framework:log_levels",
        "@llvm_git//:support",
    ],
)

cc_test(
    name = "buffer_test",
    srcs = ["buffer_test.cc"],
    deps = [
        ":buffer",
        ":testing",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "common",
    srcs = ["common.cc"],
    hdrs = ["common.h"],
    deps = [
        "//llvm_sim/framework:component",
        "@llvm_git//:support",
    ],
)

cc_library(
    name = "decoder",
    srcs = ["decoder.cc"],
    hdrs = ["decoder.h"],
    deps = [
        ":common",
        "//llvm_sim/framework:component",
        "@llvm_git//:machine_code",
    ],
)

cc_test(
    name = "decoder_test",
    srcs = ["decoder_test.cc"],
    deps = [
        ":common",
        ":decoder",
        ":testing",
        "//llvm_sim/framework:context",
        "@com_google_googletest//:gtest_main",
        "@llvm_git//:codegen",
        "@llvm_git//:machine_code",
    ],
)

cc_library(
    name = "dispatch_port",
    hdrs = ["dispatch_port.h"],
    deps = [":buffer"],
)

cc_test(
    name = "dispatch_port_test",
    srcs = ["dispatch_port_test.cc"],
    deps = [
        ":common",
        ":dispatch_port",
        ":testing",
        "//llvm_sim/framework:context",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "execution_unit",
    hdrs = ["execution_unit.h"],
    deps = [
        ":common",
        "//llvm_sim/framework:component",
        "@llvm_git//:support",
    ],
)

cc_test(
    name = "execution_unit_test",
    srcs = ["execution_unit_test.cc"],
    deps = [
        ":common",
        ":execution_unit",
        ":testing",
        "//llvm_sim/framework:context",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "fetcher",
    srcs = ["fetcher.cc"],
    hdrs = ["fetcher.h"],
    deps = [
        ":buffer",
        "//llvm_sim/framework:component",
        "@llvm_git//:machine_code",
    ],
)

cc_test(
    name = "fetcher_test",
    srcs = ["fetcher_test.cc"],
    deps = [
        ":fetcher",
        ":testing",
        "//llvm_sim/framework:context",
        "@com_google_googletest//:gtest_main",
        "@llvm_git//:machine_code",
    ],
)

cc_library(
    name = "issue_policy",
    srcs = ["issue_policy.cc"],
    hdrs = ["issue_policy.h"],
    deps = [
        "@com_google_absl//absl/memory",
        "@llvm_git//:support",
    ],
)

cc_test(
    name = "issue_policy_test",
    srcs = ["issue_policy_test.cc"],
    deps = [
        ":issue_policy",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "parser",
    srcs = ["parser.cc"],
    hdrs = ["parser.h"],
    deps = [
        ":common",
        "//llvm_sim/framework:component",
    ],
)

cc_test(
    name = "parser_test",
    srcs = ["parser_test.cc"],
    deps = [
        ":common",
        ":parser",
        ":testing",
        "//llvm_sim/framework:context",
        "@com_google_googletest//:gtest_main",
        "@llvm_git//:machine_code",
    ],
)

cc_library(
    name = "port",
    hdrs = ["port.h"],
    deps = [
        ":buffer",
        ":common",
        "//llvm_sim/framework:component",
        "@llvm_git//:support",
    ],
)

cc_test(
    name = "port_test",
    srcs = ["port_test.cc"],
    deps = [
        ":common",
        ":port",
        ":testing",
        "//llvm_sim/framework:context",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "register_renamer",
    srcs = ["register_renamer.cc"],
    hdrs = ["register_renamer.h"],
    deps = [
        ":common",
        "//llvm_sim/framework:component",
        "@llvm_git//:machine_code",
        "@llvm_git//:support",
    ],
)

cc_test(
    name = "register_renamer_test",
    srcs = ["register_renamer_test.cc"],
    features = [
        # TODO(courbet): Bazel rules prevent us from accessing X86InstrInfo.h but in
        # cmake we should be good.
        "-layering_check",
    ],
    deps = [
        ":common",
        ":register_renamer",
        ":testing",
        "//llvm_sim/framework:context",
        "@com_google_googletest//:gtest_main",
        "@llvm_git//:machine_code",
        "@llvm_git//:support",
        "@llvm_git//:x86_target",  # buildcleaner: keep
        "@llvm_git//:x86_target_info",  # buildcleaner: keep
    ],
)

cc_library(
    name = "reorder_buffer",
    srcs = ["reorder_buffer.cc"],
    hdrs = ["reorder_buffer.h"],
    deps = [
        ":common",
        ":issue_policy",
        "//llvm_sim/framework:component",
        "@com_google_absl//absl/container:flat_hash_map",
        "@llvm_git//:support",
    ],
)

cc_test(
    name = "reorder_buffer_test",
    srcs = ["reorder_buffer_test.cc"],
    deps = [
        ":common",
        ":reorder_buffer",
        ":testing",
        "//llvm_sim/framework:context",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "retirer",
    hdrs = ["retirer.h"],
    deps = [
        ":common",
        "//llvm_sim/framework:component",
        "@llvm_git//:machine_code",
    ],
)

cc_test(
    name = "retirer_test",
    srcs = ["retirer_test.cc"],
    deps = [
        ":common",
        ":retirer",
        ":testing",
        "//llvm_sim/framework:context",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "simplified_execution_units",
    hdrs = ["simplified_execution_units.h"],
    deps = [
        ":common",
        "//llvm_sim/framework:component",
        "@llvm_git//:support",
    ],
)

cc_test(
    name = "simplified_execution_units_test",
    srcs = ["simplified_execution_units_test.cc"],
    deps = [
        ":common",
        ":simplified_execution_units",
        ":testing",
        "//llvm_sim/framework:context",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "testing",
    testonly = 1,
    srcs = ["testing.cc"],
    hdrs = ["testing.h"],
    deps = [
        ":common",
        ":reorder_buffer",
        "//llvm_sim/framework:component",
        "@com_google_googletest//:gtest",
    ],
)
