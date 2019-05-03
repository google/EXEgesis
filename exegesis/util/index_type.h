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

#ifndef EXEGESIS_UTIL_INDEX_TYPE_H_
#define EXEGESIS_UTIL_INDEX_TYPE_H_

#include <functional>
#include <ostream>
#include <type_traits>

namespace exegesis {

// Defines an index type with strong type checks based on an underlying integral
// type, to be used as a handle to other objects. The class is designed so that
// two different index types are incompatible, and any attempts to pass one as
// the other leads to a compilation error.
//
// The index types support comparison, increment/decrement, but they do not
// support arithmetic, as these operations are ill-defined for a handle.
//
// Typical usage:
//  DEFINE_INDEX_TYPE(RegisterIndex, int);
//  DEFINE_INDEX_TYPE(InstructionIndex, int);
//
//  RegisterIndex reg = GetRegisterIndex();
//  DoSomethingWithRegister(reg);
//  InstructionIndex inst = GetInstructionIndex();
//  reg = inst;  // Does not compile.
//  DoSomethingWithInstruction(reg);  // Does not compile.
#define DEFINE_INDEX_TYPE(Index, ValueType) \
  struct Index##_index_tag {};              \
  using Index = ::exegesis::IndexType<Index##_index_tag, ValueType>;

template <typename TagType, typename ValueT>
class IndexType {
 public:
  static_assert(std::is_integral<ValueT>::value,
                "ValueType must be an integral type.");

  // The type of the underlying value of the index.
  using ValueType = ValueT;

  // A hasher that allows using the index in std::unordered_map and
  // std::unordered_set.
  //
  // Example usage:
  //  DEFINE_INDEX_TYPE(RegisterIndex, int);
  //  using RegisterSet = std::unordered_set<RegisterIndex,
  //      RegisterIndex::Hasher>
  struct Hasher {
    size_t operator()(const IndexType& index) const {
      std::hash<ValueType> hasher;
      return hasher(index.value());
    }
  };

  constexpr IndexType() : value_(0) {}
  explicit constexpr IndexType(ValueType value) : value_(value) {}

  constexpr IndexType(const IndexType&) = default;
  constexpr IndexType(IndexType&&) = default;

  // The assignment operations on the index work the natural way: only another
  // index of the same type may be assigned to the variable.
  IndexType& operator=(const IndexType&) = default;
  IndexType& operator=(IndexType&&) = default;

  // Comparison between two indices.
  constexpr inline bool operator==(const IndexType& other) const {
    return value_ == other.value_;
  }
  constexpr inline bool operator!=(const IndexType& other) const {
    return value_ != other.value_;
  }
  constexpr inline bool operator<(const IndexType& other) const {
    return value_ < other.value_;
  }
  constexpr inline bool operator<=(const IndexType& other) const {
    return value_ <= other.value_;
  }
  constexpr inline bool operator>(const IndexType& other) const {
    return value_ > other.value_;
  }
  constexpr inline bool operator>=(const IndexType& other) const {
    return value_ >= other.value_;
  }

  // Convenience operators for comparison between an index and the base type.
  constexpr inline bool operator==(const ValueType& other) const {
    return value_ == other;
  }
  constexpr inline bool operator!=(const ValueType& other) const {
    return value_ != other;
  }
  constexpr inline bool operator<(const ValueType& other) const {
    return value_ < other;
  }
  constexpr inline bool operator<=(const ValueType& other) const {
    return value_ <= other;
  }
  constexpr inline bool operator>(const ValueType& other) const {
    return value_ > other;
  }
  constexpr inline bool operator>=(const ValueType& other) const {
    return value_ >= other;
  }

  // Increments and decrements to support for loops.
  inline IndexType& operator++() {
    ++value_;
    return *this;
  }
  inline IndexType operator++(int) {
    const IndexType old = *this;
    ++value_;
    return old;
  }

  inline IndexType& operator--() {
    --value_;
    return *this;
  }
  inline IndexType operator--(int) {
    const IndexType old = *this;
    --value_;
    return old;
  }

  // Returns the raw value of the index.
  constexpr ValueType value() const { return value_; }

 private:
  ValueType value_;
};

template <typename TagType, typename ValueType>
std::ostream& operator<<(std::ostream& os,
                         const IndexType<TagType, ValueType>& index) {
  return os << index.value();
}

}  // namespace exegesis

#endif  // EXEGESIS_UTIL_INDEX_TYPE_H_
