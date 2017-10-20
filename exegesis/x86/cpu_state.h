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

// Utilities to represent cpu state.

#ifndef EXEGESIS_BASE_CPU_STATE_H_
#define EXEGESIS_BASE_CPU_STATE_H_

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>

#include "glog/logging.h"

namespace exegesis {

// An aligned buffer allocated on the heap.
template <int alignement, typename T>
class UniqueAlignedStorage {
 public:
  // This class implements only a trivial destructor that frees the allocated
  // memory, but does not call the destructor of T/ValueType. We need to make
  // sure that it is never used with a type where this might cause problems.
  static_assert(std::is_trivially_destructible<T>::value,
                "The type T is not trivially destructible.");
  using ValueType = typename std::remove_extent<T>::type;

  UniqueAlignedStorage() : value_(nullptr) {
    void* ptr = nullptr;
    CHECK(!posix_memalign(&ptr, alignement, sizeof(T)));
    value_ = new (ptr) T;
  }

  ~UniqueAlignedStorage() { free(value_); }

  ValueType* get() const { return value_; }

  static constexpr const size_t size = sizeof(T);

 private:
  ValueType* value_;
};

template <int alignement, typename T>
constexpr const size_t UniqueAlignedStorage<alignement, T>::size;

// Provides methods for debugging the FPU control word. See section 8.1.5 of the
// Intel SDM volume 1.
class FPUControlWord {
 public:
  explicit FPUControlWord(uint16_t raw_value) : raw_value_(raw_value) {}

  std::string DebugString() const;

  // Returns the number of bits of precision (24/53/64), or 0 if invalid.
  int GetPrecision() const;

  // Returns a string representing the rounding mode. For debug only.
  const char* GetRoundingMode() const;

  uint16_t raw_value_;
};

// Provides methods for debugging the FPU status word. See section 8.1.3 of the
// Intel SDM volume 1.
class FPUStatusWord {
 public:
  explicit FPUStatusWord(uint16_t raw_value) : raw_value_(raw_value) {}

  std::string DebugString() const;

  int GetStackTop() const;

  int GetConditionCode() const;

  uint16_t raw_value_;
};

// Provides methods for debugging the FPU tag word. See section 8.1.7 of the
// Intel SDM volume 1.
class FPUTagWord {
 public:
  explicit FPUTagWord(uint16_t raw_value) : raw_value_(raw_value) {}

  std::string DebugString() const;

  // Return a string representing the status of ST(i). For debug only.
  const char* GetStatus(int i) const;

  uint16_t raw_value_;
};

// Provides a structured view on a FPU/MMX/SSE state to be used with
// FXSAVE64/FXRSTORE64. See the documentation of FXSAVE in set Intel SDM. Note
// that this layout is valid only with FXSAVE64 (REX.W=1).
// For bit-precise documentation of each field, please refer to SDM volume 1.
class FXStateBuffer : public UniqueAlignedStorage<16, uint8_t[512]> {
 public:
  FXStateBuffer();

  // All the accessors use Intel SDM terminology.
  FPUControlWord GetFPUControlWord() const {
    return FPUControlWord(GetWordAt<0>());
  }

  FPUStatusWord GetFPUStatusWord() const {
    return FPUStatusWord(GetWordAt<2>());
  }

  // Returns the status of ST(0)-ST(7).
  FPUTagWord GetAbridgedFPUTagWord() const {
    return FPUTagWord(GetByteAt<4>());
  }

  // The three following methods respectively return the opcode, instruction
  // pointer and and operand addresses for the "last x87 non-control instruction
  // executed that incurred an unmasked x87 exception".
  uint16_t GetFPUOpCode() const { return GetWordAt<6>(); }
  uint64_t GetFPUInstructionPointerOffset() const { return GetQWordAt<8>(); }
  uint64_t GetFPUInstructionOperandDataPointerOffset() const {
    return GetQWordAt<16>();
  }

  uint32_t GetMXCSRRegisterState() const { return GetDWordAt<24>(); }

  uint32_t GetMXCSR_MASK() const { return GetDWordAt<28>(); }

  std::string DebugString() const;

 private:
  // Returns the byte at the given offset.
  template <int offset>
  uint8_t GetByteAt() const {
    return get()[offset];
  }

  // Returns the {q,d,}word with least significant byte at the given offset.
  template <int offset>
  uint16_t GetWordAt() const {
    return (static_cast<uint16_t>(GetByteAt<offset + 1>()) << 8) +
           GetByteAt<offset>();
  }
  template <int offset>
  uint32_t GetDWordAt() const {
    return (static_cast<uint32_t>(GetWordAt<offset + 2>()) << 16) +
           GetWordAt<offset>();
  }
  template <int offset>
  uint64_t GetQWordAt() const {
    return (static_cast<uint64_t>(GetDWordAt<offset + 4>()) << 32) +
           GetDWordAt<offset>();
  }
};

}  // namespace exegesis

#endif  // EXEGESIS_BASE_CPU_STATE_H_
