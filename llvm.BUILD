

# TODO(courbet): Make this configurable with select() statements.
llvm_host_triple = "x86_64"

llvm_native_arch = "X86"

# These three are the space-separated list of targets we support. When adding a
# target here, you also need to add the list of generated files to
# llvm_target_list below.
llvm_targets = "X86"

llvm_target_asm_parsers = "X86"

llvm_target_disassemblers = "X86"

# Genrules.

load("@//:llvm.bzl", "gentbl")

genrule(
    name = "generate_llvm_config_h",
    outs = ["include/llvm/Config/llvm-config.h"],
    cmd = ("""cat <<EOF > $@
#ifndef LLVM_CONFIG_H
#define LLVM_CONFIG_H
#define LLVM_BINDIR "/dev/null"
#define LLVM_ENABLE_THREADS 0
#define LLVM_HAS_ATOMICS 1
#define LLVM_INCLUDEDIR "/dev/null"
#define LLVM_INFODIR "/dev/null"
#define LLVM_MANDIR "/dev/null"
#define LLVM_ON_UNIX 1
#define HAVE_SYSEXITS_H 1
#define LLVM_VERSION_MAJOR 0
#define LLVM_VERSION_MINOR 0
#define LLVM_VERSION_PATCH 0
#define LLVM_VERSION_STRING "CPU-instructions"
""" +
           "#define LLVM_HOST_TRIPLE \"" + llvm_host_triple + "\"\n" +
           "#define LLVM_DEFAULT_TARGET_TRIPLE \"" + llvm_host_triple + "\"\n" +
           "#define LLVM_NATIVE_ARCH " + llvm_native_arch + "\n" +
           "#define LLVM_NATIVE_TARGET LLVMInitialize" + llvm_native_arch + "Target\n" +
           "#define LLVM_NATIVE_TARGETINFO LLVMInitialize" + llvm_native_arch + "TargetInfo\n" +
           "#define LLVM_NATIVE_TARGETMC LLVMInitialize" + llvm_native_arch + "TargetMC\n" +
           "#define LLVM_NATIVE_ASMPRINTER LLVMInitialize" + llvm_native_arch + "AsmPrinter\n" +
           "#define LLVM_NATIVE_ASMPARSER LLVMInitialize" + llvm_native_arch + "AsmParser\n" +
           "#define LLVM_NATIVE_MCASMINFO LLVMInitialize" + llvm_native_arch + "MCAsmInfo\n" +
           "#define LLVM_NATIVE_MCCODEGENINFO LLVMInitialize" + llvm_native_arch + "MCCodeGenInfo\n" +
           "#endif\n" +
           "EOF"),
)

genrule(
    name = "generate_config_h",
    outs = ["include/llvm/Config/config.h"],
    cmd = ("""cat <<EOF > $@
#ifndef CONFIG_H
#define CONFIG_H
#include "llvm/Config/llvm-config.h"
#define ENABLE_BACKTRACES 0
#define LLVM_ENABLE_CRASH_DUMPS 0
#define HAVE_DIRENT_H 1
#define HAVE_DLFCN_H 1
#define HAVE_DLOPEN 1
#define HAVE_DLADDR 1
#define HAVE_ERRNO_H 1
#define HAVE_EXECINFO_H 1
#define HAVE_FCNTL_H 1
#define HAVE_INT64_T 1
#define HAVE_STDINT_H 1
#define HAVE_SYS_MMAN_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_UIO_H 1
#if !defined(OS_MACOSX)
#define HAVE_FUTIMENS 1
#endif
#define HAVE_GETPAGESIZE 1
#define HAVE_GETRUSAGE 1
#define HAVE_UNISTD_H 1
#if !defined(OS_MACOSX)
#define HAVE_LINK_H 1
#endif
#if defined(OS_MACOSX)
#define HAVE_MACH_MACH_H 1
#endif
#if !defined(OS_MACOSX)
#define HAVE_MALLINFO 1
#endif
#if !defined(OS_MACOSX)
#define HAVE_MALLOC_H 1
#endif
#if defined(OS_MACOSX)
#define HAVE_MALLOC_MALLOC_H 1
#endif
#if defined(OS_MACOSX)
#define HAVE_MALLOC_ZONE_STATISTICS 1
#endif
#define HAVE_MKDTEMP 1
#define HAVE_MKSTEMP 1
#define HAVE_MKTEMP 1
#define PACKAGE_NAME "llvm"
#define PACKAGE_STRING "llvm CPU-instructions"
#define PACKAGE_VERSION "CPU-instructions"
#define BUG_REPORT_URL "http://llvm.org/bugs/"
#define RETSIGTYPE void
#endif
EOF"""),
)

genrule(
    name = "generate_abibreaking_h",
    srcs = ["include/llvm/Config/abi-breaking.h.cmake"],
    outs = ["include/llvm/Config/abi-breaking.h"],
    cmd = ("sed -e 's/#cmakedefine01 LLVM_ENABLE_ABI_BREAKING_CHECKS/#define LLVM_ENABLE_ABI_BREAKING_CHECKS 0/'" +
           "    -e 's/#cmakedefine01 LLVM_ENABLE_REVERSE_ITERATION/#define LLVM_ENABLE_REVERSE_ITERATION 0/'" +
           "  $< > $@"),
)

genrule(
    name = "generate_vcsrevision_h",
    srcs = [],
    outs = ["include/llvm/Support/VCSRevision.h"],
    cmd = "echo '' > \"$@\"",
)

genrule(
    name = "generate_targets_defs",
    srcs = [
        "include/llvm/Config/Targets.def.in",
        "include/llvm/Config/AsmParsers.def.in",
        "include/llvm/Config/AsmPrinters.def.in",
        "include/llvm/Config/Disassemblers.def.in",
    ],
    outs = [
        "include/llvm/Config/Targets.def",
        "include/llvm/Config/AsmParsers.def",
        "include/llvm/Config/AsmPrinters.def",
        "include/llvm/Config/Disassemblers.def",
    ],
    # The following script expands a set of macros in the input files from
    # the Autoconf-style token '@LLVM_ENUM_<macro>S@'. Each token becomes:
    #   LLVM_<macro>(X86)
    #   LLVM_<macro>(OtherTarget)
    #   ...
    cmd = ("out_dir=\"$(@D)/include/llvm/Config\"\n" +
           "for input in $(SRCS); do\n" +
           "  cp $$input $$out_dir/$$(basename $$input .in)\n" +
           "done\n" +
           "\n" +
           "subst_str=\"\"\n" +
           "for target in " + llvm_targets + "; do\n" +
           "  subst_str=\"$$subst_str\\\\\nLLVM_TARGET($$target)\"\n" +
           "done\n" +
           "sed -i -e \"/@LLVM_ENUM_TARGETS@/c $$subst_str\" " +
           "    $$out_dir/*.def\n" +
           "\n" +
           "subst_str=\"\"\n" +
           "for target in " + llvm_targets + "; do\n" +
           "  subst_str=\"$$subst_str\\\\\nLLVM_ASM_PRINTER($$target)\"\n" +
           "done\n" +
           "sed -i -e \"/@LLVM_ENUM_ASM_PRINTERS@/c $$subst_str\" " +
           "    $$out_dir/*.def\n" +
           "\n" +
           "subst_str=\"\"\n" +
           "for target in " + llvm_target_asm_parsers + "; do\n" +
           "  subst_str=\"$$subst_str\\\\\nLLVM_ASM_PARSER($$target)\"\n" +
           "done\n" +
           "sed -i -e \"/@LLVM_ENUM_ASM_PARSERS@/c $$subst_str\" " +
           "    $$out_dir/*.def\n" +
           "\n" +
           "subst_str=\"\"\n" +
           "for target in " + llvm_target_disassemblers + "; do\n" +
           "  subst_str=\"$$subst_str\\\\\nLLVM_DISASSEMBLER($$target)\"\n" +
           "done\n" +
           "sed -i -e \"/@LLVM_ENUM_DISASSEMBLERS@/c $$subst_str\" " +
           "    $$out_dir/*.def\n"),
    message = "Generating target definitions",
)

# Libraries.

cc_library(
    name = "AsmParser",
    srcs = glob([
        "lib/AsmParser/*.cpp",
        "lib/AsmParser/*.h",
    ]),
    hdrs = [
        "include/llvm/AsmParser/Parser.h",
        "include/llvm/AsmParser/SlotMapping.h",
    ],
    deps = [
        ":BinaryFormat",
        ":Core",
        ":Support",
    ],
)

# cc_library(
#     name = "asm_printer",
#     srcs = glob([
#         "lib/CodeGen/AsmPrinter/*.cpp",
#         "lib/CodeGen/AsmPrinter/*.h",
#     ]),
#     hdrs = [
#         "include/llvm/CodeGen/AsmPrinter.h",
#     ] + glob([
#         "lib/CodeGen/AsmPrinter/*.def",
#     ]),
#     deps = [
#         ":analysis",
#         ":codegen",
#         ":config",
#         ":debug_info_codeview",
#         ":debug_info_dwarf",
#         ":ir",
#         ":machine_code",
#         ":machine_code_parser",
#         ":support",
#         ":target_base",
#     ],
# )
cc_library(
    name = "asm_printer_defs",
    textual_hdrs = glob(["lib/CodeGen/AsmPrinter/*.def"]),
)

cc_library(
    name = "BitReader",
    srcs = glob([
        "lib/Bitcode/Reader/*.cpp",
        "lib/Bitcode/Reader/*.h",
    ]),
    hdrs = [
        "include/llvm-c/BitReader.h",
        "include/llvm/Bitcode/BitcodeAnalyzer.h",
        "include/llvm/Bitcode/BitcodeCommon.h",
        "include/llvm/Bitcode/BitcodeReader.h",
        "include/llvm/Bitcode/LLVMBitCodes.h",
    ],
    deps = [
        ":BitstreamReader",
        ":Core",
        ":Support",
        ":config",
    ],
)

cc_library(
    name = "BitWriter",
    srcs = glob([
        "lib/Bitcode/Writer/*.cpp",
        "lib/Bitcode/Writer/*.h",
    ]),
    hdrs = [
        "include/llvm-c/BitWriter.h",
        "include/llvm/Bitcode/BitcodeCommon.h",
        "include/llvm/Bitcode/BitcodeWriter.h",
        "include/llvm/Bitcode/BitcodeWriterPass.h",
        "include/llvm/Bitcode/LLVMBitCodes.h",
    ],
    deps = [
        ":Analysis",
        ":BitstreamWriter",
        ":Core",
        ":MC",
        ":Object",
        ":Support",
        ":config",
    ],
)

cc_library(
    name = "BitstreamReader",
    srcs = glob([
        "lib/Bitstream/Reader/*.cpp",
        "lib/Bitstream/Reader/*.h",
    ]),
    hdrs = [
        "include/llvm/Bitstream/BitCodes.h",
        "include/llvm/Bitstream/BitstreamReader.h",
    ],
    deps = [
        ":Support",
    ],
)

cc_library(
    name = "BitstreamWriter",
    srcs = glob([
        "lib/Bitstream/Writer/*.h",
    ]),
    hdrs = [
        "include/llvm/Bitstream/BitCodes.h",
        "include/llvm/Bitstream/BitstreamWriter.h",
    ],
    deps = [
        ":Support",
    ],
)

cc_library(
    name = "config",
    hdrs = [
        "include/llvm/Config/AsmParsers.def",
        "include/llvm/Config/AsmPrinters.def",
        "include/llvm/Config/Disassemblers.def",
        "include/llvm/Config/Targets.def",
        "include/llvm/Config/abi-breaking.h",
        "include/llvm/Config/config.h",
        "include/llvm/Config/llvm-config.h",
    ],
    defines = [
        "LLVM_ENABLE_STATS",
        "__STDC_LIMIT_MACROS",
        "__STDC_CONSTANT_MACROS",
        "_DEBUG",
        "LLVM_BUILD_GLOBAL_ISEL",
    ],
    includes = ["include"],
)

cc_library(
    name = "Analysis",
    srcs = glob([
        "lib/Analysis/*.cpp",
        "lib/Analysis/*.h",
    ]) + [
        "include/llvm-c/Analysis.h",
        "include/llvm/Analysis/Utils/Local.h",
    ],
    hdrs = glob([
        "include/llvm/Analysis/*.h",
        "include/llvm/Analysis/*.def",
    ]) + [
        "include/llvm/Transforms/Utils/LoopUtils.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":BinaryFormat",
        ":Core",
        ":Object",
        ":ProfileData",
        ":Support",
        ":config",
    ],
)

cc_library(
    name = "CodeGen",
    srcs = glob(
        [
            "lib/CodeGen/**/*.cpp",
            "lib/CodeGen/**/*.h",
            "lib/CodeGen/SelectionDAG/*.cpp",
            "lib/CodeGen/SelectionDAG/*.h",
        ],
    ),
    hdrs = [
        "include/llvm/LinkAllPasses.h",
    ] + glob(
        [
            "include/llvm/CodeGen/**/*.h",
        ],
    ),
    textual_hdrs = glob([
        "include/llvm/CodeGen/**/*.def",
        "include/llvm/CodeGen/**/*.inc",
    ]),
    visibility = ["//visibility:public"],
    deps = [
        ":Analysis",
        ":AsmParser",
        ":BinaryFormat",
        ":BitReader",
        ":BitWriter",
        ":Core",
        ":DebugInfoCodeView",
        ":DebugInfoDWARF",
        ":IPO",
        ":MC",
        ":MCParser",
        ":ProfileData",
        ":Remarks",
        ":Scalar",
        ":Support",
        ":Target",
        ":TransformUtils",
        ":asm_printer_defs",
        ":config",
    ],
)

cc_library(
    name = "DebugInfo",
    hdrs = glob(["include/llvm/DebugInfo/*.h"]),
    deps = [
        ":Object",
        ":Support",
    ],
)

cc_library(
    name = "DebugInfoCodeView",
    srcs = glob([
        "lib/DebugInfo/CodeView/*.cpp",
        "lib/DebugInfo/CodeView/*.h",
    ]),
    hdrs = glob([
        "include/llvm/DebugInfo/CodeView/*.h",
        "include/llvm/DebugInfo/CodeView/*.def",
    ]),
    includes = ["include/llvm/DebugInfo/CodeView"],
    deps = [
        ":BinaryFormat",
        ":Support",
    ],
)

cc_library(
    name = "DebugInfoDWARF",
    srcs = glob([
        "lib/DebugInfo/DWARF/*.cpp",
        "lib/DebugInfo/DWARF/*.h",
    ]),
    hdrs = glob(["include/llvm/DebugInfo/DWARF/*.h"]),
    deps = [
        ":BinaryFormat",
        ":DebugInfo",
        ":MC",
        ":Object",
        ":Support",
    ],
)

cc_library(
    name = "Demangle",
    srcs = glob([
        "lib/Demangle/*.cpp",
        "lib/Demangle/*.h",
    ]),
    hdrs = glob(["include/llvm/Demangle/*.h"]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [":config"],
)

cc_library(
    name = "TableGen",
    srcs = glob([
        "lib/TableGen/*.cpp",
        "lib/TableGen/*.h",
    ]),
    hdrs = glob(["include/llvm/TableGen/*.h"]),
    includes = ["include"],
    linkopts = ["-ldl"],
    visibility = ["//visibility:public"],
    deps = [
        ":Support",
        ":config",
    ],
)

cc_library(
    name = "ExecutionEngine",
    srcs = glob([
        "lib/ExecutionEngine/*.cpp",
        "lib/ExecutionEngine/*.h",
        "lib/ExecutionEngine/RuntimeDyld/*.cpp",
        "lib/ExecutionEngine/RuntimeDyld/*.h",
        "lib/ExecutionEngine/RuntimeDyld/Targets/*.cpp",
        "lib/ExecutionEngine/RuntimeDyld/Targets/*.h",
    ]) + [
        "include/llvm-c/ExecutionEngine.h",
        "include/llvm/DebugInfo/DIContext.h",
    ],
    hdrs = glob(
        [
            "include/llvm/ExecutionEngine/*.h",
        ],
        exclude = [
            "include/llvm/ExecutionEngine/MCJIT*.h",
            "include/llvm/ExecutionEngine/OProfileWrapper.h",
        ],
    ) + [
        "include/llvm-c/ExecutionEngine.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":BinaryFormat",
        ":CodeGen",
        ":Core",
        ":DebugInfo",
        ":MC",
        ":MCDisassembler",
        ":Object",
        ":Passes",
        ":Support",
        ":Target",
        ":config",
    ],
)

cc_library(
    name = "MCJIT",
    srcs = glob([
        "lib/ExecutionEngine/MCJIT/*.cpp",
        "lib/ExecutionEngine/MCJIT/*.h",
    ]),
    hdrs = glob(["include/llvm/ExecutionEngine/MCJIT*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        ":CodeGen",
        ":Core",
        ":ExecutionEngine",
        ":MC",
        ":Object",
        ":Support",
        ":Target",
        ":config",
    ],
)

cc_library(
    name = "llvm-tblgen-lib",
    srcs = glob([
        "utils/TableGen/*.cpp",
        "utils/TableGen/*.h",
        "utils/TableGen/GlobalISel/*.cpp",
    ]),
    hdrs = glob([
        "utils/TableGen/GlobalISel/*.h",
    ]),
    includes = ["include"],
    linkopts = ["-ldl"],
    deps = [
        ":MC",
        ":Support",
        ":TableGen",
        ":config",
    ],
)

cc_binary(
    name = "llvm-tblgen",
    includes = ["include"],
    linkopts = [
        "-ldl",
        "-lm",
        "-lpthread",
    ],
    stamp = 0,
    deps = [
        ":llvm-tblgen-lib",
    ],
)

gentbl(
    name = "intrinsic_enums_gen",
    tbl_outs = [("-gen-intrinsic-enums", "include/llvm/IR/IntrinsicEnums.inc")],
    tblgen = ":llvm-tblgen",
    td_file = "include/llvm/IR/Intrinsics.td",
    td_srcs = glob([
        "include/llvm/CodeGen/*.td",
        "include/llvm/IR/Intrinsics*.td",
    ]),
)

gentbl(
    name = "intrinsics_impl_gen",
    tbl_outs = [("-gen-intrinsic-impl", "include/llvm/IR/IntrinsicImpl.inc")],
    tblgen = ":llvm-tblgen",
    td_file = "include/llvm/IR/Intrinsics.td",
    td_srcs = glob([
        "include/llvm/CodeGen/*.td",
        "include/llvm/IR/Intrinsics*.td",
    ]),
)

llvm_target_intrinsics_list = [
    {
        "name": "AArch64",
        "intrinsic_prefix": "aarch64",
    },
    {
        "name": "AMDGPU",
        "intrinsic_prefix": "amdgcn",
    },
    {
        "name": "ARM",
        "intrinsic_prefix": "arm",
    },
    {
        "name": "BPF",
        "intrinsic_prefix": "bpf",
    },
    {
        "name": "Hexagon",
        "intrinsic_prefix": "hexagon",
    },
    {
        "name": "Mips",
        "intrinsic_prefix": "mips",
    },
    {
        "name": "NVPTX",
        "intrinsic_prefix": "nvvm",
    },
    {
        "name": "PowerPC",
        "intrinsic_prefix": "ppc",
    },
    {
        "name": "R600",
        "intrinsic_prefix": "r600",
    },
    {
        "name": "RISCV",
        "intrinsic_prefix": "riscv",
    },
    {
        "name": "S390",
        "intrinsic_prefix": "s390",
    },
    {
        "name": "WebAssembly",
        "intrinsic_prefix": "wasm",
    },
    {
        "name": "X86",
        "intrinsic_prefix": "x86",
    },
    {
        "name": "XCore",
        "intrinsic_prefix": "xcore",
    },
]

[[
    gentbl(
        name = "intrinsic_" + target["name"] + "_gen",
        tbl_outs = [(
            "-gen-intrinsic-enums -intrinsic-prefix=" + target["intrinsic_prefix"],
            "include/llvm/IR/Intrinsics" + target["name"] + ".h",
        )],
        tblgen = ":llvm-tblgen",
        td_file = "include/llvm/IR/Intrinsics.td",
        td_srcs = glob([
            "include/llvm/CodeGen/*.td",
            "include/llvm/IR/*.td",
        ]),
    ),
] for target in llvm_target_intrinsics_list]

filegroup(
    name = "llvm_intrinsics_headers",
    srcs = [
        "include/llvm/IR/Intrinsics" + target["name"] + ".h"
        for target in llvm_target_intrinsics_list
    ],
)

gentbl(
    name = "attributes_gen",
    tbl_outs = [("-gen-attrs", "include/llvm/IR/Attributes.inc")],
    tblgen = ":llvm-tblgen",
    td_file = "include/llvm/IR/Attributes.td",
    td_srcs = ["include/llvm/IR/Attributes.td"],
)

cc_library(
    name = "BinaryFormat",
    srcs = glob([
        "lib/BinaryFormat/*.cpp",
        "lib/BinaryFormat/*.h",
    ]),
    hdrs = glob([
        "include/llvm/BinaryFormat/*.h",
        "include/llvm/BinaryFormat/*.def",
        "include/llvm/BinaryFormat/ELFRelocs/*.def",
        "include/llvm/BinaryFormat/WasmRelocs/*.def",
    ]),
    copts = [
        "$(STACK_FRAME_UNLIMITED)",
    ],
    linkopts = ["-ldl"],
    deps = [
        ":Support",
    ],
)

cc_library(
    name = "Core",
    srcs = glob([
        "lib/IR/*.cpp",
        "lib/IR/*.h",
    ]),
    hdrs = glob(
        [
            "include/llvm/*.h",
            "include/llvm/IR/*.h",
        ],
        exclude = [
            "include/llvm/LinkAllPasses.h",
        ],
    ) + [
        "include/llvm-c/Comdat.h",
        "include/llvm-c/DebugInfo.h",
    ] + [":llvm_intrinsics_headers"],
    textual_hdrs = glob(["include/llvm/IR/*.def"]),
    visibility = ["//visibility:public"],
    deps = [
        ":BinaryFormat",
        ":Remarks",
        ":Support",
        ":attributes_gen",
        ":config",
        ":intrinsic_enums_gen",
        ":intrinsics_impl_gen",
    ],
)

cc_library(
    name = "ir_headers",
    hdrs = glob(
        [
            "include/llvm/*.h",
            "include/llvm/IR/*.h",
        ],
        exclude = [
            "include/llvm/LinkAllPasses.h",
        ],
    ) + [
        "include/llvm/IR/Value.def",
        "include/llvm-c/Comdat.h",
        "include/llvm-c/DebugInfo.h",
    ],
)

cc_library(
    name = "IRReader",
    srcs = glob([
        "lib/IRReader/*.cpp",
        "lib/IRReader/*.h",
    ]),
    hdrs = glob(["include/llvm/IRReader/*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        ":AsmParser",
        ":BitReader",
        ":Core",
        ":Support",
        ":config",
    ],
)

cc_library(
    name = "Linker",
    srcs = glob([
        "lib/Linker/*.cpp",
        "lib/Linker/*.h",
    ]),
    hdrs = glob(["include/llvm/Linker/*.h"]),
    deps = [
        ":Core",
        ":Support",
        ":TransformUtils",
        ":config",
    ],
)

cc_library(
    name = "MC",
    srcs = glob([
        "lib/MC/*.cpp",
        "lib/MC/*.h",
        "lib/MC/MCAnalysis/*.cpp",
        "lib/MC/MCAnalysis/*.h",
    ]),
    hdrs = glob([
        "include/llvm/MC/*.h",
        "include/llvm/MC/*.def",
        "include/llvm/MC/*.inc",
    ]),
    visibility = ["//visibility:public"],
    deps = [
        ":BinaryFormat",
        ":DebugInfoCodeView",
        ":Support",
        ":config",
        ":ir_headers",
    ],
)

cc_library(
    name = "MCDisassembler",
    srcs = glob([
        "lib/MC/MCDisassembler/*.cpp",
        "lib/MC/MCDisassembler/*.h",
    ]),
    hdrs = glob([
        "include/llvm/MC/MCDisassembler/*.h",
    ]) + [
        "include/llvm-c/Disassembler.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":MC",
        ":Support",
        ":config",
    ],
)

cc_library(
    name = "MCParser",
    srcs = glob([
        "lib/MC/MCParser/*.cpp",
        "lib/MC/MCParser/*.h",
    ]),
    hdrs = glob(["include/llvm/MC/MCParser/*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        ":BinaryFormat",
        ":MC",
        ":Support",
        ":config",
    ],
)

cc_library(
    name = "Object",
    srcs = glob([
        "lib/Object/*.cpp",
        "lib/Object/*.h",
    ]),
    hdrs = glob([
        "include/llvm/Object/*.h",
    ]) + [
        "include/llvm-c/Object.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":BinaryFormat",
        ":BitReader",
        ":Core",
        ":MC",
        ":MCParser",
        ":Support",
        ":TextAPI",
        ":config",
    ],
)

cc_library(
    name = "pass_registry_def",
    textual_hdrs = ["lib/Passes/PassRegistry.def"],
)

cc_library(
    name = "Passes",
    srcs = glob([
        "lib/Passes/*.cpp",
        "lib/Passes/*.h",
    ]),
    hdrs = glob(["include/llvm/Passes/*.h"]),
    deps = [
        ":AggressiveInstCombine",
        ":Analysis",
        ":CFGuard",
        ":CodeGen",
        ":Core",
        ":Coroutines",
        ":HelloNew",
        ":IPO",
        ":InstCombine",
        ":Instrumentation",
        ":ObjCARC",
        ":Scalar",
        ":Support",
        ":Target",
        ":TransformUtils",
        ":Vectorize",
        ":config",
        ":pass_registry_def",
    ],
)

cc_library(
    name = "ProfileData",
    srcs = glob([
        "lib/ProfileData/*.cpp",
        "lib/ProfileData/*.h",
    ]),
    hdrs = glob([
        "include/llvm/ProfileData/*.h",
        "include/llvm/ProfileData/*.inc",
    ]),
    visibility = ["//visibility:public"],
    deps = [
        ":Core",
        ":Support",
        ":config",
    ],
)

cc_library(
    name = "Support",
    srcs = glob([
        "lib/Support/*.c",
        "lib/Support/*.cpp",
        "lib/Support/*.h",
        "lib/Support/*.inc",
        "lib/Support/Unix/*.h",
        "lib/Support/Unix/*.inc",
        "include/llvm-c/*.h",
    ]),
    hdrs = glob([
        "include/llvm/Support/*.h",
        "include/llvm/ADT/*.h",
    ]) + ["include/llvm/Support/VCSRevision.h"],
    includes = ["include"],
    linkopts = ["-ldl"],
    textual_hdrs = glob([
        "include/llvm/Support/*.def",
        "include/llvm/Support/**/*.def",
    ]),
    visibility = ["//visibility:public"],
    deps = [
        ":Demangle",
        ":config",
    ],
)

cc_library(
    name = "Target",
    srcs = glob([
        "lib/Target/*.cpp",
        "lib/Target/*.h",
    ]) + [
        "include/llvm-c/Initialization.h",
        "include/llvm-c/Target.h",
    ],
    hdrs = glob([
        "include/llvm/Target/*.h",
        "include/llvm/Target/*.def",
    ]),
    visibility = ["//visibility:public"],
    deps = [
        ":Analysis",
        ":BinaryFormat",
        ":Core",
        ":MC",
        ":Support",
        ":config",
    ],
)

# Transforms.

cc_library(
    name = "TransformUtils",
    srcs = glob([
        "lib/Transforms/Utils/*.cpp",
        "lib/Transforms/Utils/*.h",
        "include/llvm/CodeGen/Passes.h",
        "include/llvm-c/Transforms/Utils.h",
    ]),
    hdrs = glob(["include/llvm/Transforms/Utils/*.h"]) + [
        "include/llvm/Transforms/Utils.h",
        "include/llvm-c/Transforms/Utils.h",
    ],
    deps = [
        ":Analysis",
        ":BinaryFormat",
        ":BitWriter",
        ":Core",
        ":Support",
        ":Target",
        ":config",
    ],
)

cc_library(
    name = "AggressiveInstCombine",
    srcs = glob([
        "lib/Transforms/AggressiveInstCombine/*.cpp",
        "lib/Transforms/AggressiveInstCombine/*.h",
    ]),
    hdrs = [
        "include/llvm-c/Transforms/AggressiveInstCombine.h",
        "include/llvm/Transforms/AggressiveInstCombine/AggressiveInstCombine.h",
    ],
    deps = [
        ":Analysis",
        ":Core",
        ":Support",
        ":TransformUtils",
    ],
)

cc_library(
    name = "CFGuard",
    srcs = glob([
        "lib/Transforms/CFGuard/*.cpp",
        "lib/Transforms/CFGuard/*.h",
    ]),
    hdrs = ["include/llvm/Transforms/CFGuard.h"],
    includes = ["include"],
    deps = [
        ":Core",
        ":Support",
    ],
)

cc_library(
    name = "Coroutines",
    srcs = glob([
        "lib/Transforms/Coroutines/*.cpp",
        "lib/Transforms/Coroutines/*.h",
    ]),
    hdrs = [
        "include/llvm-c/Transforms/Coroutines.h",
        "include/llvm/Transforms/Coroutines.h",
        "include/llvm/Transforms/Coroutines/CoroCleanup.h",
        "include/llvm/Transforms/Coroutines/CoroEarly.h",
        "include/llvm/Transforms/Coroutines/CoroElide.h",
        "include/llvm/Transforms/Coroutines/CoroSplit.h",
    ],
    deps = [
        ":Analysis",
        ":Core",
        ":IPO",
        ":Scalar",
        ":Support",
        ":TransformUtils",
        ":config",
    ],
)

cc_library(
    name = "InstCombine",
    srcs = glob([
        "lib/Transforms/InstCombine/*.cpp",
        "lib/Transforms/InstCombine/*.h",
        "include/llvm/Transforms/InstCombine/*.h",
        "include/llvm-c/Transforms/InstCombine.h",
    ]),
    deps = [
        ":Analysis",
        ":Core",
        ":InstCombineTableGen",
        ":Support",
        ":Target",
        ":TransformUtils",
        ":config",
    ],
)

gentbl(
    name = "InstCombineTableGen",
    tbl_outs = [(
        "-gen-searchable-tables",
        "lib/Target/AMDGPU/InstCombineTables.inc",
    )],
    tblgen = ":llvm-tblgen",
    td_file = "lib/Target/AMDGPU/InstCombineTables.td",
    td_srcs = glob([
        "include/llvm/CodeGen/*.td",
        "include/llvm/IR/Intrinsics*.td",
    ]) + [
        "lib/Target/AMDGPU/InstCombineTables.td",
        "include/llvm/TableGen/SearchableTable.td",
    ],
)

cc_library(
    name = "Instrumentation",
    srcs = glob([
        "lib/Transforms/Instrumentation/*.cpp",
        "lib/Transforms/Instrumentation/*.h",
        "lib/Transforms/Instrumentation/*.inc",
        "include/llvm/Transforms/Instrumentation/*.h",
    ]),
    hdrs = glob(["include/llvm/Transforms/Instrumentation/*.h"]) + [
        "include/llvm/Transforms/Instrumentation.h",
    ],
    deps = [
        ":Analysis",
        ":BinaryFormat",
        ":Core",
        ":MC",
        ":ProfileData",
        ":Support",
        ":TransformUtils",
        ":config",
    ],
)

filegroup(
    name = "omp_td_files",
    srcs = glob([
        "include/llvm/Frontend/OpenMP/*.td",
        "include/llvm/Frontend/Directive/*.td",
    ]),
)

gentbl(
    name = "omp_gen",
    library = False,
    tbl_outs = [
        ("--gen-directive-decl", "include/llvm/Frontend/OpenMP/OMP.h.inc"),
    ],
    tblgen = ":llvm-tblgen",
    td_file = "include/llvm/Frontend/OpenMP/OMP.td",
    td_srcs = [":omp_td_files"],
)

gentbl(
    name = "omp_gen_impl",
    library = False,
    tbl_outs = [
        ("--gen-directive-gen", "include/llvm/Frontend/OpenMP/OMP.cpp.inc"),
        ("--gen-directive-impl", "lib/Frontend/OpenMP/OMP.cpp"),
    ],
    tblgen = ":llvm-tblgen",
    td_file = "include/llvm/Frontend/OpenMP/OMP.td",
    td_srcs = [":omp_td_files"],
)

cc_library(
    name = "FrontendOpenMP",
    srcs = glob([
        "lib/Frontend/OpenMP/*.cpp",
    ]) + [
        "include/llvm/Frontend/OpenMP/OMP.cpp.inc",
        "lib/Frontend/OpenMP/OMP.cpp",
    ],
    hdrs = glob([
        "include/llvm/Frontend/OpenMP/*.h",
        "include/llvm/Frontend/OpenMP/OMP/*.h",
        "include/llvm/Frontend/*.h",
    ]) + ["include/llvm/Frontend/OpenMP/OMP.h.inc"],
    textual_hdrs = glob([
        "include/llvm/Frontend/OpenMP/*.def",
    ]),
    deps = [
        ":Analysis",
        ":Core",
        ":Support",
        ":TransformUtils",
    ],
)

cc_library(
    name = "IPO",
    srcs = glob([
        "lib/Transforms/IPO/*.cpp",
        "lib/Transforms/IPO/*.h",
    ]) + [
        "include/llvm-c/Transforms/IPO.h",
        "include/llvm-c/Transforms/PassManagerBuilder.h",
    ],
    hdrs = glob([
        "include/llvm/Transforms/IPO/*.h",
    ]) + [
        "include/llvm/Transforms/IPO.h",
    ],
    deps = [
        ":AggressiveInstCombine",
        ":Analysis",
        ":BinaryFormat",
        ":BitReader",
        ":BitWriter",
        ":Core",
        ":FrontendOpenMP",
        ":IRReader",
        ":InstCombine",
        ":Instrumentation",
        ":Linker",
        ":ObjCARC",
        ":Object",
        ":ProfileData",
        ":Scalar",
        ":Support",
        ":Target",
        ":TransformUtils",
        ":Vectorize",
        ":config",
    ],
)

cc_library(
    name = "HelloNew",
    srcs = glob([
        "lib/Transforms/HelloNew/*.cpp",
        "lib/Transforms/HelloNew/*.h",
    ]),
    hdrs = ["include/llvm/Transforms/HelloNew/HelloWorld.h"],
    deps = [
        ":Core",
        ":Support",
    ],
)

cc_library(
    name = "ObjCARC",
    srcs = glob([
        "lib/Transforms/ObjCARC/*.cpp",
        "lib/Transforms/ObjCARC/*.h",
    ]),
    hdrs = ["include/llvm/Transforms/ObjCARC.h"],
    deps = [
        ":Analysis",
        ":Core",
        ":Support",
        ":Target",
        ":TransformUtils",
        ":config",
    ],
)

cc_library(
    name = "Remarks",
    srcs = glob(
        [
            "lib/Remarks/*.cpp",
            "lib/Remarks/*.h",
        ],
        exclude = ["lib/Remarks/RemarkLinker.cpp"],
    ),
    hdrs = glob(
        [
            "include/llvm/Remarks/*.h",
        ],
        exclude = ["include/llvm/Remarks/RemarkLinker.h"],
    ) + [
        "include/llvm-c/Remarks.h",
    ],
    linkopts = ["-ldl"],
    deps = [
        ":BitstreamReader",
        ":BitstreamWriter",
        ":Support",
    ],
)

cc_library(
    name = "Scalar",
    srcs = glob([
        "lib/Transforms/Scalar/*.cpp",
        "lib/Transforms/Scalar/*.h",
        "include/llvm/Transforms/Scalar/*.h",
    ]) + [
        "include/llvm/Transforms/IPO.h",
        "include/llvm/Transforms/IPO/SCCP.h",
        "include/llvm-c/Initialization.h",
        "include/llvm-c/Transforms/Scalar.h",
    ],
    hdrs = [
        "include/llvm/Transforms/Scalar.h",
        "include/llvm/Transforms/Scalar/GVN.h",
    ],
    deps = [
        ":AggressiveInstCombine",
        ":Analysis",
        ":Core",
        ":InstCombine",
        ":ProfileData",
        ":Support",
        ":Target",
        ":TransformUtils",
        ":config",
    ],
)

cc_library(
    name = "Vectorize",
    srcs = glob([
        "lib/Transforms/Vectorize/*.cpp",
        "lib/Transforms/Vectorize/*.h",
        "include/llvm/Transforms/Vectorize/*.h",
    ]) + [
        "include/llvm-c/Transforms/Vectorize.h",
        "include/llvm/Transforms/Scalar/LoopPassManager.h",
    ],
    hdrs = [
        "include/llvm/Transforms/Scalar/LoopPassManager.h",
        "include/llvm/Transforms/Vectorize.h",
    ],
    deps = [
        ":Analysis",
        ":Core",
        ":Support",
        ":Target",
        ":TransformUtils",
        ":config",
    ],
)

cc_library(
    name = "TextAPI",
    srcs = glob([
        "lib/TextAPI/**/*.cpp",
    ]),
    hdrs = glob([
        "include/llvm/TextAPI/**/*.h",
        "include/llvm/TextAPI/**/*.def",
        "lib/TextAPI/MachO/*.h",
    ]),
    includes = ["include"],
    deps = [
        ":BinaryFormat",
        ":Support",
    ],
)

# Targets.

llvm_target_list = [
    {
        "name": "X86",
        "lower_name": "x86",
        "short_name": "X86",
        "tbl_outs": [
            ("-gen-register-bank", "lib/Target/X86/X86GenRegisterBank.inc"),
            ("-gen-register-info", "lib/Target/X86/X86GenRegisterInfo.inc"),
            ("-gen-disassembler", "lib/Target/X86/X86GenDisassemblerTables.inc"),
            ("-gen-instr-info", "lib/Target/X86/X86GenInstrInfo.inc"),
            ("-gen-asm-writer", "lib/Target/X86/X86GenAsmWriter.inc"),
            ("-gen-asm-writer -asmwriternum=1", "lib/Target/X86/X86GenAsmWriter1.inc"),
            ("-gen-asm-matcher", "lib/Target/X86/X86GenAsmMatcher.inc"),
            ("-gen-dag-isel", "lib/Target/X86/X86GenDAGISel.inc"),
            ("-gen-fast-isel", "lib/Target/X86/X86GenFastISel.inc"),
            ("-gen-global-isel", "lib/Target/X86/X86GenGlobalISel.inc"),
            ("-gen-callingconv", "lib/Target/X86/X86GenCallingConv.inc"),
            ("-gen-subtarget", "lib/Target/X86/X86GenSubtargetInfo.inc"),
            ("-gen-x86-EVEX2VEX-tables", "lib/Target/X86/X86GenEVEX2VEXTables.inc"),
        ],
    },
]

[[
    [gentbl(
        name = target["name"] + "CommonTableGen",
        tbl_outs = target["tbl_outs"],
        tblgen = ":llvm-tblgen",
        td_file = "lib/Target/" + target["name"] + "/" + target["short_name"] + ".td",
        td_srcs = glob([
            "lib/Target/" + target["name"] + "/*.td",
            "include/llvm/CodeGen/*.td",
            "include/llvm/IR/Intrinsics*.td",
            "include/llvm/TableGen/*.td",
            "include/llvm/Target/*.td",
            "include/llvm/Target/GlobalISel/*.td",
        ]),
    )],
    [cc_library(
        name = target["name"] + "Info",
        srcs = ["lib/Target/" + target["name"] + "/TargetInfo/" + target["name"] + "TargetInfo.cpp"],
        hdrs = glob(["lib/Target/" + target["name"] + "/TargetInfo/*.h"]),
        copts = ["-Ilib/Target/" + target["name"]],
        visibility = ["//visibility:public"],
        deps = [
            ":" + target["name"] + "CommonTableGen",
            ":Support",
            ":Target",
        ],
    )],
    [cc_library(
        name = target["name"] + "UtilsAndDesc",
        srcs = glob([
            "lib/Target/" + target["name"] + "/MCTargetDesc/*.cpp",
            "lib/Target/" + target["name"] + "/Utils/*.cpp",
            "lib/Target/" + target["name"] + "/*.h",
        ]),
        hdrs = glob([
            "lib/Target/" + target["name"] + "/MCTargetDesc/*.h",
            "lib/Target/" + target["name"] + "/Utils/*.h",
        ]),
        copts = ["-Ilib/Target/" + target["name"]],
        textual_hdrs = glob([
            "lib/Target/" + target["name"] + "/*.def",
            "lib/Target/" + target["name"] + "/*.inc",
        ]),
        visibility = ["//visibility:public"],
        deps = [
            ":BinaryFormat",
            # Depending on `:CodeGen` headers in this library is almost
            # certainly a layering problem in numerous targets.
            ":CodeGen",
            ":DebugInfoCodeView",
            ":MC",
            ":MCDisassembler",
            ":Support",
            ":Target",
            ":config",
            ":" + target["name"] + "CommonTableGen",
            ":" + target["name"] + "Info",
        ],
    )],
    [cc_library(
        name = target["name"] + "CodeGen",
        srcs = glob([
            "lib/Target/" + target["name"] + "/GISel/*.cpp",
            "lib/Target/" + target["name"] + "/GISel/*.h",
            "lib/Target/" + target["name"] + "/*.cpp",
            "lib/Target/" + target["name"] + "/*.h",
        ]),
        hdrs = ["lib/Target/" + target["name"] + "/" + target["short_name"] + ".h"],
        copts = ["-Ilib/Target/" + target["name"]],
        textual_hdrs = glob([
            "lib/Target/" + target["name"] + "/*.def",
            "lib/Target/" + target["name"] + "/*.inc",
        ]),
        visibility = ["//visibility:public"],
        deps = [
            ":Analysis",
            ":BinaryFormat",
            ":CFGuard",
            ":CodeGen",
            ":Core",
            ":IPO",
            ":MC",
            ":Passes",
            ":ProfileData",
            ":Scalar",
            ":Support",
            ":Target",
            ":TransformUtils",
            ":Vectorize",
            ":config",
            ":" + target["name"] + "CommonTableGen",
            ":" + target["name"] + "Info",
            ":" + target["name"] + "UtilsAndDesc",
        ],
    )],
    [cc_library(
        name = target["name"] + "AsmParser",
        srcs = glob([
            "lib/Target/" + target["name"] + "/AsmParser/*.cpp",
            "lib/Target/" + target["name"] + "/AsmParser/*.h",
        ]),
        copts = ["-Ilib/Target/" + target["name"]],
        visibility = ["//visibility:public"],
        deps = [
            ":BinaryFormat",
            ":MC",
            ":MCParser",
            ":Support",
            ":Target",
            ":" + target["name"] + "CodeGen",
            ":" + target["name"] + "CommonTableGen",
            ":" + target["name"] + "UtilsAndDesc",
        ],
    )],
    [cc_library(
        name = target["name"] + "Disassembler",
        srcs = glob([
            "lib/Target/" + target["name"] + "/Disassembler/*.cpp",
            "lib/Target/" + target["name"] + "/Disassembler/*.c",
            "lib/Target/" + target["name"] + "/Disassembler/*.h",
        ]),
        copts = ["-Ilib/Target/" + target["name"]],
        visibility = ["//visibility:public"],
        deps = [
            ":CodeGen",
            ":Core",
            ":MC",
            ":MCDisassembler",
            ":Support",
            ":Target",
            ":" + target["name"] + "CodeGen",
            ":" + target["name"] + "CommonTableGen",
            ":" + target["name"] + "UtilsAndDesc",
        ],
    )],
] for target in llvm_target_list]
