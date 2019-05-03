// ADD8ri uses one p0156 as expected.

int main() {
  for (int i = 0; i < LOOP_ITERATIONS; ++i) {
    asm volatile(
        R"(
          .rept 1000
          addb $0x7e, %%al
          .endr
        )"
        :
        :
        : "%al");
  }
  return 0;
}
