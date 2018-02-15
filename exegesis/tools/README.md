

# Overview

As we develop utilities to better understand CPUs, we'll document them here. The
tools available in this directory currently focus on extracting the
[ISA](https://en.wikipedia.org/wiki/Instruction_set_architecture) information
for various vendors into a unified
[`ArchitectureProto`](../proto/instructions.proto) format:

-   [Intel x86-64](#intel-x86-64)
-   [ARM A64](#arm-a64)
-   [IBM POWER](#ibm-power) _under development_

We also provide some [generic PDF tools](#generic-pdf-tools) to help with
debugging.

# Prerequisites

First install [`bazel`](http://bazel.io), Google's own build tool. It will
download and build the [dependencies](../../WORKSPACE) for you.

You also need the source code, so clone the repository with the following
command:

```shell
git clone https://github.com/google/EXEgesis
```

# Intel x86-64

`parse_intel_sdm` extracts instruction information from Intel's Software
Development Manual (SDM). We provide a bazel rule that automatically downloads
the latest tested SDM version, and parses it using the default set of cleanups.


You can also download the most recent published version of the SDM (which might
not be supported by our tools yet) from the [Intel Developer
Zone](https://software.intel.com/en-us/articles/intel-sdm) and run the
`parse_intel_sdm` tool manually.

For the complete list of supported and tested versions, see
[`sdm_patches`](../x86/pdf/sdm_patches/). \
Please [file a bug](https://github.com/google/EXEgesis/issues) if the SDM
version you downloaded is unsupported.

### Usage

To download and parse the most recent version of the SDM, run

```shell
bazel build -c opt //exegesis/data/x86:intel_instruction_sets.pbtxt
```

The parsed data will then be located in
`bazel-genfiles/exegesis/data/x86/intel_instruction_sets.pbtxt`.

#### Manual parsing

The following command parses `/path/to/intel-sdm.pdf`, patches it to fix typos,
extracts the instructions and removes mistakes and inconsistencies from the
resulting database:

```shell
bazel run -c opt //exegesis/tools:parse_intel_sdm -- \
  --exegesis_input_spec=/path/to/intel-sdm.pdf \
  --exegesis_output_file_base=/tmp/intel_isa \
  --exegesis_transforms=default
```

The `--exegesis_transforms` flag specifies which cleanups to apply.

This creates the following files:

-   `/tmp/intel_isa.pbtxt` contains the raw data as parsed from the PDF.
-   `/tmp/intel_isa_transformed.pbtxt` contains the cleaned up version with
    transforms applied.

They both contain an [`ArchitectureProto`](../proto/instructions.proto)
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
bazel run -c opt //exegesis/tools:parse_intel_sdm --define libunwind=true --  \
  --exegesis_input_spec=/path/to/intel-sdm.pdf  \
  --exegesis_output_file_base=/tmp/intel_isa \
  --exegesis_transforms=default
```

### More details

For more details, have a look at the local [README](../x86/pdf/README.md).

# ARM A64

`parse_arm_xml` generates architecture information for the ARM v8-A ISA. You
will need to download the ARM v8-A XML instruction database available
at https://developer.arm.com/products/architecture/a-profile/exploration-tools
and extract its content locally on your machine. This tool supports at least the
following versions:

* [A64_v83A_ISA_xml_00bet6](https://developer.arm.com/-/media/developer/products/architecture/armv8-a-architecture/A64_v83A_ISA_xml_00bet6.tar.gz)
* [A64_v83A_ISA_xml_00bet5](https://developer.arm.com/-/media/developer/products/architecture/armv8-a-architecture/A64_v83A_ISA_xml_00bet5.tar.gz)
* [A64_v83A_ISA_xml_00bet4](https://developer.arm.com/-/media/developer/products/architecture/armv8-a-architecture/A64_v83A_ISA_xml_00bet4.tar.gz)
* [A64_v82A_ISA_xml_00bet3.1](https://developer.arm.com/-/media/developer/products/architecture/armv8-a-architecture/A64_v82A_ISA_xml_00bet3.1.tar.gz)

You can find more information on the ARM v8-A XML architecture specification on
https://alastairreid.github.io/alastairreid.github.io/ARM-v8a-xml-release/

### Usage

The following command parses all instructions, assuming that XML files reside
inside `/path/to/xml-directory/`:

```shell
bazel run -c opt //exegesis/tools:parse_arm_xml -- \
  --exegesis_arm_xml_path=/path/to/xml-directory/ \
  --exegesis_xml_database_output_file=/tmp/arm_xml_database.pbtxt \
  --exegesis_isa_output_file=/tmp/arm_isa.pbtxt
```

The above command will create the following files in protobuf
[text format](https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.text_format):

-   `/tmp/arm_xml_database.pbtxt` contains a raw translation of the instruction
    database as an [`XmlDatabase`](../arm/xml/parser.proto) proto.
-   `/tmp/arm_isa.pbtxt` contains the same data but further transformed into the
    [`ArchitectureProto`](../proto/instructions.proto) unified format.

### More details

For more details, have a look at the local [README](../arm/xml/README.md).

# Generic PDF tools

This section describes a set of more generic tools that we built to work with
PDF files. We don't expect you to use them but it's helpful for debugging
purposes or to port patches from one PDF version to another.

### `pdf2proto`

Transforms a PDF file into a
[PdfDocument](../proto/pdf/pdf_document.proto).

```shell
bazel run -c opt //exegesis/tools:pdf2proto --    \
  --exegesis_pdf_input_file=/path/to/pdf_file.pdf \
  --exegesis_pdf_output_file=/tmp/pdf_file.pdf.pb
```

### `proto_patch_helper`

Greps through a [PdfDocument](../proto/pdf/pdf_document.proto) and
displays a [PdfDocumentChange](../proto/pdf/pdf_document.proto) for all
the matching [PdfTextBlock](../proto/pdf/pdf_document.proto)s.

```shell
bazel run -c opt //exegesis/tools::proto_patch_helper -- \
  --exegesis_proto_input_file=/path/to/pdf_file.pdf.pb  \
  --exegesis_match_expression='<regex pattern>'
```

### `proto_patch_migrate`

Migrates a set of [PdfDocumentChanges](../proto/pdf/pdf_document.proto)
from PdfDocument to PdfDocument.

```shell
bazel run -c opt //exegesis/tools:proto_patch_migrate -- \
  --exegesis_from_proto_file=/path/from_pdf_file.pdf.pb \
  --exegesis_to_proto_file=/path/to_pdf_file.pdf.pb \
  --exegesis_output_file_base=/tmp/migration_base
```

