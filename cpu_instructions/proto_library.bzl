"""Custom build rules."""

def cpu_instructions_proto_library(name, srcs, cc_api_version=None,
                                   js_api_version=None, deps=[],
                                   visibility=None):
  """Rule for building proto libraries.

  Args:
      name: name of the proto library
      srcs: proto sources
      cc_api_version: unused
      js_api_version: unused
      deps: other proto_library(s) that this target depends on.
      visibility: target visibility
  """
  _ = cc_api_version  # Unused
  _ = js_api_version  # Unused
  out_hdrs = [p.replace(".proto", ".pb.h") for p in srcs]
  out_srcs = [p.replace(".proto", ".pb.cc") for p in srcs]
  deps_proto_files = [d + "_proto_files" for d in deps]
  # Compile the proto to cc files.
  native.genrule(
      name = name + "_cc_files",
      # bazel only allows the genrule command to read files specified in
      # srcs, so we need to add proto files for deps for includes to work.
      srcs = srcs + deps_proto_files,
      outs = out_hdrs + out_srcs,
      cmd = ("$(location @protobuf_git//:protoc) --cpp_out=$(GENDIR) " +
             " ".join(["$(location %s)" % src for src in srcs])),
      tools = ["@protobuf_git//:protoc"],
      visibility = ["//visibility:private"],
  )
  # Make a library with the cc files.
  native.cc_library(
      name = name,
      srcs = out_srcs,
      hdrs = out_hdrs,
      deps = deps + [
          "@protobuf_git//:protobuf",
      ],
      visibility = visibility,
  )
  # Make the proto files available to deps for inclusion.
  native.filegroup(
      name = name + "_proto_files",
      srcs = srcs + deps_proto_files,
      visibility = visibility,
  )
