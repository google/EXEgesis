# Description:
#   Libraries for computing itineraries.

package(default_visibility = ["//visibility:public"])

licenses(["notice"])  # Apache 2.0

# A library interfacing to libpfm4.
cc_library(
    name = "perf_subsystem",
    srcs = ["perf_subsystem.cc"],
    hdrs = ["perf_subsystem.h"],
    deps = [
        "//base",
        "//cpu_instructions/base:cpu_type",
        "//cpu_instructions/base:host_cpu",
        "//cpu_instructions/proto:cpu_type_proto",
        "//external:gflags",
        "//external:glog",
        "//external:libpfm4",
        "//external:protobuf_clib",
        "//external:protobuf_clib_for_base",
        "//strings",
        "//util/gtl:map_util",
    ],
)

cc_test(
    name = "perf_subsystem_test",
    size = "small",
    srcs = ["perf_subsystem_test.cc"],
    copts = ["-O2"],  # because of clang
    deps = [
        ":perf_subsystem",
        "//base",
        "//external:gflags",
        "//external:glog",
        "//external:googletest_main",
        "//external:protobuf_clib_for_base",
        "//strings",
    ],
)
