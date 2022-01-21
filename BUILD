# A database of instructions and a parser for the Intel(r) 64 and IA-32
# Architectures Software Developer's Manual.

package(default_visibility = ["//visibility:private"])

licenses(["notice"])

exports_files([
    "LICENSE",
    "llvm.bzl",
])

# Generates a text file with the version of LLVM used in the build. The version
# is required to generate some of the LLVM headers, but it is not possible to
# get the version from inside the @llvm_git repository.
# We use the git hash or tag as a version string; this hash or tag is extracted
# from the llvm_git rule in the WORKSPACE file, to avoid duplication of this
# information.
genrule(
    name = "llvm_git_revision_gen",
    srcs = ["WORKSPACE"],
    outs = ["llvm_git_revision"],
    cmd = r"""perl -ne 'if (/strip_prefix = "llvm-project-(.*)\/llvm"/) { print "$$1\n"; exit } ' < $< > $@""",
    visibility = ["//visibility:public"],
)

package_group(
    name = "internal_users",
    packages = [
        "//exegesis/...",
        "//llvm_sim/...",
    ],
)
