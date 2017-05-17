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
#include <unordered_set>
#include "strings/string.h"

#include "exegesis/base/cleanup_instruction_set.h"
#include "exegesis/proto/instructions.pb.h"
#include "glog/logging.h"
#include "src/google/protobuf/repeated_field.h"
#include "strings/string_view.h"
#include "util/gtl/container_algorithm.h"
#include "util/gtl/map_util.h"
#include "util/task/status.h"

namespace cpu_instructions {
namespace x86 {

using ::cpu_instructions::gtl::c_linear_search;
using ::cpu_instructions::util::OkStatus;
using ::cpu_instructions::util::Status;

Status RemoveDuplicateInstructions(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  std::unordered_set<string> visited_instructions;

  // A function that keeps track of instruction it has already encountered.
  // Returns true if an instruction was already seen, false otherwise.
  auto remove_instruction_if_visited =
      [&visited_instructions](const InstructionProto& instruction) {
        const string serialized_instruction = instruction.SerializeAsString();
        return !visited_instructions.insert(serialized_instruction).second;
      };

  ::google::protobuf::RepeatedPtrField<InstructionProto>* const instructions =
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
  ::google::protobuf::RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  const auto uses_fwait_for_sync = [](const InstructionProto& instruction) {
    return StringPiece(instruction.raw_encoding_specification())
        .starts_with(kFWaitPrefix);
  };
  instructions->erase(std::remove_if(instructions->begin(), instructions->end(),
                                     uses_fwait_for_sync),
                      instructions->end());
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveInstructionsWaitingForFpuSync, 0);

Status RemoveNonEncodableInstructions(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  ::google::protobuf::RepeatedPtrField<InstructionProto>* const instructions =
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
  ::google::protobuf::RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  const auto uses_rep_or_repne = [](const InstructionProto& instruction) {
    StringPiece mnemonic(instruction.vendor_syntax().mnemonic());
    return mnemonic.starts_with(kRepPrefix);
  };
  instructions->erase(std::remove_if(instructions->begin(), instructions->end(),
                                     uses_rep_or_repne),
                      instructions->end());
  return OkStatus();
}
// TODO(ondrasej): In addition to removing them, we should also add an attribute
// saying whether the REP/REPE/REPNE prefix is allowed.
REGISTER_INSTRUCTION_SET_TRANSFORM(RemoveRepAndRepneInstructions, 0);

const std::unordered_set<string>* const kRemovedEncodingSpecifications =
    new std::unordered_set<string>(
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
const std::unordered_set<string>* const kRemovedMnemonics =
    new std::unordered_set<string>({"XLAT"});

Status RemoveSpecialCaseInstructions(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  ::google::protobuf::RepeatedPtrField<InstructionProto>* const instructions =
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
  ::google::protobuf::RepeatedPtrField<InstructionProto>* const instructions =
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

}  // namespace x86
}  // namespace cpu_instructions
