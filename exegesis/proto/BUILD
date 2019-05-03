# Description:
#   Protocol buffers.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

# Represents information about CPU models coming from the CPUID
# instructions/registers of the chips.
proto_library(
    name = "cpuid_proto",
    srcs = ["cpuid.proto"],
    deps = [
        "//exegesis/proto/x86:cpuid_proto",
    ],
)

cc_proto_library(
    name = "cpuid_cc_proto",
    deps = [":cpuid_proto"],
)

# Represents CPU microarchitectures and models.
proto_library(
    name = "microarchitecture_proto",
    srcs = ["microarchitecture.proto"],
    visibility = [
        "//visibility:public",
    ],
)

cc_proto_library(
    name = "microarchitecture_cc_proto",
    deps = [":microarchitecture_proto"],
)

# Represents the registers available in an instruction set.
proto_library(
    name = "registers_proto",
    srcs = ["registers.proto"],
)

cc_proto_library(
    name = "registers_cc_proto",
    deps = [":registers_proto"],
)

# Represents instruction sets for a given cpu, as well as information gathered
# on every instruction.
proto_library(
    name = "instructions_proto",
    srcs = ["instructions.proto"],
    deps = [
        ":instruction_encoding_proto",
        ":microarchitecture_proto",
        ":registers_proto",
        "//exegesis/proto/x86:encoding_specification_proto",
    ],
)

cc_proto_library(
    name = "instructions_cc_proto",
    deps = [":instructions_proto"],
)

# Represents how to encode an instruction.
proto_library(
    name = "instruction_encoding_proto",
    srcs = ["instruction_encoding.proto"],
)

cc_proto_library(
    name = "instruction_encoding_cc_proto",
    deps = [":instruction_encoding_proto"],
)
