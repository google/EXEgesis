Investigating differences between 16 and 8/32/64 bit ALU operations on Haswell.

16-bit ALU instructions have a different port usage pattern than 8/32/64-bit ALU
instructions: They typically uses p6 instead of p0.
