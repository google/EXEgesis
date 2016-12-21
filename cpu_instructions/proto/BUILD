# Description:
#   Protocol buffers.

package(default_visibility = ["//visibility:public"])

load("//cpu_instructions:proto_library.bzl", "cpu_instructions_proto_library")

# Represents the pair (family, model) for a given CPU.

cpu_instructions_proto_library(
    name = "cpu_type_proto",
    srcs = ["cpu_type.proto"],
    cc_api_version = 2,
    js_api_version = 2,
)

# Represents information about a CPU.

cpu_instructions_proto_library(
    name = "cpu_info_proto",
    srcs = ["cpu_info.proto"],
    cc_api_version = 2,
    js_api_version = 2,
    deps = [":cpu_type_proto"],
)

# Represents instruction sets for a given cpu, as well as information gathered
# on every instruction.

cpu_instructions_proto_library(
    name = "instructions_proto",
    srcs = ["instructions.proto"],
    cc_api_version = 2,
    js_api_version = 2,
    visibility = ["//visibility:public"],
    deps = [
        ":cpu_info_proto",
        ":cpu_type_proto",
    ],
)

