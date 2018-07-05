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

#include "exegesis/base/registers.h"

#include <string>
#include <vector>

#include "exegesis/testing/test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

using ::exegesis::testing::EqualsProto;

TEST(RegistersTest, MakeRegistersFromBaseNames) {
  constexpr char kExpectedRegisters[] = R"proto(
    register_groups {
      name: "RAX group"
      description: "The group of registers aliased with RAX"
      registers {
        name: "RAX"
        binary_encoding: 0
        position_in_group { lsb: 0 msb: 63 }
        register_class: GENERAL_PURPOSE_REGISTER_64_BIT
      }
      registers {
        name: "AL"
        binary_encoding: 0
        position_in_group { lsb: 0 msb: 7 }
        register_class: GENERAL_PURPOSE_REGISTER_8_BIT
      }
      registers {
        name: "AH"
        binary_encoding: 4
        position_in_group { lsb: 8 msb: 15 }
        register_class: GENERAL_PURPOSE_REGISTER_8_BIT
      }
    }
    register_groups {
      name: "RCX group"
      description: "The group of registers aliased with RCX"
      registers {
        name: "RCX"
        binary_encoding: 1
        position_in_group { lsb: 0 msb: 63 }
        register_class: GENERAL_PURPOSE_REGISTER_64_BIT
      }
      registers {
        name: "CL"
        binary_encoding: 1
        position_in_group { lsb: 0 msb: 7 }
        register_class: GENERAL_PURPOSE_REGISTER_8_BIT
      }
      registers {
        name: "CH"
        binary_encoding: 5
        position_in_group { lsb: 8 msb: 15 }
        register_class: GENERAL_PURPOSE_REGISTER_8_BIT
      }
    })proto";
  const std::vector<RegisterTemplate> kTemplates = {
      {"R", "X", 0, 63, 0, "", RegisterProto::GENERAL_PURPOSE_REGISTER_64_BIT},
      {"", "L", 0, 7, 0, "", RegisterProto::GENERAL_PURPOSE_REGISTER_8_BIT},
      {"", "H", 8, 15, 4, "", RegisterProto::GENERAL_PURPOSE_REGISTER_8_BIT}};
  const std::vector<std::string> kBaseNames = {"A", "C"};
  EXPECT_THAT(MakeRegistersFromBaseNames(kTemplates, kBaseNames, 0),
              EqualsProto(kExpectedRegisters));
}

TEST(RegistersTest, MakeRegistersFromBaseNameAndIndices) {
  constexpr const char kExpectedRegisters[] = R"proto(
    register_groups {
      name: "XMM4 group"
      description: "The group of registers aliased with XMM4"
      registers {
        name: "XMM4"
        binary_encoding: 4
        position_in_group { lsb: 0 msb: 127 }
        feature_name: "SSE"
        register_class: VECTOR_REGISTER_128_BIT
      }
      registers {
        name: "YMM4"
        binary_encoding: 4
        position_in_group { lsb: 0 msb: 255 }
        feature_name: "AVX"
        register_class: VECTOR_REGISTER_256_BIT
      }
      registers {
        name: "ZMM4"
        binary_encoding: 4
        position_in_group { lsb: 0 msb: 511 }
        feature_name: "AVX512"
        register_class: VECTOR_REGISTER_512_BIT
      }
    }
    register_groups {
      name: "XMM5 group"
      description: "The group of registers aliased with XMM5"
      registers {
        name: "XMM5"
        binary_encoding: 5
        position_in_group { lsb: 0 msb: 127 }
        feature_name: "SSE"
        register_class: VECTOR_REGISTER_128_BIT
      }
      registers {
        name: "YMM5"
        binary_encoding: 5
        position_in_group { lsb: 0 msb: 255 }
        feature_name: "AVX"
        register_class: VECTOR_REGISTER_256_BIT
      }
      registers {
        name: "ZMM5"
        binary_encoding: 5
        position_in_group { lsb: 0 msb: 511 }
        feature_name: "AVX512"
        register_class: VECTOR_REGISTER_512_BIT
      }
    })proto";
  const std::vector<RegisterTemplate> kTemplates = {
      {"X", "", 0, 127, 0, "SSE", RegisterProto::VECTOR_REGISTER_128_BIT},
      {"Y", "", 0, 255, 0, "AVX", RegisterProto::VECTOR_REGISTER_256_BIT},
      {"Z", "", 0, 511, 0, "AVX512", RegisterProto::VECTOR_REGISTER_512_BIT}};
  constexpr char kBaseName[] = "MM";
  constexpr int kBeginIndex = 4;
  constexpr int kEndIndex = 6;
  EXPECT_THAT(MakeRegistersFromBaseNameAndIndices(
                  kTemplates, kBaseName, kBeginIndex, kEndIndex, kBeginIndex),
              EqualsProto(kExpectedRegisters));
}

}  // namespace
}  // namespace exegesis
