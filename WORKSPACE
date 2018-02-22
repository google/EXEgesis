# ===== Abseil =====

git_repository(
    name = "com_google_absl",
    remote = "https://github.com/abseil/abseil-cpp.git",
    commit = "c742b72354a84958b6a061755249822eeef87d06",
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
llvm_commit = "0f0caba04ff8e48f15b35dbca80af4266a8c5184"
new_http_archive(
    name = "llvm_git",
    url = "https://codeload.github.com/llvm-mirror/llvm/zip/" + llvm_commit,
    sha256 = "94c619ebb7995dee29c93b63506b78e1ee8ffbcb9e15f58e0f78af481f8455bb",
    type = "zip",
    build_file = "llvm.BUILD",
    strip_prefix = "llvm-" + llvm_commit,
)

# ===== Intel SDM =====

http_file(
    name = "intel_sdm_pdf",
    url = "https://software.intel.com/sites/default/files/managed/39/c5/325462-sdm-vol-1-2abcd-3abcd.pdf",
    sha256 = "3edaf91e17c64f9f3bdaabd5c743e60656b80edc19fe4a2560bf25d6725f39fb",
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
