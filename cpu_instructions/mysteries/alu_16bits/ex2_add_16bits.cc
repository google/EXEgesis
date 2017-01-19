// ADD16ri does *not* use one p0156 as expected. It seems that port 6 is always
// used instead of port 0.

int main() {
  for (int i = 0; i < LOOP_ITERATIONS; ++i) {
    asm volatile(
        R"(
          .rept 1000
          addw $0x7ffe, %%ax
          .endr
        )"
        :
        :
        : "%ax");
  }
  return 0;
}
