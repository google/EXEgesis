# Contains classes, functions and data structures that are platform independent.
# Code in this directory is only allowed to depend on proto/ and util/.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "init_main",
    srcs = ["init_main.cc"],
    hdrs = ["init_main.h"],
    deps = [
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/flags:parse",
    ],
)

cc_library(
    name = "architecture",
    srcs = ["architecture.cc"],
    hdrs = ["architecture.h"],
    deps = [
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:index_type",
        "//util/gtl:map_util",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/memory",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "architecture_test",
    size = "small",
    srcs = ["architecture_test.cc"],
    deps = [
        ":architecture",
        "//exegesis/testing:test_util",
        "//exegesis/util:proto_util",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "architecture_provider",
    srcs = ["architecture_provider.cc"],
    hdrs = ["architecture_provider.h"],
    deps = [
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:proto_util",
        "//util/gtl:map_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "architecture_provider_test",
    size = "small",
    srcs = ["architecture_provider_test.cc"],
    deps = [
        ":architecture_provider",
        "//exegesis/testing:test_util",
        "//exegesis/util:proto_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# Framework for writing procedural clean-ups of an instruction set.
cc_library(
    name = "cleanup_instruction_set",
    srcs = ["cleanup_instruction_set.cc"],
    hdrs = ["cleanup_instruction_set.h"],
    deps = [
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//util/gtl:map_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/flags:flag",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
    ],
    alwayslink = 1,
)

cc_library(
    name = "cleanup_instruction_set_test_utils",
    testonly = 1,
    srcs = ["cleanup_instruction_set_test_utils.cc"],
    hdrs = ["cleanup_instruction_set_test_utils.h"],
    deps = [
        ":cleanup_instruction_set",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/testing:test_util",
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "cleanup_instruction_set_test",
    size = "small",
    srcs = ["cleanup_instruction_set_test.cc"],
    deps = [
        ":cleanup_instruction_set",
        ":cleanup_instruction_set_test_utils",
        "//base",
        "//exegesis/testing:test_util",
        "//util/task:status",
        "@com_github_glog_glog//:glog",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

# Utilities for working with CPUID data.
cc_library(
    name = "cpuid",
    srcs = [
        "cpuid.cc",
        "cpuid_x86.cc",
    ],
    hdrs = [
        "cpuid.h",
        "cpuid_x86.h",
    ],
    deps = [
        ":cpu_info",
        "//base",
        "//exegesis/proto:cpuid_cc_proto",
        "//exegesis/proto/x86:cpuid_cc_proto",
        "//exegesis/util:structured_register",
        "//util/gtl:map_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_test(
    name = "cpuid_test",
    size = "small",
    srcs = [
        "cpuid_test.cc",
        "cpuid_x86_test.cc",
    ],
    deps = [
        ":cpu_info",
        ":cpuid",
        "//exegesis/testing:test_util",
        "//exegesis/util:proto_util",
        "//util/task:status",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# A library that represents the host CPU info.
cc_library(
    name = "cpu_info",
    srcs = ["cpu_info.cc"],
    hdrs = ["cpu_info.h"],
    deps = [
        "//base",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//util/gtl:map_util",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "cpu_info_test",
    srcs = ["cpu_info_test.cc"],
    deps = [
        ":cpu_info",
        ":cpuid",
        "//base",
        "//exegesis/proto:cpuid_cc_proto",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//exegesis/proto/x86:cpuid_cc_proto",
        "//exegesis/util:proto_util",
        "//util/task:status",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

# A library to represent known CPU microarchitectures and models.
cc_library(
    name = "microarchitecture",
    srcs = ["microarchitecture.cc"],
    hdrs = ["microarchitecture.h"],
    deps = [
        ":port_mask",
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//util/gtl:map_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "microarchitecture_test",
    srcs = ["microarchitecture_test.cc"],
    deps = [
        ":microarchitecture",
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:proto_util",
        "//exegesis/x86:microarchitectures",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

# A library that provides information about the host CPU. Note that this library
# compiles only on platform where reading the host CPU is supported.
cc_library(
    name = "host_cpu",
    srcs = ["host_cpu.cc"],
    hdrs = ["host_cpu.h"],
    deps = [
        ":cpu_info",
        ":cpuid",
        "//base",
        "//exegesis/proto:cpuid_cc_proto",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "host_cpu_test",
    size = "small",
    srcs = ["host_cpu_test.cc"],
    deps = [
        ":cpu_info",
        ":host_cpu",
        "//exegesis/proto:cpuid_cc_proto",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# A special type for Opcodes.
cc_library(
    name = "opcode",
    srcs = ["opcode.cc"],
    hdrs = ["opcode.h"],
    deps = [
        "//base",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/container:node_hash_set",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "opcode_test",
    size = "small",
    srcs = ["opcode_test.cc"],
    deps = [
        ":opcode",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# An efficient representation of the execution unit port mask.
cc_library(
    name = "port_mask",
    srcs = ["port_mask.cc"],
    hdrs = ["port_mask.h"],
    deps = [
        "//base",
        "//exegesis/proto:microarchitecture_cc_proto",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "port_mask_test",
    size = "small",
    srcs = ["port_mask_test.cc"],
    deps = [
        ":port_mask",
        "//base",
        "//exegesis/proto:microarchitecture_cc_proto",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

# Utilities to prettyprint protos.
cc_library(
    name = "prettyprint",
    srcs = ["prettyprint.cc"],
    hdrs = ["prettyprint.h"],
    deps = [
        ":cpu_info",
        ":microarchitecture",
        ":port_mask",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//exegesis/util:instruction_syntax",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "prettyprint_test",
    size = "small",
    srcs = ["prettyprint_test.cc"],
    deps = [
        ":prettyprint",
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//exegesis/util:proto_util",
        "@com_github_glog_glog//:glog",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

# A library for working with register protos.
cc_library(
    name = "registers",
    srcs = ["registers.cc"],
    hdrs = ["registers.h"],
    deps = [
        "//base",
        "//exegesis/proto:registers_cc_proto",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "registers_test",
    size = "small",
    srcs = ["registers_test.cc"],
    deps = [
        ":registers",
        "//exegesis/testing:test_util",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# Utilities for restrict instruction set to a certain range.
cc_library(
    name = "restrict",
    srcs = ["restrict.cc"],
    hdrs = ["restrict.h"],
    deps = [
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:instruction_syntax",
        "//net/proto2/util/public:repeated_field_util",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "restrict_test",
    size = "small",
    srcs = ["restrict_test.cc"],
    deps = [
        ":restrict",
        "//base",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/testing:test_util",
        "//exegesis/util:proto_util",
        "@com_github_glog_glog//:glog",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

# Factory functions for obtaining the list of instruction set transforms.
cc_library(
    name = "transform_factory",
    srcs = ["transform_factory.cc"],
    hdrs = ["transform_factory.h"],
    deps = [
        ":cleanup_instruction_set",
        "//base",
        "//util/gtl:map_util",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_test(
    name = "transform_factory_test",
    srcs = ["transform_factory_test.cc"],
    deps = [
        ":cleanup_instruction_set",
        ":transform_factory",
        "@com_google_absl//absl/flags:flag",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
