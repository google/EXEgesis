// Same as ex3_addrm8_singlelocation, on 32-bit memory. The results are the same
// as with 8-bit memory.

int main() {
  unsigned char memory[4];
  for (int i = 0; i < LOOP_ITERATIONS; ++i) {
    asm volatile(
        R"(
          movq %[address], %%rsi
          .rept 1000
          addl $1, (%%rsi)
          .endr
        )"
        :
        : [ address ] "r"(memory)
        : "%rsi");
  }
  return 0;
}
