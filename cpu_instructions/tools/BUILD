# Description:
#   All public tools for the project.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0


# A tool that parses the Intel Software Development Manual.

cc_binary(
    name = "parse_sdm",
    srcs = ["parse_sdm.cc"],
    deps = [
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//strings",
        "//cpu_instructions/base:transform_factory",
        "//cpu_instructions/x86/pdf:parse_sdm",
        "//cpu_instructions/x86/pdf:proto_util",
        "//util/task:status",
    ],
)
