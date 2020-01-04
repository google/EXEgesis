# Contains functions and data structures that concern specifically the
# x86 and x86-64 platforms.

package(default_visibility = ["//:internal_users"])

licenses(["notice"])  # Apache 2.0

# A collection of utilities used by the instruction set cleanup library.
cc_library(
    name = "cleanup_instruction_set_utils",
    srcs = ["cleanup_instruction_set_utils.cc"],
    hdrs = ["cleanup_instruction_set_utils.h"],
    deps = [
        "//base",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto/x86:encoding_specification_cc_proto",
        "//exegesis/proto/x86:instruction_encoding_cc_proto",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "cleanup_instruction_set_utils_test",
    size = "small",
    srcs = ["cleanup_instruction_set_utils_test.cc"],
    deps = [
        ":cleanup_instruction_set_utils",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto/x86:encoding_specification_cc_proto",
        "//exegesis/testing:test_util",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

# A collection of libraries that implement the different cleanups of the
# instruction set. To make maintenance easier, each file has its own cc_library
# target and a separate test.
# The target cleanup_instruction_set_all integrates all the cleanup libraries
# into a single target.
cc_library(
    name = "cleanup_instruction_set_all",
    visibility = ["//visibility:public"],
    deps = [
        ":cleanup_instruction_set_alternatives",
        ":cleanup_instruction_set_checks",
        ":cleanup_instruction_set_encoding",
        ":cleanup_instruction_set_evex",
        ":cleanup_instruction_set_fix_operands",
        ":cleanup_instruction_set_merge_instructions",
        ":cleanup_instruction_set_missing_instructions",
        ":cleanup_instruction_set_operand_info",
        ":cleanup_instruction_set_operand_size_override",
        ":cleanup_instruction_set_properties",
        ":cleanup_instruction_set_removals",
        ":cleanup_instruction_set_rex",
        ":cleanup_instruction_set_utils",
    ],
    alwayslink = 1,
)

cc_library(
    name = "cleanup_instruction_set_alternatives",
    srcs = ["cleanup_instruction_set_alternatives.cc"],
    hdrs = ["cleanup_instruction_set_alternatives.h"],
    deps = [
        "//base",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//util/gtl:map_util",
        "//util/task:status",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_alternatives_test",
    size = "small",
    srcs = ["cleanup_instruction_set_alternatives_test.cc"],
    deps = [
        ":cleanup_instruction_set_alternatives",
        "//exegesis/base:cleanup_instruction_set_test_utils",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "cleanup_instruction_set_checks",
    srcs = ["cleanup_instruction_set_checks.cc"],
    hdrs = ["cleanup_instruction_set_checks.h"],
    deps = [
        ":instruction_set_utils",
        "//base",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto:registers_cc_proto",
        "//exegesis/proto/x86:encoding_specification_cc_proto",
        "//exegesis/proto/x86:instruction_encoding_cc_proto",
        "//exegesis/util:bits",
        "//exegesis/util:category_util",
        "//exegesis/util:instruction_syntax",
        "//exegesis/util:status_util",
        "//util/gtl:map_util",
        "//util/task:status",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_protobuf//:protobuf_lite",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_checks_test",
    size = "small",
    srcs = ["cleanup_instruction_set_checks_test.cc"],
    deps = [
        ":cleanup_instruction_set_checks",
        "//exegesis/testing:test_util",
        "//exegesis/util:proto_util",
        "//util/task:status",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "cleanup_instruction_set_encoding",
    srcs = ["cleanup_instruction_set_encoding.cc"],
    hdrs = ["cleanup_instruction_set_encoding.h"],
    deps = [
        ":cleanup_instruction_set_utils",
        ":encoding_specification",
        "//base",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto/x86:encoding_specification_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//exegesis/util:status_util",
        "//util/gtl:map_util",
        "//util/task:status",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_encoding_test",
    size = "small",
    srcs = ["cleanup_instruction_set_encoding_test.cc"],
    deps = [
        ":cleanup_instruction_set_encoding",
        "//exegesis/base:cleanup_instruction_set_test_utils",
        "//exegesis/testing:test_util",
        "//util/task:status",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "cleanup_instruction_set_evex",
    srcs = ["cleanup_instruction_set_evex.cc"],
    hdrs = ["cleanup_instruction_set_evex.h"],
    deps = [
        "//base",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto/x86:encoding_specification_cc_proto",
        "//exegesis/util:category_util",
        "//exegesis/util:instruction_syntax",
        "//util/gtl:map_util",
        "//util/task:status",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_evex_test",
    size = "small",
    srcs = ["cleanup_instruction_set_evex_test.cc"],
    deps = [
        ":cleanup_instruction_set_evex",
        "//base",
        "//exegesis/base:cleanup_instruction_set_test_utils",
        "@com_github_glog_glog//:glog",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_library(
    name = "cleanup_instruction_set_fix_operands",
    srcs = ["cleanup_instruction_set_fix_operands.cc"],
    hdrs = ["cleanup_instruction_set_fix_operands.h"],
    deps = [
        ":cleanup_instruction_set_utils",
        "//base",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//util/gtl:map_util",
        "//util/task:status",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_fix_operands_test",
    size = "small",
    srcs = ["cleanup_instruction_set_fix_operands_test.cc"],
    deps = [
        ":cleanup_instruction_set_fix_operands",
        "//exegesis/base:cleanup_instruction_set_test_utils",
        "//util/task:status",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "cleanup_instruction_set_merge_instructions",
    srcs = ["cleanup_instruction_set_merge_instructions.cc"],
    hdrs = ["cleanup_instruction_set_merge_instructions.h"],
    deps = [
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//exegesis/util:status_util",
        "//util/task:status",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/strings",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_merge_instructions_test",
    size = "small",
    srcs = ["cleanup_instruction_set_merge_instructions_test.cc"],
    deps = [
        ":cleanup_instruction_set_merge_instructions",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/base:cleanup_instruction_set_test_utils",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "cleanup_instruction_set_missing_instructions",
    srcs = ["cleanup_instruction_set_missing_instructions.cc"],
    hdrs = ["cleanup_instruction_set_missing_instructions.h"],
    deps = [
        "//base",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//exegesis/util:proto_util",
        "//util/task:status",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_missing_instructions_test",
    size = "small",
    srcs = ["cleanup_instruction_set_missing_instructions_test.cc"],
    deps = [
        ":cleanup_instruction_set_missing_instructions",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/base:cleanup_instruction_set_test_utils",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "cleanup_instruction_set_operand_info",
    srcs = ["cleanup_instruction_set_operand_info.cc"],
    hdrs = ["cleanup_instruction_set_operand_info.h"],
    deps = [
        ":encoding_specification",
        "//base",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto:registers_cc_proto",
        "//exegesis/proto/x86:encoding_specification_cc_proto",
        "//exegesis/util:category_util",
        "//exegesis/util:instruction_syntax",
        "//exegesis/util:status_util",
        "//util/gtl:map_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_operand_info_test",
    size = "small",
    srcs = ["cleanup_instruction_set_operand_info_test.cc"],
    deps = [
        ":cleanup_instruction_set_operand_info",
        "//exegesis/base:cleanup_instruction_set_test_utils",
        "//exegesis/testing:test_util",
        "//util/task:status",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "cleanup_instruction_set_operand_size_override",
    srcs = ["cleanup_instruction_set_operand_size_override.cc"],
    hdrs = ["cleanup_instruction_set_operand_size_override.h"],
    deps = [
        ":cleanup_instruction_set_utils",
        ":encoding_specification",
        "//base",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/base:prettyprint",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto/x86:instruction_encoding_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//util/gtl:map_util",
        "//util/task:status",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_operand_size_override_test",
    size = "small",
    srcs = ["cleanup_instruction_set_operand_size_override_test.cc"],
    deps = [
        ":cleanup_instruction_set_operand_size_override",
        "//exegesis/base:cleanup_instruction_set_test_utils",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "cleanup_instruction_set_properties",
    srcs = ["cleanup_instruction_set_properties.cc"],
    hdrs = ["cleanup_instruction_set_properties.h"],
    deps = [
        "//base",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//util/gtl:map_util",
        "//util/task:status",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_protobuf//:protobuf_lite",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_properties_test",
    size = "small",
    srcs = ["cleanup_instruction_set_properties_test.cc"],
    deps = [
        ":cleanup_instruction_set_properties",
        "//exegesis/base:cleanup_instruction_set_test_utils",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "cleanup_instruction_set_rex",
    srcs = ["cleanup_instruction_set_rex.cc"],
    hdrs = ["cleanup_instruction_set_rex.h"],
    deps = [
        ":cleanup_instruction_set_utils",
        "//base",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/base:opcode",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto/x86:encoding_specification_cc_proto",
        "//exegesis/proto/x86:instruction_encoding_cc_proto",
        "//util/task:status",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_rex_test",
    size = "small",
    srcs = ["cleanup_instruction_set_rex_test.cc"],
    deps = [
        ":cleanup_instruction_set_rex",
        "//exegesis/base:cleanup_instruction_set_test_utils",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "cleanup_instruction_set_removals",
    srcs = ["cleanup_instruction_set_removals.cc"],
    hdrs = ["cleanup_instruction_set_removals.h"],
    deps = [
        "//base",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//exegesis/util:status_util",
        "//util/gtl:map_util",
        "//util/task:status",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/algorithm:container",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/container:node_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cleanup_instruction_set_removals_test",
    size = "small",
    srcs = ["cleanup_instruction_set_removals_test.cc"],
    deps = [
        ":cleanup_instruction_set_removals",
        "//exegesis/base:cleanup_instruction_set_test_utils",
        "//exegesis/testing:test_util",
        "//exegesis/util:proto_util",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# Utilities to represent the CPU state.
cc_library(
    name = "cpu_state",
    srcs = ["cpu_state.cc"],
    hdrs = ["cpu_state.h"],
    deps = [
        "//base",
        "//exegesis/util:bits",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "cpu_state_test",
    srcs = ["cpu_state_test.cc"],
    deps = [
        ":cpu_state",
        "//base",
        "@com_github_glog_glog//:glog",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

# A library for working with the instruction encoding specification used in the
# Intel x86-64 reference manual.
cc_library(
    name = "encoding_specification",
    srcs = ["encoding_specification.cc"],
    hdrs = ["encoding_specification.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":instruction_encoding_constants",
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto/x86:encoding_specification_cc_proto",
        "//exegesis/proto/x86:instruction_encoding_cc_proto",
        "//util/gtl:map_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/algorithm:container",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_test(
    name = "encoding_specification_test",
    size = "small",
    srcs = ["encoding_specification_test.cc"],
    deps = [
        ":encoding_specification",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/testing:test_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# An instruction encoder that generates x86-64 binary code.
cc_library(
    name = "instruction_encoder",
    srcs = ["instruction_encoder.cc"],
    hdrs = ["instruction_encoder.h"],
    deps = [
        ":instruction_encoding",
        "//base",
        "//exegesis/proto/x86:decoded_instruction_cc_proto",
        "//exegesis/proto/x86:encoding_specification_cc_proto",
        "//exegesis/util:bits",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "instruction_encoder_test",
    size = "small",
    srcs = ["instruction_encoder_test.cc"],
    deps = [
        ":architecture",
        ":instruction_encoder",
        ":instruction_encoding_test_utils",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/testing:test_util",
        "//exegesis/util:proto_util",
        "//util/task:statusor",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/types:span",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# A parser for the x86-64 instruction binary encoding.
cc_library(
    name = "instruction_parser",
    srcs = ["instruction_parser.cc"],
    hdrs = ["instruction_parser.h"],
    deps = [
        ":architecture",
        ":instruction_encoding",
        "//base",
        "//exegesis/base:opcode",
        "//exegesis/proto/x86:encoding_specification_cc_proto",
        "//exegesis/proto/x86:instruction_encoding_cc_proto",
        "//exegesis/util:bits",
        "//exegesis/util:strings",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/algorithm:container",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:span",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "instruction_parser_test",
    size = "small",
    srcs = ["instruction_parser_test.cc"],
    deps = [
        ":instruction_encoder",
        ":instruction_parser",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto/x86:decoded_instruction_cc_proto",
        "//exegesis/testing:test_util",
        "//exegesis/util:strings",
        "//util/task:statusor",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

# A library that contains constants relating to the encoding of x86
# instructions.
cc_library(
    name = "instruction_encoding_constants",
    hdrs = ["instruction_encoding_constants.h"],
)

# A library of helper functions for working with the x86-64 instruction encoding
# and the DecodedInstruction structure.
cc_library(
    name = "instruction_encoding",
    srcs = ["instruction_encoding.cc"],
    hdrs = ["instruction_encoding.h"],
    deps = [
        ":encoding_specification",
        ":instruction_encoding_constants",
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto/x86:decoded_instruction_cc_proto",
        "//exegesis/proto/x86:encoding_specification_cc_proto",
        "//exegesis/proto/x86:instruction_encoding_cc_proto",
        "//exegesis/util:bits",
        "//exegesis/util:category_util",
        "//exegesis/util:index_type",
        "//exegesis/util:instruction_syntax",
        "//util/gtl:map_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/algorithm:container",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "instruction_encoding_test",
    size = "small",
    srcs = ["instruction_encoding_test.cc"],
    deps = [
        ":encoding_specification",
        ":instruction_encoding",
        ":instruction_encoding_test_utils",
        "//exegesis/proto/x86:encoding_specification_cc_proto",
        "//exegesis/testing:test_util",
        "//exegesis/util:proto_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

# A library of helper functions for testing the instruction encoding code.
cc_library(
    name = "instruction_encoding_test_utils",
    testonly = 1,
    srcs = ["instruction_encoding_test_utils.cc"],
    hdrs = ["instruction_encoding_test_utils.h"],
    deps = [
        ":encoding_specification",
        ":instruction_encoder",
        "//exegesis/llvm:assembler_disassembler",
        "//exegesis/proto/x86:decoded_instruction_cc_proto",
        "//exegesis/proto/x86:encoding_specification_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//exegesis/util:strings",
        "//util/task:status",
        "//util/task:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "instruction_encoding_test_utils_test",
    size = "small",
    srcs = ["instruction_encoding_test_utils_test.cc"],
    deps = [
        ":instruction_encoding_test_utils",
        "//exegesis/proto/x86:decoded_instruction_cc_proto",
        "//exegesis/util:proto_util",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# A wrapper class around the x86-64 architecture proto.
cc_library(
    name = "architecture",
    srcs = ["architecture.cc"],
    hdrs = ["architecture.h"],
    deps = [
        ":instruction_encoding",
        "//base",
        "//exegesis/base:architecture",
        "//exegesis/base:opcode",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto/x86:decoded_instruction_cc_proto",
        "//exegesis/proto/x86:encoding_specification_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//util/gtl:map_util",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "architecture_test",
    size = "small",
    srcs = ["architecture_test.cc"],
    deps = [
        ":architecture",
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:proto_util",
        "//util/gtl:map_util",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

# A library with helper functions for working with the instruction set.
cc_library(
    name = "instruction_set_utils",
    srcs = ["instruction_set_utils.cc"],
    hdrs = ["instruction_set_utils.h"],
    deps = ["@com_google_absl//absl/container:flat_hash_set"],
)

# A library that contains information about the x86-64 microarchitectures.
cc_library(
    name = "microarchitectures",
    srcs = ["microarchitectures.cc"],
    hdrs = ["microarchitectures.h"],
    deps = [
        "//base",
        "//exegesis/base:microarchitecture",
        "//exegesis/proto:microarchitecture_cc_proto",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
    ],
    alwayslink = 1,
)

cc_test(
    name = "microarchitectures_test",
    srcs = ["microarchitectures_test.cc"],
    deps = [
        ":microarchitectures",
        "//exegesis/base:microarchitecture",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# A library that contains information about the x86-64 registers.
cc_library(
    name = "registers",
    srcs = ["registers.cc"],
    hdrs = ["registers.h"],
    deps = [
        "//exegesis/base:registers",
        "//exegesis/proto:registers_cc_proto",
        "//exegesis/util:proto_util",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "registers_test",
    size = "small",
    srcs = ["registers_test.cc"],
    deps = [
        ":registers",
        "//exegesis/testing:test_util",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "operand_translator",
    srcs = ["operand_translator.cc"],
    hdrs = ["operand_translator.h"],
    deps = [
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//util/gtl:map_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "operand_translator_test",
    size = "small",
    srcs = ["operand_translator_test.cc"],
    deps = [
        ":operand_translator",
        "//exegesis/testing:test_util",
        "//exegesis/util:proto_util",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
