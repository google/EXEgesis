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

#ifndef EXEGESIS_BASE_OPCODE_H_
#define EXEGESIS_BASE_OPCODE_H_

#include <cstdint>
#include <ostream>
#include <string>
#include <unordered_set>

#include "absl/container/node_hash_set.h"

namespace exegesis {

// Represents an opcode of an instruction set architecture. Based on the
// architectures we've encountered so far (x86, x86-64, ARM, Power), we're
// assuming that the opcode has at most 32 bits and that the first byte of the
// opcode is either non-zero, or the opcode has only one byte.
//
// This class is a thin wrapper over uint32_t that provides strong types and
// removes operations such as integer arithmetics that are not well defined for
// the opcodes.
class Opcode {
 public:
  // A hasher that allows using the Opcode in hash-tables (such as,
  // std::unordered_map, std::unordered_set, or absl::flat_hash_map).
  //
  // Example usage:
  // using OpcodeSet = std::unordered_set<Opcode, Opcode::Hasher>;
  struct Hasher {
    constexpr size_t operator()(const Opcode& opcode) const {
      return static_cast<size_t>(opcode.value());
    }
  };

  // An opcode with a value that does not correspond to any existing opcode in
  // the architectures supported by EXEgesis.
  static constexpr Opcode InvalidOpcode() {
    return Opcode(kInvalidOpcodeValue);
  }

  constexpr Opcode() : value_(kInvalidOpcodeValue) {}
  explicit constexpr Opcode(uint32_t value) : value_(value) {}
  constexpr Opcode(const Opcode& opcode) = default;
  constexpr Opcode(Opcode&& opcode) = default;

  // Defines the "natural" assignment and comparison operators to allow using
  // the Opcode class in maps, ...
  // There is no arithmetic, because using arithmetic operations on the opcode
  // is not a well defined operation.
  Opcode& operator=(const Opcode& opcode) = default;
  Opcode& operator=(Opcode&& opcode) = default;
  constexpr inline bool operator==(const Opcode& opcode) const {
    return value_ == opcode.value_;
  }
  constexpr inline bool operator!=(const Opcode& opcode) const {
    return value_ != opcode.value_;
  }
  constexpr inline bool operator<(const Opcode& opcode) const {
    return value_ < opcode.value_;
  }
  constexpr inline bool operator<=(const Opcode& opcode) const {
    return value_ <= opcode.value_;
  }
  constexpr inline bool operator>(const Opcode& opcode) const {
    return value_ > opcode.value_;
  }
  constexpr inline bool operator>=(const Opcode& opcode) const {
    return value_ >= opcode.value_;
  }

  // Returns the numerical value of the opcode.
  constexpr uint32_t value() const { return value_; }

  // Formats the opcode in the way used in the Intel manuals (uppercase
  // hexadecimal numbers, bytes are separated by spaces).
  std::string ToString() const;

 private:
  // An opcode value that is not used by any existing instruction. It is used to
  // initialize the Opcode objects using the default constructor.
  static constexpr uint32_t kInvalidOpcodeValue = 0xffffffff;

  uint32_t value_;
};

using OpcodeSet = absl::node_hash_set<Opcode, Opcode::Hasher>;

std::ostream& operator<<(std::ostream& os, const exegesis::Opcode& opcode);

}  // namespace exegesis

#endif  // EXEGESIS_BASE_OPCODE_H_
