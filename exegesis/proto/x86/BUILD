# Description:
#   Contains definitions of protocol buffers that are specific to the Intel 64
#   and IA-32 architectures.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

# Protocol buffers for storing the CPUID information.
proto_library(
    name = "cpuid_proto",
    srcs = ["cpuid.proto"],
)

cc_proto_library(
    name = "cpuid_cc_proto",
    deps = [":cpuid_proto"],
)

# Protocol buffers that contain structured data of the binary encoding of an
# instruction and a library for easier manipulation of these data.
proto_library(
    name = "decoded_instruction_proto",
    srcs = ["decoded_instruction.proto"],
    deps = ["//exegesis/proto/x86:instruction_encoding_proto"],
)

cc_proto_library(
    name = "decoded_instruction_cc_proto",
    deps = [":decoded_instruction_proto"],
)

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
