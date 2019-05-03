# Back-to-back imuls. The uops do not compete for ports after the first
# iteration, but they have data dependencies. The inverse throughput is ~6
# (twice the latency of one imul).
imul rax, rdx
imul rdx, rax
