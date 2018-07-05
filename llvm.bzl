"""Custom build rules for llvm."""

def gentbl(name, tblgen, td_file, td_srcs, tbl_outs, library = True, **kwdargs):
    """Generates tabular code from a table definition file.

    Args:
      name: The name of the build rule for use in dependencies.
      tblgen: The binary used to produce the output.
      td_file: The primary table definitions file.
      td_srcs: A list of table definition files included transitively.
      tbl_outs: A list of tuples (opts, out), where each opts is a string of
      options passed to tblgen, and the out is the corresponding output file
      produced.
      library: Whether to bundle the generated files into a library.
      **kwdargs: extra arguments
    """
    if td_file not in td_srcs:
        td_srcs += [td_file]
    includes = []
    for (opts, out) in tbl_outs:
        outdir = out[:out.rindex("/")]
        if outdir not in includes:
            includes.append(outdir)
        rule_suffix = "_".join(opts.replace("-", "_").replace("=", "_").split(" "))
        native.genrule(
            name = "%s_%s_genrule" % (name, rule_suffix),
            srcs = td_srcs,
            outs = [out],
            tools = [tblgen],
            message = "Generating code from table: %s" % td_file,
            cmd = (("$(location %s) " + "-I external/llvm_git/include " +
                    "-I third_party/llvm/llvm/tools/clang/include " +
                    "-I $$(dirname $(location %s)) " + "%s $(location %s) -o $@") % (
                tblgen,
                td_file,
                opts,
                td_file,
            )),
        )

    # For now, all generated files can be assumed to comprise public interfaces.
    # If this is not true, you should specify library = False
    # and list the generated '.inc' files in "srcs".
    if library:
        native.cc_library(
            name = name,
            textual_hdrs = [f for (_, f) in tbl_outs],
            includes = includes,
            **kwdargs
        )
