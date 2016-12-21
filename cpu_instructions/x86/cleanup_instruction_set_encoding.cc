// Copyright 2016 Google Inc.
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

#include "cpu_instructions/x86/cleanup_instruction_set_encoding.h"

#include <algorithm>
#include "strings/string.h"
#include <unordered_set>
#include <vector>

#include "glog/logging.h"
#include "src/google/protobuf/repeated_field.h"
#include "strings/str_cat.h"
#include "strings/util.h"
#include "util/gtl/map_util.h"
#include "cpu_instructions/base/cleanup_instruction_set.h"
#include "cpu_instructions/x86/cleanup_instruction_set_utils.h"
#include "re2/re2.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"
#include "util/task/status_macros.h"

namespace cpu_instructions {
namespace x86 {

using ::cpu_instructions::util::InvalidArgumentError;
using ::cpu_instructions::util::Status;

Status AddMissingMemoryOffsetEncoding(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  static const char kAddressSizeOverridePrefix[] = "67 ";
  static const char k32BitImmediateValueSuffix[] = " id";
  static const char k64BitImmediateValueSuffix[] = " io";
  const std::unordered_set<string> kEncodingSpecifications = {
      "A0", "REX.W + A0", "A1", "REX.W + A1",
      "A2", "REX.W + A2", "A3", "REX.W + A3"};
  std::vector<InstructionProto> new_instructions;
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    const string& binary_encoding = instruction.binary_encoding();
    if (ContainsKey(kEncodingSpecifications, binary_encoding)) {
      new_instructions.push_back(instruction);
      InstructionProto& new_instruction = new_instructions.back();
      new_instruction.set_binary_encoding(StrCat(kAddressSizeOverridePrefix,
                                                 binary_encoding,
                                                 k32BitImmediateValueSuffix));
      // NOTE(user): Changing the binary encoding of the original proto will
      // either invalidate or change the value of the variable binary_encoding.
      // We must thus be careful to not use this variable after it is changed by
      // instruction.set_binary_encoding.
      instruction.set_binary_encoding(
          StrCat(binary_encoding, k64BitImmediateValueSuffix));
    }
  }
  for (InstructionProto& new_instruction : new_instructions) {
    instruction_set->add_instructions()->Swap(&new_instruction);
  }
  return Status::OK;
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddMissingMemoryOffsetEncoding, 1000);

namespace {

// Adds the REX.W prefix to the binary encoding specification of the given
// instruction proto. If the instruction proto already has the prefix, it is not
// added and a warning is printed to the log.
void AddRexWPrefixToInstructionProto(InstructionProto* instruction) {
  CHECK(instruction != nullptr);
  static const char kRexWPrefix[] = "REX.W";
  const string& binary_encoding = instruction->binary_encoding();
  if (binary_encoding.find(kRexWPrefix) == string::npos) {
    instruction->set_binary_encoding(StrCat(kRexWPrefix, " ", binary_encoding));
  } else {
    LOG(WARNING) << "The instruction already has a REX.W prefix: "
                 << binary_encoding;
  }
}

}  // namespace

Status FixBinaryEncodingSpecificationOfPopFsAndGs(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  static const char kPopInstruction[] = "POP";
  static const char k16Bits[] = "16 bits";
  static const char k64Bits[] = "64 bits";
  const std::unordered_set<string> kFsAndGsOperands = {"FS", "GS"};

  // First find all occurences of the POP FS and GS instructions.
  std::vector<InstructionProto*> pop_instructions;
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    const InstructionFormat& vendor_syntax = instruction.vendor_syntax();
    if (vendor_syntax.operands_size() == 1 &&
        vendor_syntax.mnemonic() == kPopInstruction &&
        ContainsKey(kFsAndGsOperands, vendor_syntax.operands(0).name())) {
      pop_instructions.push_back(&instruction);
    }
  }

  // Make modifications to the 16-bit versions, and make a new copy of the
  // 64-bit versions. We can't add the new copies to instruction_set directly
  // from the loop, because that could invalidate the pointers that we collected
  // in pop_instructions.
  std::vector<InstructionProto> new_pop_instructions;
  for (InstructionProto* const instruction : pop_instructions) {
    // The only way to find out which version it is is from the description of
    // the instruction.
    const string& description = instruction->description();
    if (description.find(k16Bits) != string::npos) {
      AddOperandSizeOverrideToInstructionProto(instruction);
    } else if (description.find(k64Bits) != string::npos) {
      new_pop_instructions.push_back(*instruction);
      AddRexWPrefixToInstructionProto(&new_pop_instructions.back());
    }
  }
  for (InstructionProto& new_pop_instruction : new_pop_instructions) {
    new_pop_instruction.Swap(instruction_set->add_instructions());
  }

  return Status::OK;
}
REGISTER_INSTRUCTION_SET_TRANSFORM(FixBinaryEncodingSpecificationOfPopFsAndGs,
                                   1000);

Status FixBinaryEncodingSpecificationOfPushFsAndGs(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  static const char kPushInstruction[] = "PUSH";
  const std::unordered_set<string> kFsAndGsOperands = {"FS", "GS"};

  // Find the existing PUSH instructions for FS and GS, and create the remaining
  // versions of the instructions. Note that we can't add the new versions
  // directly to instruction_set, because that might invalidate the iterators
  // used in the for loop.
  std::vector<InstructionProto> new_push_instructions;
  for (const InstructionProto& instruction : instruction_set->instructions()) {
    const InstructionFormat& vendor_syntax = instruction.vendor_syntax();
    if (vendor_syntax.operands_size() == 1 &&
        vendor_syntax.mnemonic() == kPushInstruction &&
        ContainsKey(kFsAndGsOperands, vendor_syntax.operands(0).name())) {
      // There is only one version of each of the instruction. Keep this as the
      // base version (64-bit), and add a 16-bit version and a 64-bit version
      // with a REX.W prefix. Note that this way we miss the 32-bit version, but
      // since we focus on the 64-bit mode anyway, we would remove it at a later
      // stage anyway.
      new_push_instructions.push_back(instruction);
      AddOperandSizeOverrideToInstructionProto(&new_push_instructions.back());
      new_push_instructions.push_back(instruction);
      AddRexWPrefixToInstructionProto(&new_push_instructions.back());
    }
  }
  for (InstructionProto& new_push_instruction : new_push_instructions) {
    new_push_instruction.Swap(instruction_set->add_instructions());
  }
  return Status::OK;
}
REGISTER_INSTRUCTION_SET_TRANSFORM(FixBinaryEncodingSpecificationOfPushFsAndGs,
                                   1000);

Status FixAndCleanUpBinaryEncodingSpecificationsOfSetInstructions(
    InstructionSetProto* instruction_set) {
  static const char* const kBinaryEncodings[] = {
      "0F 90", "0F 91", "0F 92", "0F 93", "0F 93", "0F 94",
      "0F 95", "0F 96", "0F 97", "0F 98", "0F 99", "0F 9A",
      "0F 9B", "0F 9C", "0F 9D", "0F 9E", "0F 9F",
  };
  std::unordered_set<string> replaced_encodings;
  std::unordered_set<string> removed_encodings;
  for (const char* const binary_encoding : kBinaryEncodings) {
    replaced_encodings.insert(binary_encoding);
    removed_encodings.insert(StrCat("REX + ", binary_encoding));
  }
  google::protobuf::RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();

  // Remove the REX versions of the instruction, because the REX prefix doesn't
  // change anything (it is there only for the register index extension bits).
  instructions->erase(
      std::remove_if(instructions->begin(), instructions->end(),
                     [&removed_encodings](const InstructionProto& instruction) {
                       return ContainsKey(removed_encodings,
                                          instruction.binary_encoding());
                     }),
      instructions->end());

  // Fix the binary encoding of the non-REX versions.
  for (InstructionProto& instruction : *instructions) {
    const string& binary_encoding = instruction.binary_encoding();
    if (ContainsKey(replaced_encodings, binary_encoding)) {
      instruction.set_binary_encoding(StrCat(binary_encoding, " /0"));
    }
  }

  return Status::OK;
}
REGISTER_INSTRUCTION_SET_TRANSFORM(
    FixAndCleanUpBinaryEncodingSpecificationsOfSetInstructions, 1000);

Status FixBinaryEncodingSpecificationOfXBegin(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  static const char kXBeginBinaryEncoding[] = "C7 F8";
  const std::unordered_map<string, string> kOperandToBinaryEncoding = {
      {"rel16", "66 C7 F8 cw"}, {"rel32", "C7 F8 cd"}};
  Status status = Status::OK;
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    if (instruction.binary_encoding() == kXBeginBinaryEncoding) {
      const InstructionFormat& vendor_syntax = instruction.vendor_syntax();
      if (vendor_syntax.operands_size() != 1) {
        status = util::InvalidArgumentError(
            "Unexpected number of arguments of a XBEGIN instruction: ");
        LOG(ERROR) << status;
        continue;
      }
      if (!FindCopy(kOperandToBinaryEncoding, vendor_syntax.operands(0).name(),
                    instruction.mutable_binary_encoding())) {
        status = InvalidArgumentError(
            StrCat("Unexpected argument of a XBEGIN instruction: ",
                   vendor_syntax.operands(0).name()));
        LOG(ERROR) << status;
        continue;
      }
    }
  }
  return status;
}
REGISTER_INSTRUCTION_SET_TRANSFORM(FixBinaryEncodingSpecificationOfXBegin,
                                   1000);

Status FixBinaryEncodingSpecifications(InstructionSetProto* instruction_set) {
  const RE2 fix_w0_regexp("^(VEX[^ ]*\\.)0 ");
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    string binary_encoding = instruction.binary_encoding();

    GlobalReplaceSubstring("0f", "0F", &binary_encoding);
    GlobalReplaceSubstring("imm8", "ib", &binary_encoding);
    RE2::Replace(&binary_encoding, fix_w0_regexp, "\\1W0 ");

    instruction.set_binary_encoding(binary_encoding);
  }
  return Status::OK;
}
REGISTER_INSTRUCTION_SET_TRANSFORM(FixBinaryEncodingSpecifications, 1000);

Status AddMissingModRmAndImmediateSpecification(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  static const char kFullModRmSuffix[] = "/r";
  const std::unordered_set<string> kMissingModRMInstructionMnemonics = {
      "CVTDQ2PD", "VMOVD"};
  static const char kImmediateByteSuffix[] = "ib";
  const std::unordered_set<string> kMissingImmediateInstructionMnemonics = {
      "KSHIFTLB", "KSHIFTLW", "KSHIFTLD",  "KSHIFTLQ",    "KSHIFTRB",
      "KSHIFTRW", "KSHIFTRD", "KSHIFTRQ",  "VFIXUPIMMPS", "VFPCLASSSS",
      "VRANGESD", "VRANGESS", "VREDUCESD",
  };
  static const char kVSibSuffix[] = "/vsib";
  const std::unordered_set<string> kMissingVSibInstructionMnemonics = {
      "VGATHERDPD", "VGATHERQPD", "VGATHERDPS", "VGATHERQPS",
      "VPGATHERDD", "VPGATHERDQ", "VPGATHERQD", "VPGATHERQQ",
  };

  // Fixes instruction encodings for instructions matching the given mnemonics
  // by adding the given suffix if need be.
  const auto maybe_fix = [](const std::unordered_set<string>& mnemonics,
                            const StringPiece suffix,
                            InstructionProto* instruction) {
    const string& mnemonic = instruction->vendor_syntax().mnemonic();
    Status status = Status::OK;
    if (ContainsKey(mnemonics, mnemonic)) {
      if (instruction->binary_encoding().empty()) {
        status = InvalidArgumentError(StrCat(
            "The instruction does not have binary encoding specification: ",
            mnemonic));
      }

      const StringPiece binary_encoding_spec = instruction->binary_encoding();
      if (!binary_encoding_spec.ends_with(suffix)) {
        instruction->set_binary_encoding(
            StrCat(instruction->binary_encoding(), " ", suffix));
      }
    }
    return status;
  };

  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    RETURN_IF_ERROR(maybe_fix(kMissingModRMInstructionMnemonics,
                              kFullModRmSuffix, &instruction));
    RETURN_IF_ERROR(maybe_fix(kMissingImmediateInstructionMnemonics,
                              kImmediateByteSuffix, &instruction));
    RETURN_IF_ERROR(
        maybe_fix(kMissingVSibInstructionMnemonics, kVSibSuffix, &instruction));
  }
  return Status::OK;
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddMissingModRmAndImmediateSpecification,
                                   1000);

}  // namespace x86
}  // namespace cpu_instructions
