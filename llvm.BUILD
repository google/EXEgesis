

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
    name = "asm_parser",
    srcs = glob([
        "lib/AsmParser/*.cpp",
        "lib/AsmParser/*.h",
    ]),
    hdrs = [
        "include/llvm/AsmParser/Parser.h",
        "include/llvm/AsmParser/SlotMapping.h",
    ],
    deps = [
        ":ir",
        ":support",
    ],
)

cc_library(
    name = "asm_printer",
    srcs = glob([
        "lib/CodeGen/AsmPrinter/*.cpp",
        "lib/CodeGen/AsmPrinter/*.h",
    ]),
    hdrs = [
               "include/llvm/CodeGen/AsmPrinter.h",
           ] +
           glob(["lib/CodeGen/AsmPrinter/*.def"]),
    deps = [
        ":analysis",
        ":codegen",
        ":config",
        ":debug_info_codeview",
        ":ir",
        ":machine_code",
        ":machine_code_parser",
        ":support",
        ":target_base",
    ],
)

cc_library(
    name = "bit_reader",
    srcs = glob([
        "lib/Bitcode/Reader/*.cpp",
        "lib/Bitcode/Reader/*.h",
    ]) + [
        "include/llvm-c/BitReader.h",
    ],
    hdrs = [
        "include/llvm/Bitcode/BitCodes.h",
        "include/llvm/Bitcode/BitcodeReader.h",
        "include/llvm/Bitcode/BitstreamReader.h",
        "include/llvm/Bitcode/LLVMBitCodes.h",
    ],
    deps = [
        ":config",
        ":ir",
        ":support",
    ],
)

cc_library(
    name = "bit_writer",
    srcs = glob([
        "lib/Bitcode/Writer/*.cpp",
        "lib/Bitcode/Writer/*.h",
    ]) + [
        "include/llvm-c/BitWriter.h",
    ],
    hdrs = [
        "include/llvm/Bitcode/BitCodes.h",
        "include/llvm/Bitcode/BitcodeWriter.h",
        "include/llvm/Bitcode/BitcodeWriterPass.h",
        "include/llvm/Bitcode/BitstreamWriter.h",
        "include/llvm/Bitcode/LLVMBitCodes.h",
    ],
    deps = [
        ":analysis",
        ":config",
        ":ir",
        ":support",
    ],
)

cc_library(
    name = "config",
    hdrs = [
        "include/llvm/Config/AsmParsers.def",
        "include/llvm/Config/AsmPrinters.def",
        "include/llvm/Config/Disassemblers.def",
        "include/llvm/Config/Targets.def",
        "include/llvm/Config/config.h",
        "include/llvm/Config/llvm-config.h",
        "include/llvm/Config/abi-breaking.h",
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
    name = "analysis",
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
        ":config",
        ":ir",
        ":object",
        ":profiledata",
        ":support",
    ],
)

cc_library(
    name = "codegen",
    srcs = glob(
        [
            "lib/CodeGen/**/*.cpp",
            "lib/CodeGen/**/*.h",
        ],
        exclude = [
            "lib/CodeGen/AsmPrinter/*.cpp",
            "lib/CodeGen/AsmPrinter/*.h",
            "lib/CodeGen/SelectionDAG/*.cpp",
            "lib/CodeGen/SelectionDAG/*.h",
        ],
    ),
    hdrs = glob(
        [
            "include/llvm/CodeGen/*.h",
            "include/llvm/CodeGen/*.def",
            "include/llvm/CodeGen/**/*.h",
            "include/llvm/CodeGen/**/*.inc",
        ],
        exclude = [
            "include/llvm/CodeGen/AsmPrinter.h",
            "include/llvm/CodeGen/SelectionDAG*.h",
        ],
    ),
    visibility = ["//visibility:public"],
    deps = [
        ":asm_parser",
        ":analysis",
        ":bit_reader",
        ":bit_writer",
        ":config",
        ":ir",
        ":machine_code",
        ":support",
        ":target_base",
        ":transform_utils",
        ":scalar_transforms",
    ],
)

cc_library(
    name = "debug_info_codeview",
    srcs = glob([
        "lib/DebugInfo/CodeView/*.cpp",
        "lib/DebugInfo/CodeView/*.h",
    ]),
    hdrs = glob([
        "include/llvm/DebugInfo/CodeView/*.h",
        "include/llvm/DebugInfo/CodeView/*.def",
    ]),
    deps = [
        ":binary_format",
        ":support",
    ],
    includes = ["include/llvm/DebugInfo/CodeView"],
)

cc_library(
    name = "demangle",
    srcs = glob([
        "lib/Demangle/*.cpp",
        "lib/Demangle/*.h",
    ]),
    hdrs = glob(["include/llvm/Demangle/*.h"]),
    visibility = ["//visibility:public"],
    deps = [":config"],
    includes = ["include"],
)

cc_library(
    name = "tablegen",
    srcs = glob([
        "lib/TableGen/*.cpp",
        "lib/TableGen/*.h",
    ]),
    hdrs = glob(["include/llvm/TableGen/*.h"]),
    linkopts = ["-ldl"],
    visibility = ["//visibility:public"],
    deps = [
        ":config",
        ":support",
    ],
    includes = ["include"],
)

cc_library(
    name = "execution_engine",
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
    hdrs = [
        "include/llvm/ExecutionEngine/ExecutionEngine.h",
        "include/llvm/ExecutionEngine/GenericValue.h",
        "include/llvm/ExecutionEngine/JITEventListener.h",
        "include/llvm/ExecutionEngine/JITSymbol.h",
        "include/llvm/ExecutionEngine/ObjectCache.h",
        "include/llvm/ExecutionEngine/RTDyldMemoryManager.h",
        "include/llvm/ExecutionEngine/RuntimeDyld.h",
        "include/llvm/ExecutionEngine/RuntimeDyldChecker.h",
        "include/llvm/ExecutionEngine/SectionMemoryManager.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":codegen",
        ":config",
        ":ir",
        ":machine_code_disassembler",
        ":object",
        ":support",
        ":target_base",
    ],
)

cc_library(
    name = "mcjit",
    srcs = glob([
        "lib/ExecutionEngine/MCJIT/*.cpp",
        "lib/ExecutionEngine/MCJIT/*.h",
    ]),
    hdrs = glob(["include/llvm/ExecutionEngine/MCJIT*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        ":codegen",
        ":config",
        ":execution_engine",
        ":ir",
        ":machine_code",
        ":object",
        ":support",
        ":target_base",
    ],
)

cc_library(
    name = "llvm-tblgen-lib",
    srcs = glob([
        "utils/TableGen/*.cpp",
        "utils/TableGen/*.h",
        "include/llvm/CodeGen/*.h",
        "include/llvm/MC/*.h",
        "lib/Target/X86/Disassembler/X86DisassemblerDecoderCommon.h",
    ]),
    hdrs = [
        "include/llvm/Support/TargetOpcodes.def",
    ],
    linkopts = ["-ldl"],
    deps = [
        ":config",
        ":support",
        ":tablegen",
    ],
    includes = ["include"],
)

cc_binary(
    name = "llvm-tblgen",
    linkopts = ["-ldl", "-lm", "-lpthread"],
    stamp = 0,
    deps = [
        ":llvm-tblgen-lib",
    ],
    includes = ["include"],
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

gentbl(
    name = "attributes_gen",
    tbl_outs = [("-gen-attrs", "include/llvm/IR/Attributes.inc")],
    tblgen = ":llvm-tblgen",
    td_file = "include/llvm/IR/Attributes.td",
    td_srcs = ["include/llvm/IR/Attributes.td"],
)

gentbl(
    name = "attributes_compat_gen",
    tbl_outs = [("-gen-attrs", "lib/IR/AttributesCompatFunc.inc")],
    tblgen = ":llvm-tblgen",
    td_file = "lib/IR/AttributesCompatFunc.td",
    td_srcs = [
        "lib/IR/AttributesCompatFunc.td",
        "include/llvm/IR/Attributes.td",
    ],
)

cc_library(
    name = "binary_format",
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
        ":support",
    ],
)

cc_library(
    name = "ir",
    srcs = glob([
        "lib/IR/*.cpp",
        "lib/IR/*.h",
        "include/llvm/IR/*.inc",
        "include/llvm/Analysis/*.h",
    ]) + [
        "include/llvm/Bitcode/BitcodeReader.h",
        "include/llvm/Bitcode/BitCodes.h",
        "include/llvm/CodeGen/ValueTypes.h",
        "include/llvm-c/Core.h",
    ],
    hdrs = glob(
        [
            "include/llvm/*.h",
            "include/llvm/Analysis/*.def",
            "include/llvm/IR/*.h",
            "include/llvm/IR/*.def",
            "include/llvm/IR/*.td",
            "include/llvm/Assembly/*Writer.h",
            "include/llvm/CodeGen/*.td",
        ],
        exclude = [
            "include/llvm/LinkAllPasses.h",
            "include/llvm/LinkAllIR.h",
        ],
    ),
    visibility = ["//visibility:public"],
    includes = ["include"],
    deps = [
        ":attributes_compat_gen",
        ":attributes_gen",
        ":binary_format",
        ":config",
        ":intrinsic_enums_gen",
        ":intrinsics_impl_gen",
        ":support",
    ],
)

cc_library(
    name = "ir_reader",
    srcs = glob([
        "lib/IRReader/*.cpp",
        "lib/IRReader/*.h",
    ]),
    hdrs = glob(["include/llvm/IRReader/*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        ":asm_parser",
        ":bit_reader",
        ":config",
        ":ir",
        ":support",
    ],
)

cc_library(
    name = "linker",
    srcs = glob([
        "lib/Linker/*.cpp",
        "lib/Linker/*.h",
    ]),
    hdrs = glob(["include/llvm/Linker/*.h"]),
    deps = [
        ":config",
        ":ir",
        ":support",
        ":transform_utils",
    ],
)

cc_library(
    name = "machine_code",
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
        ":binary_format",
        ":config",
        ":debug_info_codeview",
        ":support",
    ],
)

cc_library(
    name = "machine_code_disassembler",
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
        ":binary_format",
        ":config",
        ":machine_code",
        ":support",
    ],
)

cc_library(
    name = "machine_code_parser",
    srcs = glob([
        "lib/MC/MCParser/*.cpp",
        "lib/MC/MCParser/*.h",
    ]),
    hdrs = glob(["include/llvm/MC/MCParser/*.h"]),
    visibility = ["//visibility:public"],
    deps = [
        ":config",
        ":machine_code",
        ":support",
    ],
)

cc_library(
    name = "object",
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
        ":bit_reader",
        ":ir",
        ":machine_code",
        ":machine_code_parser",
        ":support",
    ],
)

cc_library(
    name = "profiledata",
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
        ":ir",
        ":support",
    ],
)

cc_library(
    name = "selection_dag",
    srcs = glob([
        "lib/CodeGen/SelectionDAG/*.cpp",
        "lib/CodeGen/SelectionDAG/*.h",
    ]),
    hdrs = glob(["include/llvm/CodeGen/SelectionDAG*.h"]),
    deps = [
        ":analysis",
        ":asm_printer",
        ":codegen",
        ":config",
        ":ir",
        ":support",
    ],
)

cc_library(
    name = "support",
    srcs = glob([
        "lib/Support/*.c",
        "lib/Support/*.cpp",
        "lib/Support/*.h",
        "lib/Support/*.inc",
        "lib/Support/Unix/*.h",
        "lib/Support/Unix/*.inc",
        "include/llvm-c/*.h",
    ]) + [
        "include/llvm/Support/DataTypes.h",
        "include/llvm/Support/VCSRevision.h",
    ],
    hdrs = glob([
        "include/llvm/Support/*.h",
        "include/llvm/Support/*.def",
        "include/llvm/Support/**/*.def",
        "include/llvm/ADT/*.h",
    ]),
    includes = ["include"],
    linkopts = ["-ldl"],
    visibility = ["//visibility:public"],
    deps = [
        ":config",
        ":demangle",
    ],
)

cc_library(
    name = "target_base",
    srcs = glob([
        "lib/Target/*.cpp",
        "lib/Target/*.h",
        "include/llvm/CodeGen/*.h",
    ]) + [
        "include/llvm-c/Initialization.h",
        "include/llvm-c/Target.h",
    ],
    hdrs = glob([
        "include/llvm/CodeGen/*.def",
        "include/llvm/Target/*.h",
        "include/llvm/Target/*.def",
    ]),
    visibility = ["//visibility:public"],
    deps = [
        ":analysis",
        ":config",
        ":ir",
        ":machine_code",
        ":support",
    ],
)

# Transforms.

cc_library(
    name = "transform_base",
    hdrs = glob(["include/llvm/Transforms/*.h"]),
    deps = [
        ":analysis",
        ":config",
        ":ir",
        ":support",
    ],
)

cc_library(
    name = "transform_utils",
    srcs = glob([
        "lib/Transforms/Utils/*.cpp",
        "lib/Transforms/Utils/*.h",
        "include/llvm/CodeGen/Passes.h",
        "include/llvm-c/Transforms/Utils.h",
    ]),
    hdrs = glob(["include/llvm/Transforms/Utils/*.h"]),
    deps = [
        ":transform_base",
        ":analysis",
        ":config",
        ":ir",
        ":support",
    ],
)

cc_library(
    name = "aggressiveinstcombine_transforms",
    srcs = glob([
        "lib/Transforms/AggressiveInstCombine/*.cpp",
        "lib/Transforms/AggressiveInstCombine/*.h",
    ]),
    hdrs = [
        "include/llvm-c/Transforms/AggressiveInstCombine.h",
        "include/llvm/Transforms/AggressiveInstCombine/AggressiveInstCombine.h",
    ],
    deps = [
        ":analysis",
        ":support",
        ":transform_utils",
    ],
)

cc_library(
    name = "coroutine_transforms",
    srcs = glob([
        "lib/Transforms/Coroutines/*.cpp",
        "lib/Transforms/Coroutines/*.h",
    ]) + [
        "include/llvm/Transforms/IPO/PassManagerBuilder.h",
    ],
    hdrs = ["include/llvm/Transforms/Coroutines.h"],
    deps = [
        ":analysis",
        ":config",
        ":ir",
        ":support",
        ":transform_base",
        ":transform_utils",
    ],
)

cc_library(
    name = "instcombine_transforms",
    srcs = glob([
        "lib/Transforms/InstCombine/*.cpp",
        "lib/Transforms/InstCombine/*.h",
        "include/llvm/Transforms/InstCombine/*.h",
        "include/llvm-c/Transforms/InstCombine.h",
    ]),
    deps = [
        ":analysis",
        ":config",
        ":instcombine_transforms_gen",
        ":ir",
        ":support",
        ":transform_base",
        ":transform_utils",
    ],
)

gentbl(
    name = "instcombine_transforms_gen",
    tbl_outs = [(
        "-gen-searchable-tables",
        "lib/Transforms/InstCombine/InstCombineTables.inc",
    )],
    tblgen = ":llvm-tblgen",
    td_file = "lib/Transforms/InstCombine/InstCombineTables.td",
    td_srcs = glob([
        "include/llvm/CodeGen/*.td",
        "include/llvm/IR/Intrinsics*.td",
    ]) + ["include/llvm/TableGen/SearchableTable.td"],
)

cc_library(
    name = "instrumentation_transforms",
    srcs = glob([
        "lib/Transforms/Instrumentation/*.cpp",
        "lib/Transforms/Instrumentation/*.h",
        "include/llvm/Transforms/Instrumentation/*.h",
    ]),
    deps = [
        ":analysis",
        ":config",
        ":ir",
        ":profiledata",
        ":support",
        ":transform_base",
        ":transform_utils",
    ],
)

cc_library(
    name = "ipo_transforms",
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
        ":config",
        ":bit_writer",
        ":ir",
        ":ir_reader",
        ":linker",
        ":scalar_transforms",
        ":support",
        ":target_base",
        ":transform_base",
        ":transform_utils",
        ":instrumentation_transforms",
    ],
)

cc_library(
    name = "objcarc_transforms",
    srcs = glob([
        "lib/Transforms/ObjCARC/*.cpp",
        "lib/Transforms/ObjCARC/*.h",
    ]),
    hdrs = ["include/llvm/Transforms/ObjCARC.h"],
    deps = [
        ":analysis",
        ":config",
        ":ir",
        ":support",
        ":target_base",
        ":transform_utils",
    ],
)

cc_library(
    name = "scalar_transforms",
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
        ":analysis",
        ":config",
        ":instcombine_transforms",
        ":aggressiveinstcombine_transforms",
        ":ir",
        ":profiledata",
        ":support",
        ":target_base",
        ":transform_base",
        ":transform_utils",
    ],
)

cc_library(
    name = "vectorize_transforms",
    srcs = glob([
        "lib/Transforms/Vectorize/*.cpp",
        "lib/Transforms/Vectorize/*.h",
        "include/llvm/Transforms/Vectorize/*.h",
    ]) + [
        "include/llvm-c/Transforms/Vectorize.h",
        "include/llvm/Transforms/Scalar/LoopPassManager.h",
    ],
    hdrs = [
        "include/llvm/Transforms/Vectorize.h",
        "include/llvm/Transforms/Scalar/LoopPassManager.h",
    ],
    deps = [
        ":analysis",
        ":config",
        ":ir",
        ":transform_base",
        ":transform_utils",
        ":support",
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
        name = target["lower_name"] + "_target_gen",
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
        name = target["lower_name"] + "_target_info",
        srcs = [
            "lib/Target/" + target["name"] + "/TargetInfo/" + target["name"] + "TargetInfo.cpp",
        ],
        hdrs = [
            "lib/Target/" + target["name"] + "/MCTargetDesc/" + target["name"] + "MCTargetDesc.h",
        ],
        copts = ["-Ilib/Target/" + target["name"]],
        visibility = ["//visibility:public"],
        deps = [
            ":target_base",
            ":" + target["lower_name"] + "_target_gen",
        ],
    )],
    [cc_library(
        name = target["lower_name"] + "_target_utils",
        srcs = glob(["lib/Target/" + target["name"] + "/Utils/*.cpp"]),
        hdrs = glob(["lib/Target/" + target["name"] + "/Utils/*.h"]),
        copts = ["-Ilib/Target/" + target["name"]],
        visibility = ["//visibility:public"],
        deps = [
            ":codegen",
            ":machine_code",
            ":target_base",
        ],
    )],
    [cc_library(
        name = target["lower_name"] + "_asm_printer",
        srcs = glob(
            ["lib/Target/" + target["name"] + "/InstPrinter/*.cpp"],
        ) + [
            "lib/Target/X86/MCTargetDesc/X86BaseInfo.h",
        ],
        hdrs = glob(["lib/Target/" + target["name"] + "/InstPrinter/*.h"]),
        copts = ["-Ilib/Target/" + target["name"]],
        visibility = ["//visibility:public"],
        deps = [
            ":machine_code",
            ":support",
            ":" + target["lower_name"] + "_target_gen",
            ":" + target["lower_name"] + "_target_info",
            ":" + target["lower_name"] + "_target_utils",
        ],
    )],
    [cc_library(
        name = target["lower_name"] + "_mc_target_desc",
        srcs = glob(["lib/Target/" + target["name"] + "/MCTargetDesc/*.cpp"]),
        hdrs = glob(["lib/Target/" + target["name"] + "/MCTargetDesc/*.h"]),
        copts = ["-Ilib/Target/" + target["name"]],
        visibility = ["//visibility:public"],
        deps = [
            ":config",
            ":machine_code",
            ":machine_code_disassembler",
            ":support",
            ":target_base",
            ":" + target["lower_name"] + "_target_gen",
            ":" + target["lower_name"] + "_asm_printer",
            ":" + target["lower_name"] + "_target_utils",
            ":" + target["lower_name"] + "_target_info",
        ],
    )],
    [cc_library(
        name = target["lower_name"] + "_target",
        srcs = glob([
            "lib/Target/" + target["name"] + "/*.cpp",
            "lib/Target/" + target["name"] + "/*.h",
        ]),
        hdrs = glob(["lib/Target/" + target["name"] + "/" + target["short_name"] + "*.inc"]) +
               [
                   "lib/Target/" + target["name"] + "/" + target["short_name"] + ".h",
                   "lib/Target/" + target["name"] + "/" + target["short_name"] + "GenRegisterBankInfo.def",
               ],
        copts = ["-Ilib/Target/" + target["name"]],
        visibility = ["//visibility:public"],
        deps = [
            ":asm_printer",
            ":codegen",
            ":config",
            ":ipo_transforms",
            ":ir",
            ":machine_code",
            ":scalar_transforms",
            ":selection_dag",
            ":support",
            ":target_base",
            ":vectorize_transforms",
            ":" + target["lower_name"] + "_asm_printer",
            ":" + target["lower_name"] + "_mc_target_desc",
            ":" + target["lower_name"] + "_target_gen",
            ":" + target["lower_name"] + "_target_utils",
            ":" + target["lower_name"] + "_target_info",
        ],
    )],
    [cc_library(
        name = target["lower_name"] + "_target_asm_parser",
        srcs = glob([
            "lib/Target/" + target["name"] + "/AsmParser/*.cpp",
            "lib/Target/" + target["name"] + "/AsmParser/*.h",
        ]),
        copts = ["-Ilib/Target/" + target["name"]],
        visibility = ["//visibility:public"],
        deps = [
            ":machine_code",
            ":support",
            ":machine_code_parser",
            ":target_base",
            ":" + target["lower_name"] + "_target",
            ":" + target["lower_name"] + "_target_utils",
        ],
    )],
    [cc_library(
        name = target["lower_name"] + "_target_disassembler",
        srcs = glob([
            "lib/Target/" + target["name"] + "/Disassembler/*.cpp",
            "lib/Target/" + target["name"] + "/Disassembler/*.c",
            "lib/Target/" + target["name"] + "/Disassembler/*.h",
        ]),
        copts = ["-Ilib/Target/" + target["name"]],
        visibility = ["//visibility:public"],
        deps = [
            ":codegen",
            ":ir",
            ":machine_code",
            ":support",
            ":target_base",
            ":" + target["lower_name"] + "_target",
            ":" + target["lower_name"] + "_target_utils",
        ],
    )],
] for target in llvm_target_list]
