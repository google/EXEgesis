# Contains functions and data structures that concern specifically the
# x86 and x86-64 platforms.

package(default_visibility = ["//cpu_instructions:__subpackages__"])

load("//cpu_instructions:proto_library.bzl", "cpu_instructions_proto_library")

# A collection of utilities used by the instruction set cleanup library.
cc_library(
    name = "cleanup_instruction_set_utils",
    srcs = ["cleanup_instruction_set_utils.cc"],
    hdrs = ["cleanup_instruction_set_utils.h"],
    deps = [
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//strings",
        "//cpu_instructions/base:cleanup_instruction_set",
        "//cpu_instructions/proto:instructions_proto",
    ],
)

cc_test(
    name = "cleanup_instruction_set_utils_test",
    size = "small",
    srcs = ["cleanup_instruction_set_utils_test.cc"],
    deps = [
        ":cleanup_instruction_set_utils",
        "//external:protobuf_clib",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
        "//cpu_instructions/proto:instructions_proto",
    ],
)

# A collection of libraries that implement the different cleanups of the
# instruction set. To make maintenance easier, each file has its own cc_library
# target and a separate test.
# The target cleanup_instruction_set_all integrates all the cleanup libraries
# into a single target.
cc_library(
    name = "cleanup_instruction_set_all",
    deps = [
        ":cleanup_instruction_set_alternatives",
        ":cleanup_instruction_set_asm_syntax",
        ":cleanup_instruction_set_encoding",
        ":cleanup_instruction_set_fix_operands",
        ":cleanup_instruction_set_operand_info",
        ":cleanup_instruction_set_operand_size_override",
        ":cleanup_instruction_set_properties",
        ":cleanup_instruction_set_removals",
        ":cleanup_instruction_set_utils",
    ],
    alwayslink = 1,
)

cc_library(
    name = "cleanup_instruction_set_alternatives",
    srcs = ["cleanup_instruction_set_alternatives.cc"],
    hdrs = ["cleanup_instruction_set_alternatives.h"],
    deps = [
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//strings",
        "//util/gtl:map_util",
        "//cpu_instructions/base:cleanup_instruction_set",
        "//cpu_instructions/proto:instructions_proto",
        "//util/task:status",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_alternatives_test",
    size = "small",
    srcs = ["cleanup_instruction_set_alternatives_test.cc"],
    deps = [
        ":cleanup_instruction_set_alternatives",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
        "//cpu_instructions/base:cleanup_instruction_set_test_utils",
    ],
)

cc_library(
    name = "cleanup_instruction_set_asm_syntax",
    srcs = ["cleanup_instruction_set_asm_syntax.cc"],
    hdrs = ["cleanup_instruction_set_asm_syntax.h"],
    deps = [
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//strings",
        "//util/gtl:map_util",
        "//cpu_instructions/base:cleanup_instruction_set",
        "//cpu_instructions/proto:instructions_proto",
        "//util/task:status",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_asm_syntax_test",
    size = "small",
    srcs = ["cleanup_instruction_set_asm_syntax_test.cc"],
    deps = [
        ":cleanup_instruction_set_asm_syntax",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
        "//cpu_instructions/base:cleanup_instruction_set_test_utils",
    ],
)

cc_library(
    name = "cleanup_instruction_set_encoding",
    srcs = ["cleanup_instruction_set_encoding.cc"],
    hdrs = ["cleanup_instruction_set_encoding.h"],
    deps = [
        ":cleanup_instruction_set_utils",
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//external:protobuf_clib",
        "//strings",
        "//util/gtl:map_util",
        "//cpu_instructions/base:cleanup_instruction_set",
        "//cpu_instructions/proto:instructions_proto",
        "//external:re2",
        "//util/task:status",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_encoding_test",
    size = "small",
    srcs = ["cleanup_instruction_set_encoding_test.cc"],
    deps = [
        ":cleanup_instruction_set_encoding",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
        "//cpu_instructions/base:cleanup_instruction_set_test_utils",
    ],
)

cc_library(
    name = "cleanup_instruction_set_fix_operands",
    srcs = ["cleanup_instruction_set_fix_operands.cc"],
    hdrs = ["cleanup_instruction_set_fix_operands.h"],
    deps = [
        ":cleanup_instruction_set_utils",
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//external:protobuf_clib",
        "//strings",
        "//util/gtl:map_util",
        "//cpu_instructions/base:cleanup_instruction_set",
        "//cpu_instructions/proto:instructions_proto",
        "//util/task:status",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_fix_operands_test",
    size = "small",
    srcs = ["cleanup_instruction_set_fix_operands_test.cc"],
    deps = [
        ":cleanup_instruction_set_fix_operands",
        "//external:protobuf_clib",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
        "//cpu_instructions/base:cleanup_instruction_set_test_utils",
        "//util/task:status",
    ],
)

cc_library(
    name = "cleanup_instruction_set_operand_info",
    srcs = ["cleanup_instruction_set_operand_info.cc"],
    hdrs = ["cleanup_instruction_set_operand_info.h"],
    deps = [
        ":encoding_specification",
        ":encoding_specification_proto",
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//strings",
        "//util/gtl:map_util",
        "//cpu_instructions/base:cleanup_instruction_set",
        "//cpu_instructions/proto:instructions_proto",
        "//util/task:status",
        "//util/task:statusor",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_operand_info_test",
    size = "small",
    srcs = ["cleanup_instruction_set_operand_info_test.cc"],
    deps = [
        ":cleanup_instruction_set_operand_info",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
        "//cpu_instructions/base:cleanup_instruction_set_test_utils",
    ],
)

cc_library(
    name = "cleanup_instruction_set_operand_size_override",
    srcs = ["cleanup_instruction_set_operand_size_override.cc"],
    hdrs = ["cleanup_instruction_set_operand_size_override.h"],
    deps = [
        ":cleanup_instruction_set_utils",
        ":encoding_specification",
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//strings",
        "//util/gtl:map_util",
        "//cpu_instructions/base:cleanup_instruction_set",
        "//cpu_instructions/proto:instructions_proto",
        "//cpu_instructions/util:instruction_syntax",
        "//util/task:status",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_operand_size_override_test",
    size = "small",
    srcs = ["cleanup_instruction_set_operand_size_override_test.cc"],
    deps = [
        ":cleanup_instruction_set_operand_size_override",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
        "//cpu_instructions/base:cleanup_instruction_set_test_utils",
    ],
)

cc_library(
    name = "cleanup_instruction_set_properties",
    srcs = ["cleanup_instruction_set_properties.cc"],
    hdrs = ["cleanup_instruction_set_properties.h"],
    deps = [
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//util/gtl:map_util",
        "//cpu_instructions/base:cleanup_instruction_set",
        "//cpu_instructions/proto:instructions_proto",
        "//util/task:status",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_properties_test",
    size = "small",
    srcs = ["cleanup_instruction_set_properties_test.cc"],
    deps = [
        ":cleanup_instruction_set_properties",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
        "//cpu_instructions/base:cleanup_instruction_set_test_utils",
    ],
)

cc_library(
    name = "cleanup_instruction_set_removals",
    srcs = ["cleanup_instruction_set_removals.cc"],
    hdrs = ["cleanup_instruction_set_removals.h"],
    deps = [
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//external:protobuf_clib",
        "//strings",
        "//util/gtl:container_algorithm",
        "//util/gtl:map_util",
        "//cpu_instructions/base:cleanup_instruction_set",
        "//cpu_instructions/proto:instructions_proto",
        "//util/task:status",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_removals_test",
    size = "small",
    srcs = ["cleanup_instruction_set_removals_test.cc"],
    deps = [
        ":cleanup_instruction_set_removals",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
        "//cpu_instructions/base:cleanup_instruction_set_test_utils",
    ],
)


# Protocol buffer library with the definitions of enums and constants for the
# x86-64 binary encoding.
cpu_instructions_proto_library(
    name = "instruction_encoding_proto",
    srcs = ["instruction_encoding.proto"],
    cc_api_version = 2,
)


# A library for working with the instruction encoding specification used in the
# Intel x86-64 reference manual.
cpu_instructions_proto_library(
    name = "encoding_specification_proto",
    srcs = ["encoding_specification.proto"],
    cc_api_version = 2,
    deps = [":instruction_encoding_proto"],
)

cc_library(
    name = "encoding_specification",
    srcs = ["encoding_specification.cc"],
    hdrs = ["encoding_specification.h"],
    deps = [
        ":encoding_specification_proto",
        ":instruction_encoding_proto",
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//strings",
        "//util/gtl:map_util",
        "//cpu_instructions/proto:instructions_proto",
        "//external:re2",
        "//util/task:status",
        "//util/task:statusor",
    ],
)

cc_test(
    name = "encoding_specification_test",
    size = "small",
    srcs = ["encoding_specification_test.cc"],
    deps = [
        ":encoding_specification",
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
        "//strings",
        "//cpu_instructions/base:cleanup_instruction_set",
        "//util/task:status",
        "//util/task:statusor",
    ],
)

