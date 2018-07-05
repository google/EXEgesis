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

#include "exegesis/base/microarchitecture.h"

#include "absl/strings/str_cat.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/util/proto_util.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

TEST(MicroArchitectureDataTest, Works) {
  auto architecture_proto = std::make_shared<const ArchitectureProto>(
      ParseProtoFromStringOrDie<ArchitectureProto>(R"proto(
        instruction_set { instructions { llvm_mnemonic: "F4KE" } }
        per_microarchitecture_itineraries {
          microarchitecture_id: 'blah'
          itineraries { llvm_mnemonic: "F4KE" }
        }
        per_microarchitecture_itineraries {
          microarchitecture_id: 'hsw'
          itineraries { llvm_mnemonic: "F4KE" }
        }
      )proto"));

  const auto statusor_microarchitecture =
      MicroArchitectureData::ForMicroArchitectureId(architecture_proto, "hsw");
  ASSERT_TRUE(statusor_microarchitecture.ok());
  const auto& microarchitecture = statusor_microarchitecture.ValueOrDie();
  EXPECT_EQ(microarchitecture.instruction_set().instructions_size(), 1);
  EXPECT_EQ(microarchitecture.itineraries().microarchitecture_id(), "hsw");
}

}  // namespace
}  // namespace exegesis
