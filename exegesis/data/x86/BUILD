# Contains rules that download and parse the instruction data for x86-64.

package(default_visibility = ["//:internal_users"])

licenses(["notice"])

# The parsed data from the Intel SDM.
genrule(
    name = "parse_intel_sdm",
    srcs = [
        "@intel_sdm_pdf//file",
        "//exegesis/x86/pdf:sdm_patches",
    ],
    outs = ["intel_instruction_sets.pbtxt"],
    cmd = "$(location //exegesis/tools:parse_intel_sdm) " +
          "--exegesis_input_spec=$(location @intel_sdm_pdf//file) " +
          "--exegesis_output_file_base=$$(dirname $@)/intel_instruction_sets ",
    tools = ["//exegesis/tools:parse_intel_sdm"],
)
