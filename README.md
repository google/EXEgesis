

# Goal

Google's EXEgesis project aims to improve code generation in compilers, via:

1.  Providing machine-readable lists of instructions for [hardware
    vendors](https://en.wikipedia.org/wiki/List_of_computer_hardware_manufacturers#Central_processing_units_.28CPUs.29)
    and [microarchitectures](https://en.wikipedia.org/wiki/Microarchitecture).
2.  Inferring latencies and
    [µOps](https://en.wikipedia.org/wiki/Micro-operation) scheduling for each
    instruction/microarchitecture pair.
3.  Providing tools for debugging the performance of code based on this data.

For a high-level overview of our efforts, see the
[slides](https://goo.gl/koSKFK) for a tech talk about EXEgesis (July 2017).

## Details

This repository provides a set of [tools](exegesis/tools/README.md) for
extracting data about instructions and latencies from canonical sources and
converting them into machine-readable form. Some require parsing PDF files;
others are more straightforward.

When latencies and µOps scheduling are not available in the documentation, we
auto generate benchmarks to measure them.

The output data is available in the form of a [Protocol
Buffer](https://developers.google.com/protocol-buffers/)
[message](exegesis/proto/microarchitecture.proto).

It includes:

-   A textual description. e.g. `Add with carry imm8 to AL.`
-   The raw encoding. e.g. `14 ib` and equivalent LLVM mnemonic. e.g. `ADC8i8`
-   Per-microarchitecture instruction latencies. e.g. `min_latency: 2,
    max_latency: 2`
-   Per-microarchitecture instruction schedulings. e.g. `Port 0 or 1 or 5 or 6`
    -   This identifies the [execution
        units](https://en.wikipedia.org/wiki/Execution_unit) on which the µOps
        can be scheduled.
    -   For example, here is the description of [Intel Haswell
        Microarchitecture](https://en.wikichip.org/wiki/intel/microarchitectures/haswell#Block_Diagram),
        it contains 7 ports, the `Add with carry imm8 to AL` instruction above
        can execute on ports 0, 1, 5 or 6.

## What's Next

-   Intel x86-64 - [done](exegesis/x86/pdf/README.md)
-   IBM POWER - underway
-   ARM Cortex - underway

## Get Involved

*   Issue tracker: https://github.com/google/EXEgesis/issues
*   Mailing list: <exegesis-discuss@googlegroups.com>

We welcome patches -- see [CONTRIBUTING](CONTRIBUTING) for more information on
how to submit a patch.

