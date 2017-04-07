# Description:
#   Libraries for interacting with LLVM.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

# Library for compiling inline assembly code.
cc_library(
    name = "inline_asm",
    srcs = ["inline_asm.cc"],
    hdrs = ["inline_asm.h"],
    deps = [
        "//base",
        "//cpu_instructions/llvm:llvm_utils",
        "//external:gflags",
        "//external:glog",
        "//external:llvm_execution_engine",
        "//external:llvm_ir",
        "//external:llvm_mcjit",
        "//external:llvm_support",
        "//external:protobuf_clib_for_base",
        "//strings",
        "//util/gtl:map_util",
        "//util/gtl:ptr_util",
    ],
)

cc_test(
    name = "inline_asm_test",
    size = "small",
    srcs = ["inline_asm_test.cc"],
    deps = [
        ":inline_asm",
        "//base",
        "//cpu_instructions/llvm:llvm_utils",
        "//cpu_instructions/util:strings",
        "//external:gflags",
        "//external:glog",
        "//external:googletest",
        "//external:googletest_main",
        "//external:llvm_ir",
        "//external:llvm_support",
        "//external:protobuf_clib_for_base",
    ],
)

# A library that contains all the LLVM targets necessary to initialize the LLVM
# subsystems.
cc_library(
    name = "llvm_targets",
    deps = [
        "//external:llvm_x86_target",
        "//external:llvm_x86_target_asm_parser",
        "//external:llvm_x86_target_disassembler",
        "//external:llvm_x86_target_info",
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
        "//cpu_instructions/llvm:llvm_targets",
        "//external:gflags",
        "//external:glog",
        "//external:llvm_analysis",  # buildcleaner: keep
        "//external:llvm_codegen",
        "//external:llvm_ir",
        "//external:llvm_machine_code",
        "//external:llvm_support",
        "//external:llvm_target_base",
        "//external:protobuf_clib_for_base",
        "//strings",
        "//util/task:status",
        "//util/task:statusor",
    ],
)
