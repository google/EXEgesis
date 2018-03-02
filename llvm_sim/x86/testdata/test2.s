# Note that this code does not really make sense in isolation, but was a minimal
# reproduction case for early prototypes.
shl r12, 2
lea r13, [r12 + r12 * 0x2]
pxor xmm4, xmm4
pxor xmm5, xmm5
pxor xmm6, xmm6
pxor xmm7, xmm7
pxor xmm8, xmm8
pxor xmm9, xmm9
pxor xmm10, xmm10
pxor xmm11, xmm11
