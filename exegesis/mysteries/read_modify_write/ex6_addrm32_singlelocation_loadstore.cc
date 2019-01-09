// Same as ex5_addrm8_singlelocation_loadstore, on 32-bit memory. Results are
// different from the ones obtained on 8-bit memory or the ones using a direct
// operation on a memory operand. In particular, 3 memory writes on port 4 are
// issued per iteration. NOTE(bdb): This does not seem to be alignment-related.

int main() {
  unsigned char memory[41];
  for (int i = 0; i < LOOP_ITERATIONS; ++i) {
    asm volatile(
        R"(
          movq %[address], %%rsi
          .rept 1000
          movl (%%rsi),%%ebx
          addl $1, %%ebx
          movl %%ebx,(%%rsi)
          .endr
        )"
        :
        : [ address ] "r"(memory)
        : "%rsi", "%ebx");
  }
  return 0;
}
