

# Parsing the x86 instructions manual.

This file describes the internals of `parse_intel_sdm`: a tool to read Intel's
Software Development Manual (SDM). If you just want to use `parse_intel_sdm`,
have a look at the main [README](../../tools/README.md).

## Overview

The PDF itself has no explicit semantic structure to exploit. Most of the
structure is in the formatting: Each instruction is in a section that contains
an instruction table and a optional operand encoding table. Consequently, this
code reads the low-level drawing commands to extract the instruction
information.

*   We first extract a PDF representation into a
    [`PdfDocument`](../../proto/pdf/pdf_document.proto) protobuf that just
    adds some structure to the PDF data. For each page, characters are grouped
    in blocks of text, then futher organized into tables with rows and columns.
*   We apply a list of patches to the `PdfDocument` to fix some typos and
    formatting errors in the SDM. For the complete list of supported and tested
    versions, see the files in [`sdm_patches`](sdm_patches/).
*   The `PdfDocument` is then interpreted by detecting and parsing instruction
    and operand encoding tables. The result is an
    [`SDMDocument`](../../proto/pdf/pdf_document.proto) protobuf that
    represents the SDM-specific structure. At that point we still keep some PDF
    data for easier debugging.
*   Finally we convert the `SDMDocument` to the final
    [`ArchitectureProto`](../../proto/instructions.proto) representation.

