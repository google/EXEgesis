# Description:
#   Protocol buffers.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

# Represents CPU microarchitectures and models.

proto_library(
    name = "microarchitecture_proto",
    srcs = ["microarchitecture.proto"],
)

cc_proto_library(
    name = "microarchitecture_cc_proto",
    deps = [
        ":microarchitecture_proto",
    ],
)

# Represents instruction sets for a given cpu, as well as information gathered
# on every instruction.

proto_library(
    name = "instructions_proto",
    srcs = ["instructions.proto"],
    deps = [
        ":microarchitecture_proto",
        "//exegesis/proto/x86:encoding_specification_proto",
    ],
)

cc_proto_library(
    name = "instructions_cc_proto",
    deps = [
        ":instructions_proto",
    ],
)
