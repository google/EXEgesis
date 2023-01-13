# State of the repository

**As of January 2023, this project is no longer being maintained.** Many parts
of this projects have open-source alternatives that are part of a bigger effort
and that are actively maintained:

-   [llvm-exegesis](https://llvm.org/docs/CommandGuide/llvm-exegesis.html)
    allows analyzing individual instructions or snippets of assembly code.
-   [llvm-mca](https://llvm.org/docs/CommandGuide/llvm-mca.html) is a CPU
    pipeline simulation tool.
-   [uops.info](https://uops.info/) provides detailed performance
    characteristics and a machine-readable database of x86-64 instructions.

# Goal

Google's EXEgesis project aims to improve code generation in compilers, via:

1.  Providing machine-readable lists of instructions for [hardware
    vendors](https://en.wikipedia.org/wiki/List_of_computer_hardware_manufacturers#Central_processing_units_.28CPUs.29)
    and [microarchitectures](https://en.wikipedia.org/wiki/Microarchitecture).
2.  Providing tools for debugging the performance of code based on this data.

For a high-level overview of our efforts, see the
[slides](https://goo.gl/koSKFK) for a tech talk about EXEgesis (July 2017).

We are providing tools to measure instruction latencies and
[µOps](https://en.wikipedia.org/wiki/Micro-operation) scheduling. We have
contributed that part into LLVM as as the
[`llvm-exegesis`](https://llvm.org/docs/CommandGuide/llvm-exegesis.html) tool.

## Details

This repository provides a set of [tools](exegesis/tools/README.md) for
extracting data about instructions and latencies from canonical sources and
converting them into machine-readable form. Some require parsing PDF files;
others are more straightforward.

The output data is available in the form of a [Protocol
Buffer](https://developers.google.com/protocol-buffers/)
[message](exegesis/proto/microarchitecture.proto).

It includes:

-   A textual description. e.g. `Add with carry imm8 to AL.`
-   The raw encoding. e.g. `14 ib` and equivalent LLVM mnemonic. e.g. `ADC8i8`

## What's Next

-   Intel x86-64 - [done](exegesis/x86/pdf/README.md)
-   IBM POWER - underway
-   ARM Cortex - underway

## Get Involved

*   Issue tracker: https://github.com/google/EXEgesis/issues
*   Mailing list: <exegesis-discuss@googlegroups.com>

We welcome patches -- see [CONTRIBUTING](CONTRIBUTING) for more information on
how to submit a patch.
