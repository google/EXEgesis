# Description:
#   Contains definitions of protocol buffers that are specific to the Intel 64
#   and IA-32 architectures.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

load("//cpu_instructions:proto_library.bzl", "cpu_instructions_proto_library")


# Protocol buffer library with the definitions of enums and constants for the
# x86-64 binary encoding.
cpu_instructions_proto_library(
    name = "instruction_encoding_proto",
    srcs = ["instruction_encoding.proto"],
    cc_api_version = 2,
    js_api_version = 2,
)

# The binary encoding specification of an instruction.
cpu_instructions_proto_library(
    name = "encoding_specification_proto",
    srcs = ["encoding_specification.proto"],
    cc_api_version = 2,
    js_api_version = 2,
    deps = [":instruction_encoding_proto"],
)
