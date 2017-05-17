
# Parsing the x86 instructions manual.

## Motivation

There are many uses for a machine-readable x86 instruction set.
These instructions are defined in the Intel Software Development
Manual (SDM), in PDF format. The purpose of this code is to parse
instruction information from that PDF and export it into a structured database
of instructions in machine readable format.

## Get involved

*   Issue tracker: https://github.com/google/EXEgesis/issues
*   Mailing list: exegesis-discuss@googlegroups.com

We welcome patches -- see CONTRIBUTING for more information on how to submit a patch.

## Requirements/Building

To build the tool, you will need [`bazel`](http://bazel.io). Simply clone the
repository and run bazel:

```
git clone https://github.com/google/CPU-instructions
bazel -c opt build //exegesis/tools:parse_sdm
```

You don't need to worry about dependencies since bazel will download and build
them for you. The exact list of dependencies can be found in the
[`WORKSPACE` file](WORKSPACE).

### Known Issues

#### libunwind linking errors

In case you have libunwind installed on your system and compilation fails with undefined references:

```
undefined reference to `_ULx86_64_init_local'
undefined reference to `_Ux86_64_getcontext'
undefined reference to `_ULx86_64_get_reg'
undefined reference to `_ULx86_64_step'
```

Just add `--define libunwind=true` to the command line like so:

```
# Building
bazel -c opt build //exegesis/tools:parse_sdm --define libunwind=true

# Executing
bazel -c opt run exegesis/tools:parse_sdm --define libunwind=true -- \
  --exegesis_input_spec=/path/to/intel-sdm.pdf \
  --exegesis_output_file_base=/tmp/instructions
```

## Usage

To use this code, you will need to download the Intel SDM. This parser supports
at least the following versions of the manual:

*   December 2016: Intel® 64 and IA-32 architectures software developer’s manual
    combined volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, and 3D
*   September 2016: Intel® 64 and IA-32 Architectures Software Developer’s
    Manual Volume 2 (2A, 2B, 2C & 2D): Instruction Set Reference, A-Z

For the complete list of supported and tested versions, see the files in
[`sdm_patches`](exegesis/x86/pdf/sdm_patches/).

The most recent version of the SDM can be downloaded from the [Intel Developer
Zone](https://software.intel.com/en-us/articles/intel-sdm). The September 2016
and earlier versions can be found [in the Internet
Archive](https://web.archive.org/web/20161029005713/http://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-instruction-set-reference-manual-325383.pdf).

Here's a sample command line to parse all instructions, assuming that the manual
has been downloaded as `/path/to/intel-sdm.pdf`.

```
bazel -c opt run exegesis/tools:parse_sdm -- \
  --exegesis_input_spec=/path/to/intel-sdm.pdf \
  --exegesis_output_file_base=/tmp/instructions
```

## Output

The above command will create a file `/tmp/instructions.pbtxt` that contains an
[`InstructionSetProto`](exegesis/proto/instructions.proto) in protobuf
[text format](https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.text_format).


## Cleaning up the Database

After parsing, there are still mistakes and inconsistencies in the database of
instructions. We provide a way to apply rules or heuristics on the database to
fix these issues. These range from point fixes (e.g. "fix the binary encoding of
XBEGIN") to more complex heuristics (e.g. "replace use of 'reg' operands
with the right register depending of the size of the operand").

The transforms to apply can be specified in the `--exegesis_transforms`
flag. The flag can be a comma-separated list of transform names, or `default` to
apply default transforms. Note that this is different from an empty/unspecified
flag, where no transform is applied.

```
bazel -c opt run exegesis/tools:parse_sdm -- \
  --exegesis_input_spec=/path/to/intel-sdm.pdf \
  --exegesis_output_file_base=/tmp/instructions \
  --exegesis_transforms=default
```

The result is written to `/tmp/instructions_transformed.pbtxt`.

## More details

### Code Structure of the SDM Parser

The PDF itself has no explicit semantic structure to exploit. Most of the
structure is in the formatting: Each instruction is in a section that contains
an instruction table and a optional operand encoding table. Consequently, this
code reads the low-level drawing commands to extract the instruction
information.

*   We first extract a PDF representation into a
    [`PdfDocument`](exegesis/x86/pdf/pdf_document.proto) protobuf that
    just adds some structure to the PDF data. For each page, characters are
    grouped in blocks of text, then futher organized into tables with rows and
    columns.
*   We apply a list of patches to the `PdfDocument` to fix some typos and
    formatting errors in the SDM. The patches are given in the file
    [`pdf_document_patches.pbtxt`](exegesis/x86/pdf/pdf_document_patches.pbtxt).
*   The `PdfDocument` is then interpreted by detecting and parsing instruction
    and operand encoding tables. The result is an
    [`SDMDocument`](nexegesis/x86/pdf/pdf_document.proto) protobuf that
    represents the SDM-specific structure. At that point we still keep some
    PDF data for easier debugging.
*   Finally we convert the `SDMDocument` to the final `InstructionSetProto`
    representation.


### PDF Formatting Issues And Typos:

#### September 2016

This is a non-exhaustive list of additional issues with this version of the PDF:

*   Vendor syntax is missing for some instructions, e.g. `PACKUSDW`

#### June 2016

This is a non-exhaustive list of additional issues with this version of the PDF:

*   Some operand encoding tables are missing a right border.
*   Some validity modes are specified as `VV` instead of `V/V`.
*   Additional inconsistent column headers: `Operand Instruction` instead of
    `Operand/Instruction` when both are specified in the same column.
*   Additional ways in which the operand encoding format do not conform to the
    specification, e.g. `ModRM:r/m (r, ModRM:[7:6] must be 11b)`.
*   Page titles are not consistent between pages for multi-page intructions
    (e.g. `VEXP2PD`).

#### April 2016

*   Tables don't have a structure, they are just text. The content of each cell
    can be split on several lines. So we have to recreate the structure with
    some heuristics.
*   Table headers have typos and inconsistencies.
*   Table cell values do not follow section 3.1.1.5 of the documentation. For
    example, "Valid" is sometimes given as `Valid` and sometimes as `V`.
*   Sometimes two cells are merged, e.g. "Opcode" and "Instruction" into
    "Opcode/Instruction".
*   Page title is supposed to be `<ADDSPS>/.../<ADDSPQ>-Some Title` but some
    some whitespace gets thrown in randomly, and the hyphen might be a single or
    double one.
*   Notes are sometimes inside, sometimes outside the tables.
*   Tables that span several pages are sometimes rendered as two tables (e.g.
    `CMOVcc—Conditional Move`), sometimes one table spanning several pages
    without repeating the header (e.g. `PSLLW/PSLLD/PSLLQ—Shift Packed Data Left
    Logical`).
*   Operand encoding tables do not conform to the format that is specified. In
    particular, read/write markers are often missing and not capitalized
    correctly. Some formats like those for SIB of VSIB are not documented
    anywhere.
*   Cross-references for instruction tables to operand encoding tables are
    sometimes wrong (e.g. the instruction referes to 'RMV' but the has operand
    encoding table only has an 'A', which is not referred to by the instructions
    table).
*   Operand encoding tables are sometimes disregarding some implicit operands,
    (e.g. registers).
*   Operand encoding tables are missing for all x87 instructions.

