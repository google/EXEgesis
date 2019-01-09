// Same as ex1_addrm8_array, except we do not increment RSI. This was meant to
// find an example where address generation micro-operations are eliminated
// ("folded", as mentioned in the td file for Haswell in LLVM). They are not
// folded and the latency is higher than when incrementing RSI, which is
// explainable, as it is probably necessary to read the overwritten data back
// from cache. This does not explain why there are more micro-operations on all
// the ports, in particular on port 4.

int main() {
  unsigned char memory[1];
  for (int i = 0; i < LOOP_ITERATIONS; ++i) {
    asm volatile(
        R"(
          movq %[address], %%rsi
          .rept 1000
          addb $1, (%%rsi)
          .endr
        )"
        :
        : [ address ] "r"(memory)
        : "%rsi");
  }
  return 0;
}
