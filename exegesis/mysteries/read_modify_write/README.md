Investigating Read-Modify-Write operations.

Motivation: The LLVM CPU model assumes that for Read-Modify-Write operations
(e.g. ADDmi), address computation happens only once, i.e. the uops are using
{p23, p0156, p4} instead of {p23, p0156, p237, p4}. This looks like a reasonable
assumption but does not correspond to actual measurements.

While investigating this we found that there are many more p4's than expected
when writing on a single memory location. We still have no explanation for this.

