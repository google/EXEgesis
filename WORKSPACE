# ===== re2 =====

git_repository(
    name = "com_googlesource_code_re2",
    remote = "https://github.com/google/re2.git",
    tag = "2016-11-01",
)

# ===== protobuf =====
# See https://bazel.build/blog/2017/02/27/protocol-buffers.html

git_repository(
    name = "com_google_protobuf",
    remote = "https://github.com/google/protobuf.git",
    commit = "b4b0e304be5a68de3d0ee1af9b286f958750f5e4",
)

git_repository(
    name = "com_google_protobuf_cc",
    remote = "https://github.com/google/protobuf.git",
    commit = "b4b0e304be5a68de3d0ee1af9b286f958750f5e4",
)

# ===== googletest =====

new_git_repository(
    name = "googletest_git",
    remote = "https://github.com/google/googletest.git",
    tag = "release-1.8.0",
    build_file = "gmock.BUILD",
)

# ===== utf =====

new_http_archive(
    name = "utf_archive",
    url = "https://swtch.com/plan9port/unix/libutf.tgz",
    sha256 = "7789326c507fe9c07ade0731e0b0da221385a8f7cd1faa890af92a78a953bf5e",
    build_file = "utf.BUILD",
)

# ===== gflags =====

git_repository(
    name = "com_github_gflags_gflags",
    remote = "https://github.com/gflags/gflags.git",
    tag = "v2.2.0",
)

# ===== glog =====

new_git_repository(
    name = "glog_git",
    build_file = "glog.BUILD",
    remote = "https://github.com/google/glog.git",
    tag = "v0.3.4",
)

# ===== xpdf =====

new_http_archive(
    name = "xpdf_archive",
    url = "http://ctan.math.washington.edu/tex-archive/support/xpdf/xpdf-3.04.tar.gz",
    sha256 = "11390c74733abcb262aaca4db68710f13ffffd42bfe2a0861a5dfc912b2977e5",
    build_file = "xpdf.BUILD",
)

# ===== libpfm4 =====

new_git_repository(
    name = "libpfm4_git",
    build_file = "libpfm4.BUILD",
    remote = "https://git.code.sf.net/p/perfmon2/libpfm4",
    tag = "v4.8.0",
)

# ===== LLVM =====

new_git_repository(
    name = "llvm_git",
    build_file = "llvm.BUILD",
    remote = "http://llvm.org/git/llvm.git",
    # LLVM has no tags.
    commit = "69112bd6d297c71dfa60f0e3156db54acf0bafc",
)
