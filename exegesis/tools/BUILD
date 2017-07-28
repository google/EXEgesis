# Description:
#   All public tools for the project.

package(default_visibility = ["//visibility:private"])

licenses(["notice"])  # Apache 2.0

# A tool that computes itineraries for the host cpu microarchitecture.
cc_binary(
    name = "compute_itineraries",
    srcs = ["compute_itineraries.cc"],
    deps = [
        ":architecture_flags",
        "//base",
        "//exegesis/base:microarchitecture",
        "//exegesis/itineraries:compute_itineraries",
        "//exegesis/util:proto_util",
        "//exegesis/util:system",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

# A library that provides access to instruction sets for all supported architectures.
cc_library(
    name = "architecture_flags",
    srcs = ["architecture_flags.cc"],
    hdrs = ["architecture_flags.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//base",
        "//exegesis/base:architecture_provider",
        "//exegesis/base:microarchitecture",
        "//exegesis/proto:instructions_cc_proto",
        "//strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

# A tool that parses the Intel Software Development Manual.
cc_binary(
    name = "parse_intel_sdm",
    srcs = ["parse_intel_sdm.cc"],
    deps = [
        "//base",
        "//exegesis/base:cleanup_instruction_set",
        "//exegesis/base:transform_factory",
        "//exegesis/proto:instructions_cc_proto",
        "//exegesis/util:proto_util",
        "//exegesis/x86:cleanup_instruction_set_all",
        "//exegesis/x86/pdf:parse_sdm",
        "//strings",
        "//util/task:status",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

# A tool that parses the a CPUID dumps and converts the information to
# EXEgesis formats.
cc_binary(
    name = "parse_cpuid_dump",
    srcs = ["parse_cpuid_dump.cc"],
    deps = [
        "//base",
        "//exegesis/base:cpuid",
        "//exegesis/proto:cpuid_cc_proto",
        "//exegesis/proto:microarchitecture_cc_proto",
        "//exegesis/util:file_util",
        "//exegesis/util:proto_util",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_binary(
    name = "pdf2proto",
    srcs = ["pdf2proto.cc"],
    deps = [
        "//base",
        "//exegesis/proto/pdf:pdf_document_cc_proto",
        "//exegesis/util:proto_util",
        "//exegesis/util/pdf:xpdf_util",
        "//strings",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

cc_binary(
    name = "proto_patch_helper",
    srcs = ["proto_patch_helper.cc"],
    deps = [
        "//base",
        "//exegesis/proto/pdf:pdf_document_cc_proto",
        "//exegesis/util:proto_util",
        "//exegesis/util/pdf:xpdf_util",
        "//strings",
        "//util/gtl:map_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_binary(
    name = "proto_patch_migrate",
    srcs = ["proto_patch_migrate.cc"],
    deps = [
        "//base",
        "//exegesis/proto/pdf:pdf_document_cc_proto",
        "//exegesis/util:proto_util",
        "//exegesis/util/pdf:pdf_document_utils",
        "//exegesis/util/pdf:xpdf_util",
        "//strings",
        "//util/gtl:map_util",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_glog_glog//:glog",
        "@com_google_protobuf//:protobuf_lite",
        "@com_googlesource_code_re2//:re2",
    ],
)
