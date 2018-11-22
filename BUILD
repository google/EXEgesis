# A database of instructions and a parser for the Intel(r) 64 and IA-32
# Architectures Software Developer's Manual.

package(default_visibility = ["//visibility:private"])

licenses(["notice"])  # Apache 2.0

exports_files([
    "LICENSE",
    "llvm.bzl",
])

package_group(
    name = "internal_users",
    packages = [
        "//exegesis/...",
        "//llvm_sim/...",
    ],
)
