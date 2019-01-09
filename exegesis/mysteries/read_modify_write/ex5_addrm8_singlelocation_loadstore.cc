// In this example, the direct operation on a memory operand is replaced with a
// load, an operation on a register, and a store. Results are quite different
// from the ones obtained with the direct operation on a memory operand. For
// example only one memory write on port 4 is issued per iteration.

int main() {
  unsigned char memory[1];
  for (int i = 0; i < LOOP_ITERATIONS; ++i) {
    asm volatile(
        R"(
          movq %[address], %%rsi
          .rept 1000
          movb (%%rsi),%%bl
          incb %%bl
          movb %%bl,(%%rsi)
          .endr
        )"
        :
        : [ address ] "r"(memory)
        : "%rsi", "%bl");
  }
  return 0;
}
