# Description:
#   Contains definitions of protocol buffers that are specific to the Intel 64
#   and IA-32 architectures.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

# Protocol buffer library with the definitions of enums and constants for the
# x86-64 binary encoding.
proto_library(
    name = "instruction_encoding_proto",
    srcs = ["instruction_encoding.proto"],
)

cc_proto_library(
    name = "instruction_encoding_cc_proto",
    deps = [":instruction_encoding_proto"],
)

# The binary encoding specification of an instruction.
proto_library(
    name = "encoding_specification_proto",
    srcs = ["encoding_specification.proto"],
    deps = [":instruction_encoding_proto"],
)

cc_proto_library(
    name = "encoding_specification_cc_proto",
    deps = [":encoding_specification_proto"],
)
