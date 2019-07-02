# Utilities to manipulate XML documents with tinyxml2.

package(default_visibility = ["//:internal_users"])

licenses(["notice"])  # Apache 2.0

cc_library(
    name = "xml_util",
    srcs = ["xml_util.cc"],
    hdrs = ["xml_util.h"],
    deps = [
        "//base",
        "//util/task:status",
        "//util/task:statusor",
        "@com_github_glog_glog//:glog",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf_lite",
        "@tinyxml2_git//:tinyxml2",
    ],
)

cc_test(
    name = "xml_util_test",
    srcs = ["xml_util_test.cc"],
    deps = [
        ":xml_util",
        "//exegesis/testing:test_util",
        "//util/task:status",
        "@com_google_googletest//:gtest_main",
        "@tinyxml2_git//:tinyxml2",
    ],
)
