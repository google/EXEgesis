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

git_repository(
    name = "com_google_googletest",
    remote = "https://github.com/google/googletest.git",
    commit = "c3f65335b79f47b05629e79a54685d899bc53b93",
)

# ===== benchmark =====

new_git_repository(
    name = "com_google_benchmark",
    remote = "https://github.com/google/benchmark.git",
    tag = "v1.1.0",
    build_file = "benchmark.BUILD",
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
    name = "com_github_glog_glog",
    build_file = "glog.BUILD",
    remote = "https://github.com/google/glog.git",
    tag = "v0.3.5",
)

# ===== xpdf =====

new_http_archive(
    name = "xpdf_archive",
    url = "http://mirror.neu.edu.cn/CTAN/support/xpdf/xpdf-3.04.tar.gz",
    sha256 = "11390c74733abcb262aaca4db68710f13ffffd42bfe2a0861a5dfc912b2977e5",
    build_file = "xpdf.BUILD",
)

# ===== libpfm4 =====

new_git_repository(
    name = "libpfm4_git",
    build_file = "libpfm4.BUILD",
    remote = "git://git.code.sf.net/p/perfmon2/libpfm4",
    tag = "v4.8.0",
)

# ===== or-tools =====

git_repository(
    name = "or_tools_git",
    remote = "https://github.com/google/or-tools.git",
    commit = "df7959fa5a1db1e941a1d7582b0340e1a2e7a8c8",
)

# ===== LLVM =====

new_git_repository(
    name = "llvm_git",
    build_file = "llvm.BUILD",
    remote = "https://git.llvm.org/git/llvm.git",
    # LLVM has no tags.
    commit = "389880910a2da533fbda53cdf5ddf344f70f591f",
)

# ===== Intel SDM =====

http_file(
    name = "intel_sdm_pdf",
    url = "https://software.intel.com/sites/default/files/managed/39/c5/325462-sdm-vol-1-2abcd-3abcd.pdf",
    sha256 = "398fe124b6082b0e370f0e3127c6c285a4bcc920c95fa249ee8aa76de43bb475",
)

# ==============================================================================
# Transitive Dependencies:
# See https://bazel.build/versions/master/docs/external.html#transitive-dependencies
# ==============================================================================

new_http_archive(
    name = "glpk",
    url = "http://ftp.gnu.org/gnu/glpk/glpk-4.52.tar.gz",
    sha256 = "9a5dab356268b4f177c33e00ddf8164496dc2434e83bd1114147024df983a3bb",
    build_file = "@or_tools_git//bazel:glpk.BUILD",
)
