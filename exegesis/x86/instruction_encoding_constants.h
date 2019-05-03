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

#ifndef THIRD_PARTY_EXEGESIS_EXEGESIS_X86_INSTRUCTION_ENCODING_CONSTANTS_H_
#define THIRD_PARTY_EXEGESIS_EXEGESIS_X86_INSTRUCTION_ENCODING_CONSTANTS_H_

#include <cstdint>

namespace exegesis {
namespace x86 {

// Values of the lock/rep prefix bytes.
enum : uint8_t {
  kLockPrefixByte = 0xf0,
  kRepPrefixByte = 0xf3,
  kRepNePrefixByte = 0xf2,
};

// Values of the operand and address override prefix bytes.
enum : uint8_t {
  kOperandSizeOverrideByte = 0x66,
  kAddressSizeOverrideByte = 0x67,
};

// Values of the segment override prefix bytes.
enum : uint8_t {
  kCsOverrideByte = 0x2e,
  kSsOverrideByte = 0x36,
  kDsOverrideByte = 0x3e,
  kEsOverrideByte = 0x26,
  kFsOverrideByte = 0x64,
  kGsOverrideByte = 0x65,
};

// The base of the REX prefix byte. All rex prefixes have the upper four bits
// set according to this constant; the lower four bits encode the four REX bits.
enum : uint8_t { kRexPrefixBaseByte = 0x40 };

// The VEX prefix escape bytes.
enum : uint8_t {
  kTwoByteVexPrefixEscapeByte = 0xc5,
  kThreeByteVexPrefixEscapeByte = 0xc4,
  kThreeByteXopPrefixEscapeByte = 0x8f,
};

// The EVEX prefix escape byte.
enum : uint8_t { kEvexPrefixEscapeByte = 0x62 };

// The possible values of the vector length bits of the VEX/EVEX prefixes
// (vex.ll and evex.ll).
enum : uint8_t {
  kEvexPrefixVectorLength128BitsOrZero = 0,
  kEvexPrefixVectorLength256BitsOrOne = 1,
  kEvexPrefixVectorLength512Bits = 2,
};

}  // namespace x86
}  // namespace exegesis

#endif  // THIRD_PARTY_EXEGESIS_EXEGESIS_X86_INSTRUCTION_ENCODING_CONSTANTS_H_
