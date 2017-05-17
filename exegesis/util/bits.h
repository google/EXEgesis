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

// Helper functions for reading bits of unsigned integers.

#ifndef EXEGESIS_UTIL_BITS_H_
#define EXEGESIS_UTIL_BITS_H_

#include <cstdint>

#include "glog/logging.h"

namespace exegesis {

// Returns true if the bit at 'bit_position' of 'value' is set to one.
// Otherwise, returns false. The position of the bit is zero-based.
inline bool IsNthBitSet(uint32_t value, int bit_position) {
  DCHECK_LE(0, bit_position);
  DCHECK_LT(bit_position, 32);
  return value & (uint32_t{1} << bit_position);
}

// Clears the bits between the specified bit positions in 'value';
// 'start_bit_position' is the zero-based position of the first bit included in
// the range, and 'end_bit_position' is the zero-based position of the first bit
// not included in the range.
// The function returns the value with the bits cleared.
inline uint32_t ClearBitRange(uint32_t value, int start_bit_position,
                              int end_bit_position) {
  DCHECK_LE(0, start_bit_position);
  DCHECK_LT(start_bit_position, end_bit_position);
  DCHECK_LE(end_bit_position, 32);
  constexpr uint32_t kAllBits32 = 0xffffffffUL;
  const int num_mask_bits = 32 - end_bit_position + start_bit_position;
  const uint32_t inverse_mask = (kAllBits32 >> num_mask_bits)
                                << start_bit_position;
  return value & (~inverse_mask);
}

// Extracts the integer stored between the specified bits in 'value';
// 'start_bit_position' is the zero-based position of the first bit included in
// the range, and 'end_bit_position' is the zero-based position of the first bit
// not included in the range.
inline uint32_t GetBitRange(uint32_t value, int start_bit_position,
                            int end_bit_position) {
  DCHECK_LE(0, start_bit_position);
  DCHECK_LT(start_bit_position, end_bit_position);
  DCHECK_LE(end_bit_position, 32);
  constexpr uint32_t kAllBits32 = 0xffffffff;
  const uint32_t mask = kAllBits32 >> (32 - end_bit_position);
  return (value & mask) >> start_bit_position;
}

}  // namespace exegesis

#endif  // EXEGESIS_UTIL_BITS_H_
