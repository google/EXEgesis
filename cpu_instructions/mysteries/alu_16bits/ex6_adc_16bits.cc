// ADC16ri does *not* use two p0156 as expected. It seems that port 6 is always
// used instead of port 0 for the second one.

int main() {
  for (int i = 0; i < LOOP_ITERATIONS; ++i) {
    asm volatile(
        R"(
          .rept 1000
          adcw $0x7ffe, %%ax
          .endr
        )"
        :
        :
        : "%ax");
  }
  return 0;
}
