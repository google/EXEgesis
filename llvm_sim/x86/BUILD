licenses(["notice"])  # Apache 2.0

package(
    default_visibility = ["//visibility:public"],
    features = [
        # TODO(courbet): Bazel rules prevent us from accessing X86InstrInfo.h but in
        # cmake we should be good.
        "-layering_check",
    ],
)

cc_library(
    name = "constants",
    hdrs = ["constants.h"],
)

cc_library(
    name = "haswell",
    srcs = ["haswell.cc"],
    hdrs = ["haswell.h"],
    deps = [
        ":constants",
        "//llvm_sim/components:buffer",
        "//llvm_sim/components:decoder",
        "//llvm_sim/components:dispatch_port",
        "//llvm_sim/components:execution_unit",
        "//llvm_sim/components:fetcher",
        "//llvm_sim/components:parser",
        "//llvm_sim/components:port",
        "//llvm_sim/components:register_renamer",
        "//llvm_sim/components:reorder_buffer",
        "//llvm_sim/components:retirer",
        "//llvm_sim/components:simplified_execution_units",
        "//llvm_sim/framework:context",
        "//llvm_sim/framework:simulator",
    ],
)

cc_test(
    name = "haswell_test",
    srcs = ["haswell_test.cc"],
    data = [
        "//llvm_sim/x86/testdata:test1.s",
        "//llvm_sim/x86/testdata:test10.s",
        "//llvm_sim/x86/testdata:test2.s",
        "//llvm_sim/x86/testdata:test3.s",
        "//llvm_sim/x86/testdata:test4.s",
        "//llvm_sim/x86/testdata:test5.s",
        "//llvm_sim/x86/testdata:test6.s",
        "//llvm_sim/x86/testdata:test9.s",
    ],
    deps = [
        ":faucon_lib",
        ":haswell",
        "//llvm_sim/analysis:port_pressure",
        "//llvm_sim/framework:log",
        "//llvm_sim/framework:log_levels",
        "//llvm_sim/framework:simulator",
        "@com_google_absl//absl/flags:flag",
        "@com_google_googletest//:gtest_main",
        "@llvm_git//:CodeGen",
        "@llvm_git//:Core",
        "@llvm_git//:MC",
        "@llvm_git//:Support",
        "@llvm_git//:X86AsmParser",  # buildcleaner: keep
        "@llvm_git//:X86CodeGen",  # buildcleaner: keep
        "@llvm_git//:X86Info",  # buildcleaner: keep
    ],
)

cc_library(
    name = "faucon_lib",
    srcs = ["faucon_lib.cc"],
    hdrs = ["faucon_lib.h"],
    deps = [
        ":constants",
        "//llvm_sim/framework:context",
        "//llvm_sim/framework:log",
        "@llvm_git//:Core",
        "@llvm_git//:MC",
        "@llvm_git//:MCDisassembler",
        "@llvm_git//:MCParser",
        "@llvm_git//:Object",
        "@llvm_git//:Support",
    ],
)

cc_test(
    name = "faucon_lib_test",
    srcs = ["faucon_lib_test.cc"],
    data = [
        "//llvm_sim/x86/testdata:test1.s",
        "//llvm_sim/x86/testdata:testbinary",
    ],
    deps = [
        ":constants",
        ":faucon_lib",
        "@com_google_absl//absl/flags:flag",
        "@com_google_googletest//:gtest_main",
        "@llvm_git//:Support",
        "@llvm_git//:X86AsmParser",  # buildcleaner: keep
        "@llvm_git//:X86CodeGen",  # buildcleaner: keep
        "@llvm_git//:X86Disassembler",  # buildcleaner: keep
        "@llvm_git//:X86Info",  # buildcleaner: keep
    ],
)

cc_binary(
    name = "faucon",
    srcs = ["faucon.cc"],
    deps = [
        ":constants",
        ":faucon_lib",
        ":haswell",
        "//llvm_sim/analysis:inverse_throughput",
        "//llvm_sim/analysis:port_pressure",
        "@llvm_git//:MC",
        "@llvm_git//:Support",
        "@llvm_git//:X86AsmParser",  # buildcleaner: keep
        "@llvm_git//:X86CodeGen",  # buildcleaner: keep
        "@llvm_git//:X86Disassembler",  # buildcleaner: keep
        "@llvm_git//:X86Info",  # buildcleaner: keep
    ],
)
