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

#ifndef EXEGESIS_UTIL_STRUCTURED_REGISTER_H_
#define EXEGESIS_UTIL_STRUCTURED_REGISTER_H_

#include <climits>
#include <type_traits>

namespace exegesis {

// Represents a register with access to individual bit ranges. This class can
// be used for building wrappers around registers where individual bits or bit
// ranges have a meaning.
template <typename RawValueType>
class StructuredRegister {
 public:
  static_assert(std::is_integral<RawValueType>::value &&
                    std::is_unsigned<RawValueType>::value,
                "The raw value type must be an unsigned integral type.");
  static constexpr int kNumBits = CHAR_BIT * sizeof(RawValueType);

  StructuredRegister() : raw_value_(0) {}
  StructuredRegister(RawValueType raw_value) : raw_value_(raw_value) {}

  RawValueType* mutable_raw_value() { return &raw_value_; }
  RawValueType raw_value() const { return raw_value_; }

  // Returns the bit range [msb, lsb] as an integer.
  template <int msb, int lsb>
  RawValueType ValueAt() const {
    static_assert(msb >= 0, "msb must be >= 0");
    static_assert(msb < kNumBits, "msb must be < kNumBits");
    static_assert(lsb >= 0, "lsb must be >= 0");
    static_assert(lsb < kNumBits, "lsb must be < kNumBits");
    static_assert(lsb <= msb, "lsb must be <= msb");
    return (raw_value_ >> lsb) &
           static_cast<RawValueType>((1ull << (msb - lsb + 1)) - 1ull);
  }

 private:
  RawValueType raw_value_ = 0;
};

}  // namespace exegesis

#endif  // EXEGESIS_UTIL_STRUCTURED_REGISTER_H_
