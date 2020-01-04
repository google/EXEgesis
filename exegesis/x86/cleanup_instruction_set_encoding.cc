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

#include "exegesis/x86/cleanup_instruction_set_encoding.h"

#include <algorithm>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "exegesis/base/cleanup_instruction_set.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "exegesis/util/instruction_syntax.h"
#include "exegesis/util/status_util.h"
#include "exegesis/x86/cleanup_instruction_set_utils.h"
#include "exegesis/x86/encoding_specification.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "src/google/protobuf/repeated_field.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"
#include "util/task/status_macros.h"

namespace exegesis {
namespace x86 {

using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;

Status AddMissingMemoryOffsetEncoding(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  constexpr char kAddressSizeOverridePrefix[] = "67 ";
  constexpr char k32BitImmediateValueSuffix[] = " id";
  constexpr char k64BitImmediateValueSuffix[] = " io";
  const absl::flat_hash_set<std::string> kEncodingSpecifications = {
      "A0", "REX.W + A0", "A1", "REX.W + A1",
      "A2", "REX.W + A2", "A3", "REX.W + A3"};
  std::vector<InstructionProto> new_instructions;
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    const std::string& specification = instruction.raw_encoding_specification();
    if (kEncodingSpecifications.contains(specification)) {
      new_instructions.push_back(instruction);
      InstructionProto& new_instruction = new_instructions.back();
      new_instruction.set_raw_encoding_specification(
          absl::StrCat(kAddressSizeOverridePrefix, specification,
                       k32BitImmediateValueSuffix));
      // NOTE(ondrasej): Changing the binary encoding of the original proto will
      // either invalidate or change the value of the variable specification.
      // We must thus be careful to not use this variable after it is changed by
      // instruction.set_raw_encoding_specification.
      instruction.set_raw_encoding_specification(
          absl::StrCat(specification, k64BitImmediateValueSuffix));
    }
  }
  for (InstructionProto& new_instruction : new_instructions) {
    instruction_set->add_instructions()->Swap(&new_instruction);
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddMissingMemoryOffsetEncoding, 1000);

namespace {

// Adds the REX.W prefix to the binary encoding specification of the given
// instruction proto. If the instruction proto already has the prefix, it is not
// added and a warning is printed to the log.
void AddRexWPrefixToInstructionProto(InstructionProto* instruction) {
  CHECK(instruction != nullptr);
  constexpr char kRexWPrefix[] = "REX.W";
  const std::string& specification = instruction->raw_encoding_specification();
  if (!absl::StrContains(specification, kRexWPrefix)) {
    instruction->set_raw_encoding_specification(
        absl::StrCat(kRexWPrefix, " ", specification));
  } else {
    LOG(WARNING) << "The instruction already has a REX.W prefix: "
                 << specification;
  }
}

}  // namespace

Status FixEncodingSpecificationOfPopFsAndGs(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  constexpr char kPopInstruction[] = "POP";
  constexpr char k16Bits[] = "16 bits";
  constexpr char k64Bits[] = "64 bits";
  const absl::flat_hash_set<std::string> kFsAndGsOperands = {"FS", "GS"};

  // First find all occurences of the POP FS and GS instructions.
  std::vector<InstructionProto*> pop_instructions;
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    const InstructionFormat& vendor_syntax =
        GetUniqueVendorSyntaxOrDie(instruction);
    if (vendor_syntax.operands_size() == 1 &&
        vendor_syntax.mnemonic() == kPopInstruction &&
        kFsAndGsOperands.contains(vendor_syntax.operands(0).name())) {
      pop_instructions.push_back(&instruction);
    }
  }

  // Make modifications to the 16-bit versions, and make a new copy of the
  // 64-bit versions. We can't add the new copies to instruction_set directly
  // from the loop, because that could invalidate the pointers that we collected
  // in pop_instructions.
  std::vector<InstructionProto> new_pop_instructions;
  for (InstructionProto* const instruction : pop_instructions) {
    // The only way to find out which version it is from the description of the
    // instruction.
    const std::string& description = instruction->description();
    if (absl::StrContains(description, k16Bits)) {
      AddOperandSizeOverrideToInstructionProto(instruction);
    } else if (absl::StrContains(description, k64Bits)) {
      new_pop_instructions.push_back(*instruction);
      AddRexWPrefixToInstructionProto(&new_pop_instructions.back());
    }
  }
  for (InstructionProto& new_pop_instruction : new_pop_instructions) {
    new_pop_instruction.Swap(instruction_set->add_instructions());
  }

  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(FixEncodingSpecificationOfPopFsAndGs, 1000);

Status FixEncodingSpecificationOfPushFsAndGs(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  constexpr char kPushInstruction[] = "PUSH";
  const absl::flat_hash_set<std::string> kFsAndGsOperands = {"FS", "GS"};

  // Find the existing PUSH instructions for FS and GS, and create the remaining
  // versions of the instructions. Note that we can't add the new versions
  // directly to instruction_set, because that might invalidate the iterators
  // used in the for loop.
  std::vector<InstructionProto> new_push_instructions;
  for (const InstructionProto& instruction : instruction_set->instructions()) {
    const InstructionFormat& vendor_syntax =
        GetUniqueVendorSyntaxOrDie(instruction);
    if (vendor_syntax.operands_size() == 1 &&
        vendor_syntax.mnemonic() == kPushInstruction &&
        kFsAndGsOperands.contains(vendor_syntax.operands(0).name())) {
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
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(FixEncodingSpecificationOfPushFsAndGs, 1000);

Status FixAndCleanUpEncodingSpecificationsOfSetInstructions(
    InstructionSetProto* instruction_set) {
  constexpr const char* const kEncodingSpecifications[] = {
      "0F 90", "0F 91", "0F 92", "0F 93", "0F 93", "0F 94",
      "0F 95", "0F 96", "0F 97", "0F 98", "0F 99", "0F 9A",
      "0F 9B", "0F 9C", "0F 9D", "0F 9E", "0F 9F",
  };
  absl::flat_hash_set<std::string> replaced_specifications;
  absl::flat_hash_set<std::string> removed_specifications;
  for (const char* const specification : kEncodingSpecifications) {
    replaced_specifications.insert(specification);
    removed_specifications.insert(absl::StrCat("REX + ", specification));
  }
  google::protobuf::RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();

  // Remove the REX versions of the instruction, because the REX prefix doesn't
  // change anything (it is there only for the register index extension bits).
  instructions->erase(
      std::remove_if(
          instructions->begin(), instructions->end(),
          [&removed_specifications](const InstructionProto& instruction) {
            return removed_specifications.contains(
                instruction.raw_encoding_specification());
          }),
      instructions->end());

  // Fix the binary encoding of the non-REX versions.
  for (InstructionProto& instruction : *instructions) {
    const std::string& specification = instruction.raw_encoding_specification();
    if (replaced_specifications.contains(specification)) {
      instruction.set_raw_encoding_specification(
          absl::StrCat(specification, " /0"));
    }
  }

  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(
    FixAndCleanUpEncodingSpecificationsOfSetInstructions, 1000);

Status FixEncodingSpecificationOfXBegin(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  constexpr char kXBeginEncodingSpecification[] = "C7 F8";
  const absl::flat_hash_map<std::string, std::string>
      kOperandToEncodingSpecification = {{"rel16", "66 C7 F8 cw"},
                                         {"rel32", "C7 F8 cd"}};
  Status status = OkStatus();
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    if (instruction.raw_encoding_specification() ==
        kXBeginEncodingSpecification) {
      const InstructionFormat& vendor_syntax =
          GetUniqueVendorSyntaxOrDie(instruction);
      if (vendor_syntax.operands_size() != 1) {
        status = util::InvalidArgumentError(
            "Unexpected number of arguments of a XBEGIN instruction: ");
        LOG(ERROR) << status;
        continue;
      }
      if (!gtl::FindCopy(kOperandToEncodingSpecification,
                         vendor_syntax.operands(0).name(),
                         instruction.mutable_raw_encoding_specification())) {
        status = InvalidArgumentError(
            absl::StrCat("Unexpected argument of a XBEGIN instruction: ",
                         vendor_syntax.operands(0).name()));
        LOG(ERROR) << status;
        continue;
      }
    }
  }
  return status;
}
REGISTER_INSTRUCTION_SET_TRANSFORM(FixEncodingSpecificationOfXBegin, 1000);

Status FixEncodingSpecifications(InstructionSetProto* instruction_set) {
  const RE2 fix_w0_regexp("^(VEX[^ ]*\\.)0 ");
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    std::string specification =
        absl::StrReplaceAll(instruction.raw_encoding_specification(),
                            {{"0f", "0F"}, {"imm8", "ib"}, {"/ib", "ib"}});
    RE2::Replace(&specification, fix_w0_regexp, "\\1W0 ");

    instruction.set_raw_encoding_specification(specification);
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(FixEncodingSpecifications, 1000);

Status DropModRmModDetailsFromEncodingSpecifications(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  const RE2 drop_mod_rm_mod_regexp(R"( *\(mod.*\)$)");
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    RE2::Replace(instruction.mutable_raw_encoding_specification(),
                 drop_mod_rm_mod_regexp, "");
  }

  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(
    DropModRmModDetailsFromEncodingSpecifications, 1000);

Status AddMissingModRmAndImmediateSpecification(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  constexpr char kFullModRmSuffix[] = "/r";
  const absl::flat_hash_set<std::string> kMissingModRMInstructionMnemonics = {
      "CVTDQ2PD", "VMOVD", "WRSSD", "WRSSQ", "WRUSSD", "WRUSSQ"};
  constexpr char kImmediateByteSuffix[] = "ib";
  const absl::flat_hash_set<std::string> kMissingImmediateInstructionMnemonics =
      {
          "KSHIFTLB", "KSHIFTLW", "KSHIFTLD",  "KSHIFTLQ",    "KSHIFTRB",
          "KSHIFTRW", "KSHIFTRD", "KSHIFTRQ",  "VFIXUPIMMPS", "VFPCLASSSS",
          "VRANGESD", "VRANGESS", "VREDUCESD",
      };
  constexpr char kVSibSuffix[] = "/vsib";
  const absl::flat_hash_set<std::string> kMissingVSibInstructionMnemonics = {
      "VGATHERDPD", "VGATHERQPD", "VGATHERDPS", "VGATHERQPS",
      "VPGATHERDD", "VPGATHERDQ", "VPGATHERQD", "VPGATHERQQ",
  };

  // Fixes instruction encodings for instructions matching the given mnemonics
  // by adding the given suffix if need be.
  const auto maybe_fix = [](const absl::flat_hash_set<std::string>& mnemonics,
                            const absl::string_view suffix,
                            InstructionProto* instruction) {
    Status status = OkStatus();
    if (ContainsVendorSyntaxMnemonic(mnemonics, *instruction)) {
      if (instruction->raw_encoding_specification().empty()) {
        status = InvalidArgumentError(absl::StrCat(
            "The instruction does not have binary encoding specification: ",
            instruction->DebugString()));
      }

      if (!absl::EndsWith(instruction->raw_encoding_specification(), suffix)) {
        instruction->set_raw_encoding_specification(absl::StrCat(
            instruction->raw_encoding_specification(), " ", suffix));
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
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddMissingModRmAndImmediateSpecification,
                                   1000);

Status FixRexPrefixSpecification(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  const absl::flat_hash_map<std::string, std::string> kReplacements = {
      {"REX + 0F B2 /r", "REX.W + 0F B2 /r"},
      {"REX + 0F B4 /r", "REX.W + 0F B4 /r"},
      {"REX + 0F B5 /r", "REX.W + 0F B5 /r"},
      {"REX + 0F BE /r", "REX.W + 0F BE /r"}};
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    const std::string* const replacement_raw_encoding_specification =
        gtl::FindOrNull(kReplacements,
                        instruction.raw_encoding_specification());
    if (replacement_raw_encoding_specification == nullptr) continue;
    instruction.set_raw_encoding_specification(
        *replacement_raw_encoding_specification);
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(FixRexPrefixSpecification, 1000);

Status ParseEncodingSpecifications(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  Status status = OkStatus();
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    const StatusOr<EncodingSpecification> encoding_specification_or_status =
        ParseEncodingSpecification(instruction.raw_encoding_specification());
    if (encoding_specification_or_status.ok()) {
      *instruction.mutable_x86_encoding_specification() =
          encoding_specification_or_status.ValueOrDie();
    } else {
      UpdateStatus(&status, encoding_specification_or_status.status());
      LOG(WARNING) << "Could not parse encoding specification: "
                   << instruction.raw_encoding_specification();
    }
  }
  return status;
}
// We must parse the encoding specifications after running all other encoding
// specification cleanups, but before running any other transform.
REGISTER_INSTRUCTION_SET_TRANSFORM(ParseEncodingSpecifications, 1010);

Status ConvertEncodingSpecificationOfX87FpuWithDirectAddressing(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  const absl::flat_hash_map<std::string, std::string> kReplacements = {
      {"D8 C0+i", "D8 /0"},  // FADD store to ST(0)
      {"DC C0+i", "DC /0"},  // FADD store to ST(i)
      {"DE C0+i", "DE /0"},  // FADDP
      {"D8 D0+i", "D8 /2"},  // FCOM
      {"D8 D8+i", "D8 /3"},  // FCOMP
      {"DF F0+i", "DF /6"},  // FCOMIP
      {"D8 F0+i", "D8 /6"},  // FDIV ST(0) = ST(i) / ST(0)
      {"D8 F8+i", "D8 /7"},  // FDIV ST(0) = ST(0) / ST(i)
      {"DC F0+i", "DC /6"},  // FDIV ST(i) = ST(i) / ST(0)
      {"DC F8+i", "DC /7"},  // FDIV ST(i) = ST(0) / ST(i)
      {"DE F0+i", "DE /6"},  // FDIVRP
      {"DE F8+i", "DE /7"},  // FDIVP
      {"DD C0+i", "DD /0"},  // FFREE
      {"D9 C0+i", "D9 /0"},  // FLD
      {"D8 C8+i", "D8 /1"},  // FMUL ST(0) = ST(0) * ST(i)
      {"DC C8+i", "DC /1"},  // FMUL ST(i) = ST(0) * ST(i)
      {"DE C8+i", "DE /1"},  // FMULP
      {"DD D0+i", "DD /2"},  // FST
      {"DD D8+i", "DD /3"},  // FSTP
      {"D8 E0+i", "D8 /4"},  // FSUB ST(0) = ST(i) - ST(0)
      {"D8 E8+i", "D8 /5"},  // FDIV ST(0) = ST(0) - ST(i)
      {"DC E0+i", "DC /4"},  // FDIV ST(i) = ST(i) - ST(0)
      {"DC E8+i", "DC /5"},  // FDIV ST(i) = ST(0) - ST(i)
      {"DE E8+i", "DE /5"},  // FSUBP
      {"DE E0+i", "DE /4"},  // FSUBRP
      {"DD E0+i", "DD /4"},  // FUCOM
      {"DD E8+i", "DD /5"},  // FUCOMP
      {"DB E8+i", "DB /5"},  // FUCOMI
      {"DF E8+i", "DF /5"},  // FUCOMIP
      {"D9 C8+i", "D9 /1"},  // FUCOMIP
      {"DA C0+i", "DA /0"},  // FCMOVb
      {"DA C8+i", "DA /1"},  // FCMOVe
      {"DA D0+i", "DA /2"},  // FCMOVbe
      {"DA D8+i", "DA /3"},  // FCMOVu
      {"DB C0+i", "DB /0"},  // FCMOVnb
      {"DB C8+i", "DB /1"},  // FCMOVne
      {"DB D0+i", "DB /2"},  // FCMOVnbe
      {"DB D8+i", "DB /3"},  // FCMOVnu
      {"DB F0+i", "DB /6"},  // FCOMI
  };
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    const std::string* const replacement_raw_encoding_specification =
        gtl::FindOrNull(kReplacements,
                        instruction.raw_encoding_specification());
    if (replacement_raw_encoding_specification == nullptr) continue;
    instruction.set_raw_encoding_specification(
        *replacement_raw_encoding_specification);
  }
  return OkStatus();
}
// We must convert the encoding specifications after running all other encoding
// specification cleanups, but before running any other transform.
REGISTER_INSTRUCTION_SET_TRANSFORM(
    ConvertEncodingSpecificationOfX87FpuWithDirectAddressing, 1005);

Status AddRexWPrefixedVersionOfStr(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  constexpr char kStrEncoding[] = "0F 00 /1";

  for (const InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    if (instruction.raw_encoding_specification() == kStrEncoding) {
      InstructionProto str_with_rex = instruction;
      AddRexWPrefixToInstructionProto(&str_with_rex);
      *instruction_set->add_instructions() = str_with_rex;
      break;
    }
  }

  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddRexWPrefixedVersionOfStr, 1000);

}  // namespace x86
}  // namespace exegesis
