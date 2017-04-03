# ===== re2 =====

git_repository(
    name = "com_googlesource_code_re2",
    remote = "https://github.com/google/re2.git",
    tag = "2016-11-01",
)

bind(
    name = "re2",
    actual = "@com_googlesource_code_re2//:re2",
)

# ===== protobuf =====

git_repository(
    name = "protobuf_git",
    remote = "https://github.com/google/protobuf.git",
    tag = "v3.2.0",
)

bind(
    name = "protobuf_clib",
    actual = "@protobuf_git//:protobuf",
)

bind(
    name = "protobuf_clib_for_strings",
    actual = "@protobuf_git//:protobuf_lite",
)
bind(
    name = "protobuf_clib_for_map_util",
    actual = "@protobuf_git//:protobuf_lite",
)
bind(
    name = "protobuf_clib_for_base",
    actual = "@protobuf_git//:protobuf_lite",
)
bind(
    name = "protobuf_clib_for_status",
    actual = "@protobuf_git//:protobuf_lite",
)

bind(
    name = "protobuf_compiler",
    actual = "@protobuf_git//:protoc_lib",
)

bind(
    name = "protoc",
    actual = "@protobuf_git//:protoc",
)

# ===== googletest =====

new_git_repository(
    name = "googletest_git",
    remote = "https://github.com/google/googletest.git",
    tag = "release-1.8.0",
    build_file = "gmock.BUILD",
)

bind(
    name = "googletest",
    actual = "@googletest_git//:gtest",
)

bind(
    name = "googletest_main",
    actual = "@googletest_git//:gtest_main",
)

# ===== utf =====

new_http_archive(
    name = "utf_archive",
    url = "https://swtch.com/plan9port/unix/libutf.tgz",
    sha256 = "7789326c507fe9c07ade0731e0b0da221385a8f7cd1faa890af92a78a953bf5e",
    build_file = "utf.BUILD",
)

bind(
    name = "utf",
    actual = "@utf_archive//:utf",
)

# ===== gflags =====

new_git_repository(
    name = "gflags_git",
    build_file = "gflags.BUILD",
    remote = "https://github.com/gflags/gflags.git",
    tag = "v2.1.2",
)

bind(
    name = "gflags",
    actual = "@gflags_git//:gflags",
)

# ===== glog =====

new_git_repository(
    name = "glog_git",
    build_file = "glog.BUILD",
    remote = "https://github.com/google/glog.git",
    tag = "v0.3.4",
)

bind(
    name = "glog",
    actual = "@glog_git//:glog",
)

# ===== xpdf =====

new_http_archive(
    name = "xpdf_archive",
    url = "http://ctan.math.washington.edu/tex-archive/support/xpdf/xpdf-3.04.tar.gz",
    sha256 = "11390c74733abcb262aaca4db68710f13ffffd42bfe2a0861a5dfc912b2977e5",
    build_file = "xpdf.BUILD",
)

bind(
    name = "xpdf",
    actual = "@xpdf_archive//:xpdf",
)

# ===== libpfm4 =====

new_git_repository(
    name = "libpfm4_git",
    build_file = "libpfm4.BUILD",
    remote = "https://git.code.sf.net/p/perfmon2/libpfm4",
    tag = "v4.8.0",
)

bind(
    name = "libpfm4",
    actual = "@libpfm4_git//:pfm4",
)

# ===== LLVM =====

new_git_repository(
    name = "llvm_git",
    build_file = "llvm.BUILD",
    remote = "http://llvm.org/git/llvm.git",
    # LLVM has no tags.
    commit = "69112bd6d297c71dfa60f0e3156db54acf0bafc",
)

load("//:llvm.bzl", "export_llvm_target")

export_llvm_target("analysis")
export_llvm_target("codegen")
export_llvm_target("ir")
export_llvm_target("machine_code")
export_llvm_target("support")
export_llvm_target("target_base")
export_llvm_target("x86_target")
export_llvm_target("x86_target_asm_parser")
export_llvm_target("x86_target_disassembler")
export_llvm_target("x86_target_info")

