# Contains classes, functions and data structures that are platform independent.
# Code in this directory is only allowed to depend on proto/ and util/.

package(default_visibility = ["//cpu_instructions:__subpackages__"])

licenses(["notice"])  # Apache 2.0

# Framework for writing procedural clean-ups of an instruction set.
cc_library(
    name = "cleanup_instruction_set",
    srcs = ["cleanup_instruction_set.cc"],
    hdrs = ["cleanup_instruction_set.h"],
    deps = [
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        
        "//external:protobuf_clib",
        
        "//util/gtl:map_util",
        "//cpu_instructions/proto:instructions_proto",
        "//util/task:status",
        "//util/task:statusor",
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
        "//external:protobuf_clib",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/proto:instructions_proto",
    ],
)

cc_test(
    name = "cleanup_instruction_set_test",
    size = "small",
    srcs = ["cleanup_instruction_set_test.cc"],
    deps = [
        ":cleanup_instruction_set",
        ":cleanup_instruction_set_test_utils",
        "//external:protobuf_clib",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
        "//util/task:status",
    ],
)

cc_library(
    name = "transform_factory",
    srcs = ["transform_factory.cc"],
    hdrs = ["transform_factory.h"],
    deps = [
        ":cleanup_instruction_set",
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//strings",
        "//util/gtl:map_util",
    ],
)

cc_test(
    name = "transform_factory_test",
    srcs = ["transform_factory_test.cc"],
    deps = [
        ":cleanup_instruction_set",
        ":transform_factory",
        "//base", "//external:glog", "//external:gflags", "//external:protobuf_clib_for_base",
        "//cpu_instructions/testing:googletest",
        "//cpu_instructions/testing:googletest_main",
    ],
)
