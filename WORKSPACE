# ===== Abseil =====

git_repository(
    name = "com_google_absl",
    remote = "https://github.com/abseil/abseil-cpp.git",
    commit = "4e2e6c5c0071e6430056a8ef0a6c8a1fe584d8ff",
)

# CCTZ (Time-zone framework).
http_archive(
    name = "com_googlesource_code_cctz",
    urls = ["https://github.com/google/cctz/archive/master.zip"],
    strip_prefix = "cctz-master",
)

# ===== re2 =====

git_repository(
    name = "com_googlesource_code_re2",
    remote = "https://github.com/google/re2.git",
    commit = "c4f65071cc07eb34d264b25f7b9bbb679c4d5a5a",
)

# ===== protobuf =====
# See https://bazel.build/blog/2017/02/27/protocol-buffers.html

git_repository(
    name = "com_google_protobuf",
    remote = "https://github.com/google/protobuf.git",
    commit = "35119e39a07f426f3c48a1bfb41d862994223742",
)

git_repository(
    name = "com_google_protobuf_cc",
    remote = "https://github.com/google/protobuf.git",
    commit = "35119e39a07f426f3c48a1bfb41d862994223742",
)

# ===== googletest =====

git_repository(
    name = "com_google_googletest",
    remote = "https://github.com/google/googletest.git",
    commit = "dccc2d67547a5a3a97e4f211f39df931c6fbd5d5",
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
    url = "https://9fans.github.io/plan9port/unix/libutf.tgz",
    sha256 = "262a902f622dcd28e05b8a4be10da0aa3899050d0be8f4a71780eed6b2ea65ca",
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
    remote = "https://github.com/google/glog.git",
    tag = "v0.3.5",
    build_file = "glog.BUILD",
)

# ===== xpdf =====

new_http_archive(
    name = "xpdf_archive",
    url = "http://download.openpkg.org/components/cache/xpdf/xpdf-3.04.tar.gz",
    sha256 = "11390c74733abcb262aaca4db68710f13ffffd42bfe2a0861a5dfc912b2977e5",
    build_file = "xpdf.BUILD",
)

# ===== libpfm4 =====

new_git_repository(
    name = "libpfm4_git",
    remote = "git://git.code.sf.net/p/perfmon2/libpfm4",
    tag = "v4.8.0",
    build_file = "libpfm4.BUILD",
)

# ===== tinyxml2 =====

new_git_repository(
    name = "tinyxml2_git",
    remote = "https://github.com/leethomason/tinyxml2.git",
    tag = "4.0.1",
    build_file = "tinyxml2.BUILD",
)

# ===== or-tools =====

git_repository(
    name = "or_tools_git",
    remote = "https://github.com/google/or-tools.git",
    commit = "53189020e3f995715a935aab7355357ce658fb76",
)

# ===== LLVM =====

# NOTE(ondrasej): As of December 2017, downloading the code as a zip takes less
# than 30 seconds, while cloning the repository takes several minutes.
llvm_commit = "dbb224560c427c1d2f1eb8d7ffab7a61f873033e"
new_http_archive(
    name = "llvm_git",
    url = "https://codeload.github.com/llvm-mirror/llvm/zip/" + llvm_commit,
    sha256 = "965cc8bea09465dd5815a9169698508789e401a5f558fef5a8a0384e41f0495f",
    type = "zip",
    build_file = "llvm.BUILD",
    strip_prefix = "llvm-" + llvm_commit,
)

# ===== Intel SDM =====

http_file(
    name = "intel_sdm_pdf",
    sha256 = "d1e056c9df6e02fe8ef7b7f886a6da48527f8cd76000012a483d0680c3a89811",
    url = "https://software.intel.com/sites/default/files/managed/39/c5/325462-sdm-vol-1-2abcd-3abcd.pdf",
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
