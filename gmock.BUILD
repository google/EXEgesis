cc_library(
    name = "gtest",
    srcs = [
        "googletest/src/gtest-all.cc",
        "googlemock/src/gmock-all.cc",
    ],
    hdrs = glob([
        "**/*.h",
        # gtest also includes cc files.
        "**/*.cc",
    ]),
    includes = [
        "googlemock",
        "googlemock/include",
        "googletest",
        "googletest/include",
    ],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
    # NOTE(user): The bind() calls do not transfer the testonly attribute
    # correctly from the original repository to the alias in //external, which
    # causes build failures.
    # TODO(user): Re-enable testonly when they are preserved by bind().
    # testonly = 1,
)

cc_library(
    name = "gtest_main",
    srcs = ["googlemock/src/gmock_main.cc"],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
    deps = [":gtest"],
    # TODO(user): Re-enable testonly when they are preserved by bind().
    # testonly = 1,
)
