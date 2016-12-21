cc_library(
    name = "glog",
    srcs = [
        "src/base/commandlineflags.h",
        "src/base/googleinit.h",
        "src/base/mutex.h",
        "src/config.h",
        "src/demangle.cc",
        "src/demangle.h",
        "src/logging.cc",
        "src/raw_logging.cc",
        "src/signalhandler.cc",
        "src/symbolize.cc",
        "src/symbolize.h",
        "src/utilities.cc",
        "src/utilities.h",
        "src/vlog_is_on.cc",
    ] + glob(["src/stacktrace*.h"]),
    hdrs = [
        "src/glog/log_severity.h",
        "src/glog/logging.h",
        "src/glog/raw_logging.h",
        "src/glog/stl_logging.h",
        "src/glog/vlog_is_on.h",
    ],
    copts = [
        "-Wno-sign-compare",
        "-U_XOPEN_SOURCE",
    ],
    includes = ["./src"],
    linkopts = ["-lpthread"],
    visibility = ["//visibility:public"],
    deps = [
        "//external:gflags",
    ],
)

genrule(
    name = "run_configure",
    srcs = [
        "README",
        "Makefile.in",
        "config.guess",
        "config.sub",
        "install-sh",
        "ltmain.sh",
        "missing",
        "libglog.pc.in",
        "src/config.h.in",
        "src/glog/logging.h.in",
        "src/glog/raw_logging.h.in",
        "src/glog/stl_logging.h.in",
        "src/glog/vlog_is_on.h.in",
    ],
    outs = [
        "src/config.h",
        "src/glog/logging.h",
        "src/glog/raw_logging.h",
        "src/glog/stl_logging.h",
        "src/glog/vlog_is_on.h",
    ],
    tools = [
        "configure",
    ],
    cmd = "$(location :configure)" +
          "&& cp -v src/config.h $(location src/config.h) " +
          "&& cp -v src/glog/logging.h $(location src/glog/logging.h) " +
          "&& cp -v src/glog/raw_logging.h $(location src/glog/raw_logging.h) " +
          "&& cp -v src/glog/stl_logging.h $(location src/glog/stl_logging.h) " +
          "&& cp -v src/glog/vlog_is_on.h $(location src/glog/vlog_is_on.h) "
    ,
)

