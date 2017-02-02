Investigating differences between 16 and 8/32/64 bit ALU operations on Haswell.

16-bit ALU instructions have a different port usage pattern than 8/32/64-bit ALU
instructions: They typically uses p6 instead of p0.

**explanation**

The issue is that the 16-bit version uses a length-changing prefix (LCP) that
stalls the decoder (see the `Optimization Reference Manual, "Length-Changing
Prefixes (LCP)"` for more details). Here are encodings for three different
operand widths:

|      instruction       |  encoding        |
|------------------------|------------------|
| `add $0x7e,%al`        | `04 7e`          |
| `add $0x7ffe,%ax`      | `66 05 fe 7f`    |
| `add $0x7ffffffe,%eax` | `05 fe ff ff 7f` |

Note the **66H** prefix for the 16-bit version.

If the prefix is mandatory (e.g. for `SSE`: `PADDB`), then there are no stalls.
