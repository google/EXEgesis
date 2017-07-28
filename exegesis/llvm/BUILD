# Description:
#   Libraries for interacting with LLVM.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

# Library for assembling and disassembling inline assembly code.
cc_library(
    name = "assembler_disassembler",
    srcs = ["assembler_disassembler.cc"],
    hdrs = ["assembler_disassembler.h"],
    deps = [
        "//base",
        "//exegesis/llvm:disassembler",
        "//exegesis/llvm:inline_asm",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//exegesis/util:strings",
        "//strings",
        "//util/task:statusor",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@llvm_git//:ir",
    ],
)

cc_test(
    name = "assembler_disassembler_test",
    size = "small",
    srcs = ["assembler_disassembler_test.cc"],
    deps = [
        ":assembler_disassembler",
        "//base",
        "//exegesis/testing:test_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# A wrapper around the LLVM disassembler.
cc_library(
    name = "disassembler",
    srcs = ["disassembler.cc"],
    hdrs = ["disassembler.h"],
    deps = [
        "//base",
        "//exegesis/llvm:llvm_utils",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@llvm_git//:machine_code",
        "@llvm_git//:machine_code_disassembler",
        "@llvm_git//:support",
    ],
)

cc_test(
    name = "disassembler_test",
    size = "small",
    srcs = ["disassembler_test.cc"],
    deps = [
        ":disassembler",
        "//base",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
    ],
)

# Library for compiling inline assembly code.
cc_library(
    name = "inline_asm",
    srcs = ["inline_asm.cc"],
    hdrs = ["inline_asm.h"],
    deps = [
        "//base",
        "//exegesis/llvm:llvm_utils",
        "//strings",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@llvm_git//:execution_engine",
        "@llvm_git//:ir",
        "@llvm_git//:mcjit",
        "@llvm_git//:support",
    ],
)

cc_test(
    name = "inline_asm_test",
    size = "small",
    srcs = ["inline_asm_test.cc"],
    deps = [
        ":inline_asm",
        "//base",
        "//exegesis/llvm:llvm_utils",
        "//exegesis/util:strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@googletest_git//:gtest",
        "@googletest_git//:gtest_main",
        "@llvm_git//:ir",
        "@llvm_git//:support",
    ],
)

# A library that contains all the LLVM targets necessary to initialize the LLVM
# subsystems.
cc_library(
    name = "llvm_targets",
    deps = [
        "@llvm_git//:x86_target",
        "@llvm_git//:x86_target_asm_parser",
        "@llvm_git//:x86_target_disassembler",
        "@llvm_git//:x86_target_info",
    ],
    alwayslink = 1,
)

# A target that contains a library of helper functions for initializing LLVM and
# that holds all dependencies on LLVM packages (for now, for simplicity).
cc_library(
    name = "llvm_utils",
    srcs = ["llvm_utils.cc"],
    hdrs = ["llvm_utils.h"],
    deps = [
        "//base",
        "//exegesis/llvm:llvm_targets",
        "//strings",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@llvm_git//:analysis",  # buildcleaner: keep
        "@llvm_git//:codegen",
        "@llvm_git//:ir",
        "@llvm_git//:machine_code",
        "@llvm_git//:support",
        "@llvm_git//:target_base",
    ],
)
