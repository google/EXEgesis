// ADD32ri uses one p0156 as expected.

int main() {
  for (int i = 0; i < LOOP_ITERATIONS; ++i) {
    asm volatile(
        R"(
          .rept 1000
          addl $0x7ffffffe, %%eax
          .endr
        )"
        :
        :
        : "%eax");
  }
  return 0;
}
