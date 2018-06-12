// Copyright 2017 Google Inc.
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

template <class T>
void DoNotOptimize(const T& var) {
  asm volatile("" : "+m"(const_cast<T&>(var)));
}

int main() {
  for (int i = 0; i < LOOP_ITERATIONS; ++i) {
    int ecx = 0;
    asm volatile(
        R"(
          .rept 1000
          incl %%ecx
          incl %%ecx
          decl %%ecx
          decl %%ecx
          .endr
        )"
        : "+c"(ecx)
        :
        :);
    DoNotOptimize(ecx);
  }
  return 0;
}
