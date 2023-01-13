load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# ===== Abseil =====

http_archive(
    name = "com_google_absl",
    sha256 = "a4567ff02faca671b95e31d315bab18b42b6c6f1a60e91c6ea84e5a2142112c2",
    strip_prefix = "abseil-cpp-20211102.0",
    urls = [
        "https://github.com/abseil/abseil-cpp/archive/refs/tags/20211102.0.zip",
    ],
)

# CCTZ (Time-zone framework).
http_archive(
    name = "com_googlesource_code_cctz",
    strip_prefix = "cctz-master",
    urls = ["https://github.com/google/cctz/archive/master.zip"],
)

# ===== re2 =====

# NOTE(ondrasej): The commit for the RE2 library must be taken from the Abseil
# branch so that RE2 works correctly with absl::string_view.
http_archive(
    name = "com_googlesource_code_re2",
    sha256 = "fb002599c6620611368933083b4cff031a32c8a98638877fe097f56142646ba9",
    strip_prefix = "re2-e8cb5ecb8ee1066611aa937a42fa10514edf30fb",
    urls = [
        "https://github.com/google/re2/archive/e8cb5ecb8ee1066611aa937a42fa10514edf30fb.zip",
    ],
)

# ===== protobuf =====
# See https://bazel.build/blog/2017/02/27/protocol-buffers.html

http_archive(
    name = "com_google_protobuf",
    strip_prefix = "protobuf-3.19.3",
    sha256 = "6b6bf5cd8d0cca442745c4c3c9f527c83ad6ef35a405f64db5215889ac779b42",
    urls = [
        "https://github.com/protocolbuffers/protobuf/archive/refs/tags/v3.19.3.zip",
    ],
)

http_archive(
    name = "com_google_protobuf_cc",
    strip_prefix = "protobuf-3.19.3",
    sha256 = "6b6bf5cd8d0cca442745c4c3c9f527c83ad6ef35a405f64db5215889ac779b42",
    urls = [
        "https://github.com/protocolbuffers/protobuf/archive/refs/tags/v3.19.3.zip",
    ],
)

http_archive(
    name = "rules_cc",
    sha256 = "08f3b5c25eba2f7fb51db25692c2b9df6c557e0d8084bd8dc5e936d239f7641d",
    strip_prefix = "rules_cc-22532c537959149599e79df29808176345784cc1",
    urls = [
        "https://github.com/bazelbuild/rules_cc/archive/22532c537959149599e79df29808176345784cc1.tar.gz",
    ],
)

# rules_proto defines abstract rules for building Protocol Buffers.
http_archive(
    name = "rules_proto",
    sha256 = "20b240eba17a36be4b0b22635aca63053913d5c1ee36e16be36499d167a2f533",
    strip_prefix = "rules_proto-11bf7c25e666dd7ddacbcd4d4c4a9de7a25175f8",
    urls = [
        "https://github.com/bazelbuild/rules_proto/archive/11bf7c25e666dd7ddacbcd4d4c4a9de7a25175f8.tar.gz",
    ],
)

http_archive(
    name = "bazel_skylib",
    sha256 = "fc64d71583f383157e3e5317d24e789f942bc83c76fde7e5981cadc097a3c3cc",
    strip_prefix = "bazel-skylib-1.1.1",
    urls = [
        "https://github.com/bazelbuild/bazel-skylib/archive/refs/tags/1.1.1.zip",
    ],
)

load("@rules_cc//cc:repositories.bzl", "rules_cc_dependencies", "rules_cc_toolchains")
rules_cc_dependencies()
rules_cc_toolchains()

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")
rules_proto_dependencies()
rules_proto_toolchains()

# ===== googletest =====

http_archive(
    name = "com_google_googletest",
    sha256 = "c08d14e829bf5e2ab3bda2cc80a061126f2a430fcecdb9e9c0fc8023fd39d9a5",
    strip_prefix = "googletest-07f4869221012b16b7f9ee685d94856e1fc9f361",
    urls = [
        "https://github.com/google/googletest/archive/07f4869221012b16b7f9ee685d94856e1fc9f361.tar.gz",
    ],
)

# ===== benchmark =====

http_archive(
    name = "com_google_benchmark",
    build_file = "@//:benchmark.BUILD",
    sha256 = "e7334dd254434c6668e33a54c8f839194c7c61840d52f4b6258eee28e9f3b20e",
    strip_prefix = "benchmark-1.1.0",
    urls = [
        "https://github.com/google/benchmark/archive/v1.1.0.tar.gz",
    ],
)

# ===== utf =====

http_archive(
    name = "utf_archive",
    build_file = "@//:utf.BUILD",
    sha256 = "262a902f622dcd28e05b8a4be10da0aa3899050d0be8f4a71780eed6b2ea65ca",
    urls = [
        "https://mirror.bazel.build/9fans.github.io/plan9port/unix/libutf.tgz",
        "https://9fans.github.io/plan9port/unix/libutf.tgz",
    ],
)

# ===== gflags =====

http_archive(
    name = "com_github_gflags_gflags",
    sha256 = "466c36c6508a451734e4f4d76825cf9cd9b8716d2b70ef36479ae40f08271f88",
    strip_prefix = "gflags-2.2.0",
    urls = [
        "https://github.com/gflags/gflags/archive/v2.2.0.tar.gz",
    ],
)

# ===== glog =====

http_archive(
    name = "com_github_glog_glog",
    build_file = "@//:glog.BUILD",
    sha256 = "7580e408a2c0b5a89ca214739978ce6ff480b5e7d8d7698a2aa92fadc484d1e0",
    strip_prefix = "glog-0.3.5",
    urls = [
        "https://github.com/google/glog/archive/v0.3.5.tar.gz",
    ],
)

# ===== xpdf =====

http_archive(
    name = "xpdf_archive",  # GPLv2
    build_file = "@//:xpdf.BUILD",
    sha256 = "11390c74733abcb262aaca4db68710f13ffffd42bfe2a0861a5dfc912b2977e5",
    urls = [
        "https://mirror.bazel.build/download.openpkg.org/components/cache/xpdf/xpdf-3.04.tar.gz",
        "http://download.openpkg.org/components/cache/xpdf/xpdf-3.04.tar.gz",
    ],
)

# ===== libpfm4 =====

http_archive(
    name = "libpfm4_git",
    build_file = "@//:libpfm4.BUILD",
    sha256 = "9193787a73201b4254e3669243fd71d15a9550486920861912090a09f366cf68",
    strip_prefix = "libpfm-4.8.0",
    urls = ["https://iweb.dl.sourceforge.net/project/perfmon2/libpfm4/libpfm-4.8.0.tar.gz"],
)

# ===== tinyxml2 =====

http_archive(
    name = "tinyxml2_git",  # zlib license
    build_file = "@//:tinyxml2.BUILD",
    sha256 = "cdf0c2179ae7a7931dba52463741cf59024198bbf9673bf08415bcb46344110f",
    strip_prefix = "tinyxml2-6.2.0",
    urls = [
        "https://github.com/leethomason/tinyxml2/archive/6.2.0.tar.gz",
    ],
)

# ===== or-tools =====

http_archive(
    name = "or_tools_git",  # Apache 2.0
    sha256 = "d02fb68e1967fc3b8fed622755187a3df9cc175fe816cd149278d26b51519529",
    strip_prefix = "or-tools-53189020e3f995715a935aab7355357ce658fb76",
    urls = [
        "https://github.com/google/or-tools/archive/53189020e3f995715a935aab7355357ce658fb76.tar.gz",
    ],
)

# ===== LLVM =====

http_archive(
    name = "llvm_git",
    build_file = "@//:llvm.BUILD",
    sha256 = "a84450dd5c585c69ccf82e6e1e877d689bc9366221b955e2b2c8f02a47a4639a",
    strip_prefix = "llvm-project-579c4921c0105698617cc1e1b86a9ecf3b4717ce/llvm",
    urls = [
        # Please don't refactor out the SHA; if URLs aren't greppable,
        # we can't offer you an asynchronous mirroring service that
        # allows this huge archive to download in one second.
        "https://github.com/llvm/llvm-project/archive/579c4921c0105698617cc1e1b86a9ecf3b4717ce.tar.gz",
    ],
)

http_archive(
    name = "llvm_git_bazel_overlay",
    sha256 = "a84450dd5c585c69ccf82e6e1e877d689bc9366221b955e2b2c8f02a47a4639a",
    strip_prefix = "llvm-project-579c4921c0105698617cc1e1b86a9ecf3b4717ce/utils/bazel/llvm-project-overlay/llvm",
    urls = [
        # Please don't refactor out the SHA; if URLs aren't greppable,
        # we can't offer you an asynchronous mirroring service that
        # allows this huge archive to download in one second.
        "https://github.com/llvm/llvm-project/archive/579c4921c0105698617cc1e1b86a9ecf3b4717ce.tar.gz",
    ],
)

# ==============================================================================
# Transitive Dependencies:
# See https://bazel.build/versions/master/docs/external.html#transitive-dependencies
# ==============================================================================

http_archive(
    name = "glpk",  # GPLv3
    build_file = "@or_tools_git//bazel:glpk.BUILD",
    sha256 = "9a5dab356268b4f177c33e00ddf8164496dc2434e83bd1114147024df983a3bb",
    urls = [
        "https://ftp.gnu.org/gnu/glpk/glpk-4.52.tar.gz",
    ],
)
