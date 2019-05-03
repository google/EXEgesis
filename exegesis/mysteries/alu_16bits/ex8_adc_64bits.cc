// ADC64ri32 uses two p0156 as expected.

int main() {
  for (int i = 0; i < LOOP_ITERATIONS; ++i) {
    asm volatile(
        R"(
          .rept 1000
          adcq $0x7ffffffe, %%rax
          .endr
        )"
        :
        :
        : "%rax");
  }
  return 0;
}
