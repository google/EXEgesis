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

// A benchmark for https://reviews.llvm.org/D33987.

// A note on the benchmarking methodology:
// Benchmarking this correctly is quite hard because the chain of comparisons
// in a loop will be perfectly predicted by the branch predictor. So we generate
// a bunch of tuples with a uniform distribution of cases and permute then
// randomly. Going through the list linearly does not occur any cache misses.

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <tuple>

#include "benchmark/benchmark.h"
#include "glog/logging.h"

// TODO(courbet): Right now we need the trampoline for the pass to detect the
// pattern. This is because of the loop. Handle loops.
template <typename T>
__attribute__((noinline)) bool Trampoline(const T& a, const T& b) {
  return a == b;
}

template <uint64_t elem, typename... Ts>
typename std::enable_if<(elem >= sizeof...(Ts))>::type FillRandomTuple(
    std::tuple<Ts...>* tuple) {}

template <uint64_t elem, typename... Ts>
typename std::enable_if<(elem < sizeof...(Ts))>::type FillRandomTuple(
    std::tuple<Ts...>* tuple) {
  using T = typename std::tuple_element<elem, std::tuple<Ts...>>::type;
  static_assert(std::is_unsigned<T>::value,
                "please use unsigned integers or update this function");
  // Generate a "random" value of the right size.
  constexpr const uint64_t kLargePrime = 909090909090909091;
  std::get<elem>(*tuple) = static_cast<T>((elem + 1) * kLargePrime);
  FillRandomTuple<elem + 1>(tuple);
}

template <uint64_t elem, typename... Ts>
typename std::enable_if<(elem >= sizeof...(Ts))>::type AddOneToElem(
    std::tuple<Ts...>* tuple) {}

template <uint64_t elem, typename... Ts>
typename std::enable_if<(elem < sizeof...(Ts))>::type AddOneToElem(
    std::tuple<Ts...>* tuple) {
  std::get<elem>(*tuple) += 1;
}

template <int elem, typename... Ts>
typename std::enable_if<(elem < 0)>::type FillTupleCases(
    const std::tuple<Ts...>& lhs, std::tuple<Ts...> rhs,
    std::pair<std::tuple<Ts...>, std::tuple<Ts...>>* out) {}

template <int elem, typename... Ts>
typename std::enable_if<(elem >= 0)>::type FillTupleCases(
    const std::tuple<Ts...>& lhs, std::tuple<Ts...> rhs,
    std::pair<std::tuple<Ts...>, std::tuple<Ts...>>* out) {
  AddOneToElem<elem>(&rhs);
  // Sanity check.
  if (elem == sizeof...(Ts)) {
    CHECK(Trampoline(lhs, rhs));
  } else {
    CHECK(!Trampoline(lhs, rhs));
  }
  *out = std::make_pair(lhs, rhs);
  FillTupleCases<elem - 1>(lhs, rhs, out + 1);
}

template <typename... Ts>
void BenchStdTuple(benchmark::State& state) {
  using TupleOfTs = std::tuple<Ts...>;
  using TuplePair = std::pair<TupleOfTs, TupleOfTs>;
  constexpr const int num_elems = sizeof...(Ts);
  // Equal case, then one unequal case for each element position.
  constexpr const int num_cases = num_elems + 1;

  constexpr const int kNumTuplesPerCase = 1024;
  constexpr const int kNumTuplesTotal = num_cases * kNumTuplesPerCase;
  std::array<TuplePair, kNumTuplesTotal> tuples;
  for (int i = 0; i < kNumTuplesPerCase; ++i) {
    TupleOfTs t;
    FillRandomTuple<0>(&t);
    FillTupleCases<num_elems>(t, t, tuples.data() + i * num_cases);
  }

  std::random_shuffle(tuples.begin(), tuples.end());

  // Bench.
  while (state.KeepRunning()) {
    for (const auto& tuple_pair : tuples) {
      bool res = Trampoline(tuple_pair.first, tuple_pair.second);
      benchmark::DoNotOptimize(res);
    }
  }

  state.SetBytesProcessed(kNumTuplesTotal * sizeof(TupleOfTs) *
                          state.iterations());
}

BENCHMARK_TEMPLATE(BenchStdTuple, uint8_t, uint8_t);
BENCHMARK_TEMPLATE(BenchStdTuple, uint8_t, uint8_t, uint8_t);
BENCHMARK_TEMPLATE(BenchStdTuple, uint8_t, uint8_t, uint8_t, uint8_t);

BENCHMARK_TEMPLATE(BenchStdTuple, uint16_t, uint16_t);
BENCHMARK_TEMPLATE(BenchStdTuple, uint16_t, uint16_t, uint16_t);
BENCHMARK_TEMPLATE(BenchStdTuple, uint16_t, uint16_t, uint16_t, uint16_t);

BENCHMARK_TEMPLATE(BenchStdTuple, uint32_t, uint32_t);
BENCHMARK_TEMPLATE(BenchStdTuple, uint32_t, uint32_t, uint32_t);
BENCHMARK_TEMPLATE(BenchStdTuple, uint32_t, uint32_t, uint32_t, uint32_t);

BENCHMARK_TEMPLATE(BenchStdTuple, uint64_t, uint64_t);
BENCHMARK_TEMPLATE(BenchStdTuple, uint64_t, uint64_t, uint64_t);
BENCHMARK_TEMPLATE(BenchStdTuple, uint64_t, uint64_t, uint64_t, uint64_t);

BENCHMARK_TEMPLATE(BenchStdTuple,                           //  16 x uint64_t
                   uint64_t, uint64_t, uint64_t, uint64_t,  //
                   uint64_t, uint64_t, uint64_t, uint64_t,  //
                   uint64_t, uint64_t, uint64_t, uint64_t,  //
                   uint64_t, uint64_t, uint64_t, uint64_t);

BENCHMARK_TEMPLATE(BenchStdTuple, uint16_t, uint16_t, uint32_t);
