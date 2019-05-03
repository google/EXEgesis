licenses(["notice"])  # Apache 2.0

package(
    default_visibility = [
        "//llvm_sim/x86:__pkg__",
    ],
)

exports_files([
    "test1.s",
    "test2.s",
    "test3.s",
    "test4.s",
    "test5.s",
    "test6.s",
    "test9.s",
    "test10.s",
])

cc_binary(
    name = "testbinary",
    srcs = ["testbinary.cc"],
)
