// Same as ex1_addrm8_array except we are dealing with 32-bit memory. The
// distribution between the different execution units is the same as with 8-bit
// memory.

int main() {
  unsigned char memory[4 * 1000];
  for (int i = 0; i < LOOP_ITERATIONS; ++i) {
    asm volatile(
        R"(
          movq %[address], %%rsi
          .rept 1000
          addl $1, (%%rsi)
          addq $4, %%rsi
          .endr
        )"
        :
        : [ address ] "r"(memory)
        : "%rsi");
  }
  return 0;
}
