// Copyright 2018 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cstdint>

// (Unsafely) use argc/argv as unknown-at-compile-time values so that the
// compiler doesn't elide these computations.
int main(int argc, char** argv) {
  uint32_t a = *reinterpret_cast<uint32_t*>(&argv[0]);
  uint32_t d = *reinterpret_cast<uint32_t*>(&argv[1]);
  uint32_t i = *reinterpret_cast<uint32_t*>(&argv[2]);
  uint32_t N = *reinterpret_cast<uint32_t*>(&argv[3]);

  asm volatile(R"(
    looplabel%=:
      #begin iaca seq
      .byte 0x0F, 0x0B
      movl $111, %%ebx
      .byte 0x64, 0x67, 0x90
      #end iaca seq
      addl %%ecx, %%eax
      movl %%ecx, %%r8d
      imull %%r8d, %%r8d
      addl %%r8d, %%edx
      addl $1, %%ecx
      cmp  %%ecx, %%edi
      #begin iaca seq
      movl $222, %%ebx
      .byte 0x64, 0x67, 0x90
      .byte 0x0F, 0x0B
      #end iaca seq
      jne looplabel%=

    )"
               : "+a"(a), "+d"(d), "+c"(i)
               : "D"(N)
               :);

  return static_cast<int>(a + d + i);
}
