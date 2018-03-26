# This is a gemm kernel.
outerLoop2:
pmovzxbw xmm1, [r9] # [rhs_ptr]

pmovzxbw xmm0, [r8 + 0x00] # [lhs_ptr + 0x00]
pshufd xmm2, xmm1, 0x00
pmaddwd xmm2, xmm0
paddd xmm4, xmm2
pshufd xmm3, xmm1, 0x55
pmaddwd xmm3, xmm0
paddd xmm5, xmm3

prefetcht0 [r8 + 0x80] # [lhs_ptr + 0x80]

pshufd xmm2, xmm1, 0xaa
pmaddwd xmm2, xmm0
paddd xmm6, xmm2
pshufd xmm3, xmm1, 0xff
pmaddwd xmm3, xmm0
paddd xmm7, xmm3

pmovzxbw xmm0, [r8 + 0x08] # [lhs_ptr + 0x08]
pshufd xmm2, xmm1, 0x00
pmaddwd xmm2, xmm0
paddd xmm8, xmm2
pshufd xmm3, xmm1, 0x55
pmaddwd xmm3, xmm0
paddd xmm9, xmm3

prefetcht0 [r9 + 0x80] # [rhs_ptr + 0x80]

pshufd xmm2, xmm1, 0xaa
pmaddwd xmm2, xmm0
paddd xmm10, xmm2
pshufd xmm3, xmm1, 0xff
pmaddwd xmm3, xmm0
paddd xmm11, xmm3

pmovzxbw xmm0, [r8 + 0x10] # [lhs_ptr + 0x10]
pshufd xmm2, xmm1, 0x00
pmaddwd xmm2, xmm0
paddd xmm12, xmm2
pshufd xmm3, xmm1, 0x55
pmaddwd xmm3, xmm0
paddd xmm13, xmm3

pshufd xmm2, xmm1, 0xaa
pmaddwd xmm2, xmm0
paddd xmm14, xmm2
pshufd xmm3, xmm1, 0xff
pmaddwd xmm3, xmm0
paddd xmm15, xmm3

pmovzxbw xmm1, [r9 + 0x08] # [rhs_ptr + 0x08]

pmovzxbw xmm0, [r8 + 0x18] # [lhs_ptr + 0x18]
pshufd xmm2, xmm1, 0x00
pmaddwd xmm2, xmm0
paddd xmm4, xmm2
pshufd xmm3, xmm1, 0x55
pmaddwd xmm3, xmm0
paddd xmm5, xmm3

pshufd xmm2, xmm1, 0xaa
pmaddwd xmm2, xmm0
paddd xmm6, xmm2
pshufd xmm3, xmm1, 0xff
pmaddwd xmm3, xmm0
paddd xmm7, xmm3

pmovzxbw xmm0, [r8 + 0x20] # [lhs_ptr + 0x20]
pshufd xmm2, xmm1, 0x00
pmaddwd xmm2, xmm0
paddd xmm8, xmm2
pshufd xmm3, xmm1, 0x55
pmaddwd xmm3, xmm0
paddd xmm9, xmm3

pshufd xmm2, xmm1, 0xaa
pmaddwd xmm2, xmm0
paddd xmm10, xmm2
pshufd xmm3, xmm1, 0xff
pmaddwd xmm3, xmm0
paddd xmm11, xmm3

pmovzxbw xmm0, [r8 + 0x28] # [lhs_ptr + 0x28]
pshufd xmm2, xmm1, 0x00
pmaddwd xmm2, xmm0
paddd xmm12, xmm2
pshufd xmm3, xmm1, 0x55
pmaddwd xmm3, xmm0
paddd xmm13, xmm3

pshufd xmm2, xmm1, 0xaa
pmaddwd xmm2, xmm0
paddd xmm14, xmm2
pshufd xmm3, xmm1, 0xff
pmaddwd xmm3, xmm0
paddd xmm15, xmm3

add r8, 0x30
add r9, 0x10

sub r15, 2
jnz outerLoop2
