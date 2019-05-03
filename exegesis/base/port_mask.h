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

#ifndef EXEGESIS_BASE_PORT_MASK_H_
#define EXEGESIS_BASE_PORT_MASK_H_

#include <ctype.h>
#include <sys/types.h>

#include <cstdint>
#include <string>

#include "exegesis/proto/microarchitecture.pb.h"
#include "glog/logging.h"

namespace exegesis {

// An encapsulation of uint64 use as a bit set to represent the possible
// execution ports for a micro-operation.
// It can be populated from a MicroOperation message or from a string containing
// the port numbers on which the micro-operation can be executed. The string
// may contain characters 'p' or 'P'. They will be ignored. For example,
// "P01p5" means that the micro-operation may be executed on ports 0,1 or 5.
class PortMask {
 public:
  class Hash;

  PortMask() : mask_(0) {}

  explicit constexpr PortMask(uint64_t mask) : mask_(mask) {}

  explicit PortMask(const std::string& string_mask) {
    mask_ = 0;
    for (int c : string_mask) {
      if (toupper(c) == 'P') continue;
      CHECK(isdigit(c));
      AddPossiblePort(c - '0');
    }
  }

  explicit PortMask(const std::vector<int>& v) {
    mask_ = 0;
    for (int p : v) {
      AddPossiblePort(p);
    }
  }

  // Returns the number of possible ports.
  int num_possible_ports() const { return std::distance(begin(), end()); }

  // Builds a PortMask its proto representation.
  explicit PortMask(const PortMaskProto& proto);

  uint64_t mask() const { return mask_; }
  bool operator==(const PortMask& p) const { return mask() == p.mask(); }
  bool operator!=(const PortMask& p) const { return mask() != p.mask(); }

  // Handy method to add a possible port.
  void AddPossiblePort(int port_num) { mask_ |= (uint64_t{1} << port_num); }

  // Returns a string representing the port mask. For example the port mask
  // for an instruction executable on ports 0,1,5,6 would return "P0156".
  std::string ToString() const;

  // Returns a MicroOp proto corresponding to the port mask
  PortMaskProto ToProto() const;

  // Iterator support.
  class const_iterator {
   public:
    typedef int value_type;
    typedef int& reference;
    typedef int* pointer;
    typedef int difference_type;
    typedef std::forward_iterator_tag iterator_category;
    typedef const_iterator self_type;

    explicit const_iterator(uint64_t mask) : mask_(mask), port_(-1) { next(); }
    const_iterator() : port_(64) {}

    int operator*() const { return port_; }

    self_type operator++() {
      self_type previous = *this;
      next();
      return previous;
    }
    self_type operator++(int) {
      next();
      return *this;
    }
    const reference operator*() { return port_; }
    const pointer operator->() { return &port_; }
    bool operator==(const self_type& rhs) const { return port_ == rhs.port_; }
    bool operator!=(const self_type& rhs) const { return !(*this == rhs); }

   private:
    void next() {
      if (port_ == 64) {
        return;
      }
      ++port_;
      while (port_ < 64 && (mask_ & (1ull << port_)) == 0) {
        ++port_;
      }
    }

    uint64_t mask_;
    int port_;
  };
  const_iterator begin() const { return const_iterator(mask()); }
  const_iterator end() const { return const_iterator(); }

 private:
  uint64_t mask_;
};

class PortMask::Hash {
 public:
  size_t operator()(const PortMask& port_mask) const {
    return std::hash<uint64_t>()(port_mask.mask());
  }
};

// Output stream operator.
inline std::ostream& operator<<(std::ostream& os, const PortMask& p) {
  return os << p.ToString();
}

}  // namespace exegesis
#endif  // EXEGESIS_BASE_PORT_MASK_H_
