// Test incrementing the 8-bit memory byte pointed to by RSI. Increment RSI.
// Note that the sum of the average use of ports 2, 3 and 7 is equal to two,
// which tends to prove that there are two address generation instructions.

int main() {
  unsigned char memory[1000];
  for (int i = 0; i < LOOP_ITERATIONS; ++i) {
    asm volatile(
        R"(
          movq %[address], %%rsi
          .rept 1000
          addb $1, (%%rsi)
          inc %%rsi
          .endr
        )"
        :
        : [address] "r"(memory)
        : "%rsi");
  }
  return 0;
}
