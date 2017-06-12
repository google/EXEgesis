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

#include "exegesis/itineraries/compute_itineraries.h"

#include "exegesis/base/cpu_model.h"
#include "exegesis/base/host_cpu.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/testing/test_util.h"
#include "exegesis/util/proto_util.h"
#include "exegesis/util/system.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/google/protobuf/text_format.h"

namespace exegesis {
namespace itineraries {
namespace {

TEST(ComputeItinerariesTest, ADC) {
  // Restrict instructions to the given range.
  const auto instruction_set =
      ParseProtoFromStringOrDie<InstructionSetProto>(R"(
        instructions {
          llvm_mnemonic: "ADC8i8"
          vendor_syntax {
            mnemonic: "ADC"
            operands {
              addressing_mode: DIRECT_ADDRESSING
              encoding: IMPLICIT_ENCODING
              value_size_bits: 8
              name: "AL"
              usage: USAGE_WRITE
            }
            operands {
              addressing_mode: NO_ADDRESSING
              encoding: IMMEDIATE_VALUE_ENCODING
              value_size_bits: 8
              name: "imm8"
              usage: USAGE_READ
            }
          }
          available_in_64_bit: true
          legacy_instruction: true
          raw_encoding_specification: "14 ib"
          x86_encoding_specification {
            opcode: 20
            legacy_prefixes {
            }
            immediate_value_bytes: 1
          }
        }
      )");
  // Always compute itineraries for the host CPU.
  const string& host_cpu_model_id = HostCpuInfoOrDie().cpu_model_id();
  InstructionSetItinerariesProto itineraries;
  const auto* const microarchitecture =
      MicroArchitecture::FromCpuModelId(host_cpu_model_id);
  CHECK(microarchitecture != nullptr)
      << "Unknown CPU model '" << host_cpu_model_id << "'";
  itineraries.set_microarchitecture_id(microarchitecture->proto().id());
  itineraries.add_itineraries();

  const Status status = ComputeItineraries(instruction_set, &itineraries);

  // Unfortunately, since computing itineraries is based on measurements, this
  // can sometimes fail.
  if (!status.ok()) {
    LOG(ERROR) << status;
  } else {
    EXPECT_EQ(1, itineraries.itineraries_size());

    // Chech that we've detected at least one micro op.
    EXPECT_GT(itineraries.itineraries(0).micro_ops_size(), 0);
    // This is a simple instruction.
    EXPECT_LT(itineraries.itineraries(0).micro_ops_size(), 3);
  }
}

}  // namespace
}  // namespace itineraries
}  // namespace exegesis
