# Description:
#   Protocol buffers.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

# Represents CPU microarchitectures and models.

proto_library(
    name = "cpu_type_proto",
    srcs = ["cpu_type.proto"],
)

cc_proto_library(
    name = "cpu_type_cc_proto",
    deps = [
        ":cpu_type_proto",
    ],
)

# Represents instruction sets for a given cpu, as well as information gathered
# on every instruction.

proto_library(
    name = "instructions_proto",
    srcs = ["instructions.proto"],
    deps = [
        ":cpu_type_proto",
        "//cpu_instructions/proto/x86:encoding_specification_proto",
    ],
)

cc_proto_library(
    name = "instructions_cc_proto",
    deps = [
        ":instructions_proto",
    ],
)
