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
        ":assembler_disassembler_cc_proto",
        ":disassembler",
        ":inline_asm",
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//exegesis/util:strings",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
        "@llvm_git//:Core",
    ],
)

cc_test(
    name = "assembler_disassembler_test",
    size = "small",
    srcs = ["assembler_disassembler_test.cc"],
    deps = [
        ":assembler_disassembler",
        ":assembler_disassembler_cc_proto",
        "//exegesis/testing:test_util",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

proto_library(
    name = "assembler_disassembler_proto",
    srcs = ["assembler_disassembler.proto"],
    deps = [
        "//exegesis/proto:instructions_proto",
    ],
)

cc_proto_library(
    name = "assembler_disassembler_cc_proto",
    deps = [
        ":assembler_disassembler_proto",
    ],
)

# A wrapper around the LLVM disassembler.
cc_library(
    name = "disassembler",
    srcs = ["disassembler.cc"],
    hdrs = ["disassembler.h"],
    deps = [
        ":llvm_utils",
        "//base",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:optional",
        "@com_google_absl//absl/types:span",
        "@com_google_protobuf//:protobuf_lite",
        "@llvm_git//:MC",
        "@llvm_git//:MCDisassembler",
        "@llvm_git//:Support",
    ],
)

cc_test(
    name = "disassembler_test",
    size = "small",
    srcs = ["disassembler_test.cc"],
    deps = [
        ":disassembler",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@llvm_git//:X86UtilsAndDesc",
    ],
)

# Library for compiling inline assembly code.
cc_library(
    name = "inline_asm",
    srcs = ["inline_asm.cc"],
    hdrs = ["inline_asm.h"],
    deps = [
        ":llvm_utils",
        "//base",
        "//util/gtl:map_util",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
        "@llvm_git//:Core",
        "@llvm_git//:ExecutionEngine",
        "@llvm_git//:MCJIT",
        "@llvm_git//:Support",
    ],
)

cc_test(
    name = "inline_asm_test",
    size = "small",
    srcs = ["inline_asm_test.cc"],
    deps = [
        ":inline_asm",
        ":llvm_utils",
        "//base",
        "//exegesis/testing:test_util",
        "//exegesis/util:strings",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/status",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

# A library that contains all the LLVM targets necessary to initialize the LLVM
# subsystems.
cc_library(
    name = "llvm_targets",
    deps = [
        "@llvm_git//:X86AsmParser",
        "@llvm_git//:X86CodeGen",
        "@llvm_git//:X86Disassembler",
        "@llvm_git//:X86Info",
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
        ":llvm_targets",
        "//base",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:span",
        "@com_google_protobuf//:protobuf_lite",
        "@llvm_git//:Analysis",  # buildcleaner: keep
        "@llvm_git//:CodeGen",
        "@llvm_git//:Core",
        "@llvm_git//:MC",
        "@llvm_git//:Support",
        "@llvm_git//:Target",
    ],
)
