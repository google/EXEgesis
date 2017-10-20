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

#include "exegesis/x86/cleanup_instruction_set_removals.h"

#include <algorithm>
#include <string>
#include <unordered_set>

#include "exegesis/base/cleanup_instruction_set.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/util/status_util.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "src/google/protobuf/repeated_field.h"
#include "src/google/protobuf/util/message_differencer.h"
#include "strings/str_cat.h"
#include "strings/string_view.h"
#include "strings/string_view_utils.h"
#include "util/gtl/container_algorithm.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {

// RE2 and protobuf have two slightly different implementations of StringPiece.
// In this file, we use RE2::Consume heavily, so we explicitly switch to the RE2
// implementation.
using ::re2::StringPiece;

using ::exegesis::gtl::c_linear_search;
using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;
using ::google::protobuf::RepeatedPtrField;
using ::google::protobuf::util::MessageDifferencer;

Status RemoveDuplicateInstructions(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  std::unordered_set<std::string> visited_instructions;

  // A function that keeps track of instruction it has already encountered.
  // Returns true if an instruction was already seen, false otherwise.
  auto remove_instruction_if_visited =
      [&visited_instructions](const InstructionProto& instruction) {
        const std::string serialized_instruction =
            instruction.SerializeAsString();
        return !visited_instructions.insert(serialized_instruction).second;
      };

  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  instructions->erase(std::remove_if(instructions->begin(), instructions->end(),
                                     remove_instruction_if_visited),
                      instructions->end());
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveDuplicateInstructions, 4000);

Status RemoveInstructionsWaitingForFpuSync(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  // NOTE(ondrasej): The space after the opcode is important, because with it,
  // the prefix does not match the stand-alone FWAIT instructions that is
  // encoded as "9B".
  static constexpr char kFWaitPrefix[] = "9B ";
  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  const auto uses_fwait_for_sync = [](const InstructionProto& instruction) {
    return strings::StartsWith(instruction.raw_encoding_specification(),
                               kFWaitPrefix);
  };
  instructions->erase(std::remove_if(instructions->begin(), instructions->end(),
                                     uses_fwait_for_sync),
                      instructions->end());
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveInstructionsWaitingForFpuSync, 0);

Status RemoveNonEncodableInstructions(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  instructions->erase(
      std::remove_if(instructions->begin(), instructions->end(),
                     [](const InstructionProto& instruction) {
                       return !instruction.available_in_64_bit();
                     }),
      instructions->end());
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveNonEncodableInstructions, 0);

Status RemoveRepAndRepneInstructions(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  // NOTE(ondrasej): We're comparing the REP prefix without the space after it.
  // This will match also the REPE and REPNE prefixes. On the other hand, there
  // are no instructions that would use REP in their mnemonic, so optimizing
  // the matching this way is safe.
  static constexpr char kRepPrefix[] = "REP";
  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  const auto uses_rep_or_repne = [](const InstructionProto& instruction) {
    return strings::StartsWith(instruction.vendor_syntax().mnemonic(),
                               kRepPrefix);
  };
  instructions->erase(std::remove_if(instructions->begin(), instructions->end(),
                                     uses_rep_or_repne),
                      instructions->end());
  return OkStatus();
}
// TODO(ondrasej): In addition to removing them, we should also add an attribute
// saying whether the REP/REPE/REPNE prefix is allowed.
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveRepAndRepneInstructions, 0);

const std::unordered_set<std::string>* const kRemovedEncodingSpecifications =
    new std::unordered_set<std::string>(
        {// Specializations of the ENTER instruction that create stack frame
         // pointer. There is a more generic encoding scheme C8 iw ib that
         // already covers both of these cases.
         "C8 iw 00", "C8 iw 01",
         // Specializations of several x87 floating point instructions. These
         // are "operand-less" versions of the instruction that take ST(0) and
         // ST(1) as operands. However, they are just specialization of the more
         // generic encoding scheme that encodes one of the operands in the
         // opcode.
         "DD E1", "DD E9", "DE C1", "DE E1", "DE F1", "DE F9",
         // The prefixes. They are listed as XACQUIRE and XRELEASE instructions
         // by the Intel manual, but they can only exist as a part of a larger
         // instruction, never on their own.
         "F2", "F3",
         // The CR8 version of the MOV instruction that writes to the control
         // registers CR0-CR8. These are just specialized versions of the
         // instruction that writes to CR0-CR7 (they add the REX.R bit, and they
         // replace /r in the specification with /0, because no other value of
         // the modrm.reg bits are allowed).
         "REX.R + 0F 20 /0", "REX.R + 0F 22 /0",
         // A version of CRC32 r32, r/m8 that has the REX prefix specified.
         // There is also another version of this instruction without this
         // prefix.
         // Since the REX prefix does not prescribe any particular bit to be
         // set, we believe that it is there simply to say that the instruction
         // may use it to access extended registers.
         "F2 REX 0F 38 F0 /r"});
// NOTE(ondrasej): XLAT is not recognized by the LLVM assembler (unlike its
// no-operand version XLATB).
const std::unordered_set<std::string>* const kRemovedMnemonics =
    new std::unordered_set<std::string>({"XLAT"});

Status RemoveSpecialCaseInstructions(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  const auto is_special_case_instruction =
      [](const InstructionProto& instruction) {
        return ContainsKey(*kRemovedEncodingSpecifications,
                           instruction.raw_encoding_specification()) ||
               ContainsKey(*kRemovedMnemonics,
                           instruction.vendor_syntax().mnemonic());
      };
  instructions->erase(std::remove_if(instructions->begin(), instructions->end(),
                                     is_special_case_instruction),
                      instructions->end());
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveSpecialCaseInstructions, 0);

Status RemoveUndefinedInstructions(InstructionSetProto* instruction_set) {
  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  const auto is_undefined_instruction =
      [](const InstructionProto& instruction) {
        constexpr const char* const kRemovedInstructions[] = {"UD0", "UD1"};
        return c_linear_search(kRemovedInstructions,
                               instruction.vendor_syntax().mnemonic());
      };
  instructions->erase(std::remove_if(instructions->begin(), instructions->end(),
                                     is_undefined_instruction),
                      instructions->end());
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveUndefinedInstructions, 0);

Status RemoveDuplicateInstructionsWithRexPrefix(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  // NOTE(ondrasej): We need to store a copy of the instruction protos, not
  // pointers those in instruction_set, because the contents of instruction_set
  // is modified by std::remove_if().
  std::unordered_map<std::string, std::vector<InstructionProto>>
      instructions_by_encoding;
  RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  for (const InstructionProto& instruction : instruction_set->instructions()) {
    const std::string& specification = instruction.raw_encoding_specification();
    instructions_by_encoding[specification].push_back(instruction);
  }
  Status result = OkStatus();
  const auto is_duplicate_instruction_with_rex =
      [&](const InstructionProto& instruction) {
        static constexpr char kRexPrefixRegex[] = R"(REX *\+ *)";
        static const LazyRE2 rex_prefix_parser = {kRexPrefixRegex};
        StringPiece specification = instruction.raw_encoding_specification();
        if (!RE2::Consume(&specification, *rex_prefix_parser)) return false;
        const std::vector<InstructionProto>* const other_instructions =
            FindOrNull(instructions_by_encoding, specification.as_string());
        // We remove the instruction only if there is a version without the REX
        // prefix that is equivalent in terms of vendor_syntax. If there is not,
        // we return an error status, and keep the instruction to allow
        // debugging with --exegesis_ignore_failing_transforms.
        //
        // Note that there are cases in the manual, where the REX prefix
        // actually means REX.W. Such cases are fixed by
        // FixRexPrefixSpecification() which runs in the default pipeline
        // before this transform, and they should not cause any failures here.
        if (other_instructions == nullptr) {
          const Status error = InvalidArgumentError(StrCat(
              "Instruction does not have a version without the REX prefix: ",
              instruction.raw_encoding_specification()));
          LOG(WARNING) << error;
          UpdateStatus(&result, error);
          return false;
        }
        for (const InstructionProto& other : *other_instructions) {
          if (MessageDifferencer::Equals(other.vendor_syntax(),
                                         instruction.vendor_syntax())) {
            return true;
          }
        }
        const Status error = InvalidArgumentError(
            StrCat("The REX and the non-REX versions differ: ",
                   instruction.raw_encoding_specification()));
        LOG(WARNING) << error;
        UpdateStatus(&result, error);
        return false;
      };
  instructions->erase(std::remove_if(instructions->begin(), instructions->end(),
                                     is_duplicate_instruction_with_rex),
                      instructions->end());
  return result;
}
// The checks performed in the cleanup depend on the encoding specification
// fixes done in FixRexPrefixSpecification(), thus it needs to be executed after
// the encoding specification cleanups.
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveDuplicateInstructionsWithRexPrefix,
                                   1005);

Status RemoveX87InstructionsWithGeneralVersions(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  const std::unordered_set<std::string> kRemovedEncodingSpecifications = {
      "D8 D1", "D8 D9", "DE C9", "DE E9", "D9 C9"};
  google::protobuf::RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  instructions->erase(
      std::remove_if(instructions->begin(), instructions->end(),
                     [&kRemovedEncodingSpecifications](
                         const InstructionProto& instruction) {
                       return ContainsKey(
                           kRemovedEncodingSpecifications,
                           instruction.raw_encoding_specification());
                     }),
      instructions->end());

  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveX87InstructionsWithGeneralVersions, 0);

}  // namespace x86
}  // namespace exegesis
