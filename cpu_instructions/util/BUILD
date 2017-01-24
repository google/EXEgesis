# Description:
#   Generic utility libraries.

package(
    default_visibility = ["//cpu_instructions:__subpackages__"],
)

licenses(["notice"])  # Apache 2.0


cc_library(
    name = "instruction_syntax",
    srcs = ["instruction_syntax.cc"],
    hdrs = ["instruction_syntax.h"],
    deps = [
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//strings",
        "//cpu_instructions/proto:instructions_proto",
    ],
)

cc_test(
    name = "instruction_syntax_test",
    size = "small",
    srcs = ["instruction_syntax_test.cc"],
    deps = [
        ":instruction_syntax",
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
        "//strings",
        "//cpu_instructions/proto:instructions_proto",
    ],
)
