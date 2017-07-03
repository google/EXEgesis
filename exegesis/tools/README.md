

# Overview

As we develop utilities to better understand CPUs, we'll document them here.
Currently, they're all related to parsing PDFs:

-   [`parse_intel_sdm`](#intel-x86-64) (Intel x86-64 only): Transforms the
    Intel's Software Development Manual (SDM) into an
    [`InstructionSetProto`](exegesis/proto/instructions.proto).
-   [`pdf2proto`](#pdf2proto): Transforms a PDF file into a
    [PdfDocument](exegesis/proto/pdf/pdf_document.proto).
-   [`proto_patch_helper`](#proto_patch_helper): Greps through a
    [PdfDocument](exegesis/proto/pdf/pdf_document.proto) to help creating PDF
    patches.
-   [`proto_patch_migrate`](#proto_patch_migrate): Migrates a set of patches
    from one [PdfDocument](exegesis/proto/pdf/pdf_document.proto) to another.

This file presents a set of tools to extract
[ISA](https://en.wikipedia.org/wiki/Instruction_set_architecture) information
for various vendors.

-   [Intel x86-64](#intel-x86-64)
-   [IBM POWER](#ibm-power) _under development_
-   [ARM A64](#arm-a64) _under development_

We also provide some [generic PDF tools](#generic-pdf-tools) to help with
debugging.

# Prerequisites

First install [`bazel`](http://bazel.io), Google's own build tool. It will
download and build the [dependencies](/WORKSPACE) for you.

You also need the source code, so clone the repository with the following
command:

```shell
git clone https://github.com/google/EXEgesis
```

# Intel x86-64

We have one x86-64 specific tool: `parse_intel_sdm`, which extracts instruction
information from Intel's Software Development Manual (SDM). You will need to
download the most recent version from the [Intel Developer
Zone](https://software.intel.com/en-us/articles/intel-sdm).

For the complete list of supported and tested versions, see
[`sdm_patches`](exegesis/x86/pdf/sdm_patches). Please [file a
bug](https://github.com/google/EXEgesis/issues) if the SDM version you
downloaded is unsupported.

### Usage

The following command parses `/path/to/intel-sdm.pdf`, patches it to fix typos,
extracts the instructions and removes mistakes and inconsistencies from the
resulting database.

```shell
bazel -c opt run //exegesis/tools:parse_intel_sdm --  \
  --exegesis_input_spec=/path/to/intel-sdm.pdf  \
  --exegesis_output_file_base=/tmp/instructions \
  --exegesis_transforms=default
```

The `--exegesis_transforms` flag specifies which cleanups to apply.

This creates the following files:

-   `/tmp/instructions.pbtxt` contains the raw data as parsed from the PDF.
-   `/tmp/instructions_transformed.pbtxt` contains the cleaned up version with
    transforms applied.

They both contain an [`InstructionSetProto`](exegesis/proto/instructions.proto)
in [Protobuf text
format](https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.text_format).

Note that if you don't specify `--exegesis_transforms`, no cleanup is performed
and only the raw output file is generated.

### Known issue: libunwind linking errors

In case you have libunwind installed on your system and compilation fails with
undefined references:

```
undefined reference to `_ULx86_64_init_local'
undefined reference to `_Ux86_64_getcontext'
undefined reference to `_ULx86_64_get_reg'
undefined reference to `_ULx86_64_step'
```

Just add `--define libunwind=true` to the command line like so:

```shell
bazel -c opt run //exegesis/tools:parse_intel_sdm --define libunwind=true --  \
  --exegesis_input_spec=/path/to/intel-sdm.pdf  \
  --exegesis_output_file_base=/tmp/instructions \
  --exegesis_transforms=default
```

### More details

For more details, have a look at the tool's
[README](exegesis/x86/pdf/README.md).

## IBM POWER

_Still under development. Please come back later._

## ARM A64

_Still under development. Please come back later._

## Generic PDF tools

This section describes a set of more generic tools that we built to work with
PDF files. We don't expect you to use them but it's helpful for debugging
purposes or to port patches from one PDF version to another.

### `pdf2proto`

Transforms a PDF file into a
[PdfDocument](exegesis/proto/pdf/pdf_document.proto).

```shell
bazel run -c opt //exegesis/tools:pdf2proto --    \
  --exegesis_pdf_input_file=/path/to/pdf_file.pdf \
  --exegesis_pdf_output_file=/tmp/pdf_file.pdf.pb
```

### `proto_patch_helper`

Greps through a [PdfDocument](exegesis/proto/pdf/pdf_document.proto) and
displays a [PdfDocumentChange](exegesis/proto/pdf/pdf_document.proto) for all
the matching [PdfTextBlock](exegesis/proto/pdf/pdf_document.proto)s.

```shell
bazel run -c opt //exegesis/tools::proto_patch_helper -- \
  --exegesis_proto_input_file=/path/to/pdf_file.pdf.pb  \
  --exegesis_match_expression='<regex pattern>'
```

### `proto_patch_migrate`

Migrates a set of [PdfDocumentChanges](exegesis/proto/pdf/pdf_document.proto)
from PdfDocument to PdfDocument.

```shell
bazel run -c opt  //exegesis/tools:proto_patch_migrate -- \
  --exegesis_from_proto_file=/path/from_pdf_file.pdf.pb \
  --exegesis_to_proto_file=/path/to_pdf_file.pdf.pb \
  --exegesis_output_file_base=/tmp/migration_base
```

