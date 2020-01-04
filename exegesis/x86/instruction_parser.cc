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

#include "exegesis/x86/instruction_parser.h"

#include <cstdint>
#include <string>

#include "absl/algorithm/container.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "exegesis/base/opcode.h"
#include "exegesis/util/bits.h"
#include "exegesis/util/strings.h"
#include "exegesis/x86/instruction_encoding.h"
#include "glog/logging.h"
#include "util/task/canonical_errors.h"
#include "util/task/status_macros.h"

namespace exegesis {
namespace x86 {
namespace {

using ::absl::c_linear_search;
using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::NotFoundError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;
using ::exegesis::util::StatusOr;

using X86InstructionIndex = X86Architecture::InstructionIndex;

// The "prefix" specifying that the instruction has a two- or three-byte opcode.
constexpr uint8_t kExtendedOpcodeByte = 0x0f;
// The list of primary opcode bytes that require a secondary opcode byte.
constexpr uint8_t kOpcodesWithSecondaryOpcode[] = {0x38, 0x3a};
// All of the floating point instructions use this prefix, Intel® 64 and IA-32
// Architectures Software Developer’s Manual Volume 2 (2A, 2B, 2C & 2D):
// Instruction Set Reference, A-Z, B.17 FLOATING-POINT INSTRUCTION FORMATS AND
// ENCODINGS, page: 2170.
constexpr uint8_t kFloatingPointInstructionPrefix = 0xd8 >> 3;

constexpr char kErrorMessageVexAndLegacyPrefixes[] =
    "The instruction combines a VEX prefix and one of the legacy prefixes.";

// Consumes and returns the first byte from 'slice'. It is the responsibility of
// the caller to ensure that the slice is not empty.
inline uint8_t ConsumeFront(absl::Span<const uint8_t>* span) {
  DCHECK(span != nullptr);
  DCHECK(!span->empty());
  const uint8_t front = span->front();
  span->remove_prefix(1);
  return front;
}

inline bool IsFloatingPointOpcode(uint8_t opcode_first_byte) {
  return GetBitRange(opcode_first_byte, 3, 8) ==
         kFloatingPointInstructionPrefix;
}

// Returns true if 'prefix_byte' is a REX prefix byte. This is true when the
// most significant four bits are 0100.
inline bool IsRexPrefixByte(uint8_t prefix_byte) {
  return (prefix_byte & 0xf0) == kRexPrefixBaseByte;
}

// Only applicable to AMD processers, since Intel does not support XOP prefixes.
// Returns true if 'prefix_byte' is the first byte of a REX or a XOP prefix.
// This is true when the byte is one of 0xc4, 0xc5, or 0x8f.
inline bool IsVexOrXopPrefixByte(uint8_t prefix_byte) {
  return prefix_byte == kTwoByteVexPrefixEscapeByte ||
         prefix_byte == kThreeByteVexPrefixEscapeByte ||
         prefix_byte == kThreeByteXopPrefixEscapeByte;
}

// Returns true if 'prefix_byte' is the first byte of a REX prefix. This is true
// when the byte is one of 0xc4, 0xc5.
inline bool IsVexPrefixByte(uint8_t prefix_byte) {
  return prefix_byte == kTwoByteVexPrefixEscapeByte ||
         prefix_byte == kThreeByteVexPrefixEscapeByte;
}

// Returns true if 'prefix_byte' is the first byte of an EVEX prefix.
inline bool IsEvexPrefixByte(uint8_t prefix_byte) {
  return prefix_byte == kEvexPrefixEscapeByte;
}

inline std::string ConsumeBytes(absl::Span<const uint8_t>* bytes,
                                int num_bytes) {
  DCHECK(bytes != nullptr);
  DCHECK_GE(bytes->size(), 0);
  DCHECK_GE(bytes->size(), num_bytes);
  std::string buffer(bytes->begin(), bytes->begin() + num_bytes);
  bytes->remove_prefix(num_bytes);
  return buffer;
}

}  // namespace

InstructionParser::InstructionParser(const X86Architecture* architecture)
    : architecture_(CHECK_NOTNULL(architecture)) {}

void InstructionParser::Reset() {
  instruction_.Clear();
  encoded_instruction_ = absl::Span<uint8_t>();
  specification_ = nullptr;
}

StatusOr<DecodedInstruction> InstructionParser::ConsumeBinaryEncoding(
    absl::Span<const uint8_t>* encoded_instruction) {
  CHECK(encoded_instruction != nullptr);
  Reset();
  encoded_instruction_ = *encoded_instruction;

  RETURN_IF_ERROR(ConsumePrefixes(encoded_instruction));
  RETURN_IF_ERROR(ConsumeOpcode(encoded_instruction));
  RETURN_IF_ERROR(ConsumeModRmAndSIBIfNeeded(encoded_instruction));
  RETURN_IF_ERROR(ConsumeImmediateValuesIfNeeded(encoded_instruction));
  RETURN_IF_ERROR(ConsumeCodeOffsetIfNeeded(encoded_instruction));
  RETURN_IF_ERROR(ConsumeVexSuffixIfNeeded(encoded_instruction));

  return instruction_;
}

bool InstructionParser::ConsumeSegmentOverridePrefixIfNeeded(
    absl::Span<const uint8_t>* encoded_instruction) {
  CHECK(encoded_instruction != nullptr);
  if (encoded_instruction->empty()) return false;
  bool consume_first_byte = true;
  switch (encoded_instruction->front()) {
    // The segment override and branch prediction prefixes.
    case kCsOverrideByte:
      AddSegmentOverridePrefix(LegacyEncoding::CS_OVERRIDE_OR_BRANCH_NOT_TAKEN);
      break;
    case kSsOverrideByte:
      AddSegmentOverridePrefix(LegacyEncoding::SS_OVERRIDE);
      break;
    case kDsOverrideByte:
      AddSegmentOverridePrefix(LegacyEncoding::DS_OVERRIDE_OR_BRANCH_TAKEN);
      break;
    case kEsOverrideByte:
      AddSegmentOverridePrefix(LegacyEncoding::ES_OVERRIDE);
      break;
    case kFsOverrideByte:
      AddSegmentOverridePrefix(LegacyEncoding::FS_OVERRIDE);
      break;
    case kGsOverrideByte:
      AddSegmentOverridePrefix(LegacyEncoding::GS_OVERRIDE);
      break;
    default:
      consume_first_byte = false;
      break;
  }
  if (consume_first_byte) encoded_instruction->remove_prefix(1);
  return consume_first_byte;
}

bool InstructionParser::ConsumeAddressSizeOverridePrefixIfNeeded(
    absl::Span<const uint8_t>* encoded_instruction) {
  CHECK(encoded_instruction != nullptr);
  if (!encoded_instruction->empty() &&
      encoded_instruction->front() == kAddressSizeOverrideByte) {
    AddAddressSizeOverridePrefix();
    encoded_instruction->remove_prefix(1);
    return true;
  }
  return false;
}

Status InstructionParser::ConsumePrefixes(
    absl::Span<const uint8_t>* encoded_instruction) {
  CHECK(encoded_instruction != nullptr);

  // The segment override and address size override prefixes may appear with all
  // encoding schemes, including the VEX and EVEX prefixes. In such cases, the
  // segment override would be the first byte.
  while (ConsumeSegmentOverridePrefixIfNeeded(encoded_instruction) ||
         ConsumeAddressSizeOverridePrefixIfNeeded(encoded_instruction)) {
  }

  // A VEX/EVEX prefix is mutually exclusive with all other prefixes.
  // If we detect a VEX/EVEX byte at the beginning of the instruction, we
  // use one of the specialized functions for parsing them.
  if (!encoded_instruction->empty() &&
      // XOP is used in AMD64 until Zen, Intel has VEX. Also, 3-byte XOP prefix
      // (0x8f) causes ambiguity with POP64rmm opcode which is also 0x8f.
      // Therefore we do not check for XOP prefix.
      // https://en.wikipedia.org/wiki/XOP_instruction_set (The XOP coding
      // scheme is as close to the VEX scheme as technically possible without
      // risking that the AMD codes overlap with future Intel codes.)
      IsVexPrefixByte(encoded_instruction->front())) {
    return ConsumeVexPrefix(encoded_instruction);
  }
  if (!encoded_instruction->empty() &&
      IsEvexPrefixByte(encoded_instruction->front())) {
    return ConsumeEvexPrefix(encoded_instruction);
  }

  while (!encoded_instruction->empty()) {
    const uint8_t prefix_byte = encoded_instruction->front();
    bool consumed_first_byte = false;
    switch (prefix_byte) {
      // The lock and repeat prefixes.
      case kLockPrefixByte:
        RETURN_IF_ERROR(AddLockOrRepPrefix(LegacyEncoding::LOCK_PREFIX));
        break;
      case kRepNePrefixByte:
        RETURN_IF_ERROR(AddLockOrRepPrefix(LegacyEncoding::REPNE_PREFIX));
        break;
      case kRepPrefixByte:
        RETURN_IF_ERROR(AddLockOrRepPrefix(LegacyEncoding::REP_PREFIX));
        break;

      // The segment override and branch prediction prefixes.
      case kCsOverrideByte:
      case kSsOverrideByte:
      case kDsOverrideByte:
      case kEsOverrideByte:
      case kFsOverrideByte:
      case kGsOverrideByte:
        ConsumeSegmentOverridePrefixIfNeeded(encoded_instruction);
        consumed_first_byte = true;
        break;

      // Operand size override prefix.
      case kOperandSizeOverrideByte:
        RETURN_IF_ERROR(AddOperandSizeOverridePrefix());
        break;

      // Address size override prefix.
      case kAddressSizeOverrideByte:
        AddAddressSizeOverridePrefix();
        break;

      // If we got here, we either found a REX prefix or we found the first byte
      // of the opcode.
      default:
        if (IsRexPrefixByte(prefix_byte)) {
          // TODO(ondrasej): According to wiki.osdev.org, the REX prefix must be
          // the last prefix. Add code to verify that this assumption holds.
          RETURN_IF_ERROR(ParseRexPrefix(prefix_byte));
        } else {
          // The current byte is not a prefix byte. In such case, we need to
          // for the following parser.
          return OkStatus();
        }
    }
    if (!consumed_first_byte) encoded_instruction->remove_prefix(1);
  }

  return InvalidArgumentError(
      "Reached the end of the instruction before parsing the opcode.");
}

Status InstructionParser::ParseRexPrefix(uint8_t prefix_byte) {
  CHECK(IsRexPrefixByte(prefix_byte)) << "Not a REX prefix: " << prefix_byte;
  constexpr int kRexWBit = 3;
  constexpr int kRexRBit = 2;
  constexpr int kRexXBit = 1;
  constexpr int kRexBBit = 0;
  if (instruction_.has_vex_prefix()) {
    return InvalidArgumentError(kErrorMessageVexAndLegacyPrefixes);
  }
  LegacyPrefixes* const legacy_prefixes =
      instruction_.mutable_legacy_prefixes();
  if (legacy_prefixes->has_rex()) {
    return InvalidArgumentError("Multiple REX prefixes were provided.");
  }
  RexPrefix* const rex_prefix = legacy_prefixes->mutable_rex();
  rex_prefix->set_w(IsNthBitSet(prefix_byte, kRexWBit));
  rex_prefix->set_r(IsNthBitSet(prefix_byte, kRexRBit));
  rex_prefix->set_x(IsNthBitSet(prefix_byte, kRexXBit));
  rex_prefix->set_b(IsNthBitSet(prefix_byte, kRexBBit));
  return OkStatus();
}

Status InstructionParser::ConsumeVexPrefix(
    absl::Span<const uint8_t>* encoded_instruction) {
  CHECK(encoded_instruction != nullptr);
  if (encoded_instruction->empty()) {
    return InvalidArgumentError("The VEX prefix is incomplete.");
  }
  const uint8_t vex_prefix_byte = ConsumeFront(encoded_instruction);

  VexPrefix* const vex_prefix = instruction_.mutable_vex_prefix();
  switch (vex_prefix_byte) {
    case kThreeByteVexPrefixEscapeByte: {
      // This is the three-byte VEX (0xC4) prefix.
      if (encoded_instruction->size() < 2) {
        return InvalidArgumentError("The VEX prefix is incomplete.");
      }
      const uint8_t first_data_byte = ConsumeFront(encoded_instruction);
      vex_prefix->set_not_b(IsNthBitSet(first_data_byte, 5));
      vex_prefix->set_not_r(IsNthBitSet(first_data_byte, 7));
      vex_prefix->set_not_x(IsNthBitSet(first_data_byte, 6));
      vex_prefix->set_map_select(static_cast<VexEncoding::MapSelect>(
          GetBitRange(first_data_byte, 0, 5)));
      const uint8_t second_data_byte = ConsumeFront(encoded_instruction);
      vex_prefix->set_w(IsNthBitSet(second_data_byte, 7));
      vex_prefix->set_inverted_register_operand(
          GetBitRange(second_data_byte, 3, 7));
      vex_prefix->set_use_256_bit_vector_length(
          IsNthBitSet(second_data_byte, 2));
      vex_prefix->set_mandatory_prefix(
          static_cast<VexEncoding::MandatoryPrefix>(
              GetBitRange(second_data_byte, 0, 2)));
    } break;
    case kTwoByteVexPrefixEscapeByte: {
      // This is the two-byte VEX prefix.
      if (encoded_instruction->empty()) {
        return InvalidArgumentError("The VEX prefix is incomplete.");
      }
      const uint8_t data_byte = ConsumeFront(encoded_instruction);
      vex_prefix->set_not_b(true);
      vex_prefix->set_not_r(IsNthBitSet(data_byte, 7));
      vex_prefix->set_not_x(true);
      vex_prefix->set_w(false);
      vex_prefix->set_inverted_register_operand(GetBitRange(data_byte, 3, 7));
      vex_prefix->set_use_256_bit_vector_length(IsNthBitSet(data_byte, 2));
      vex_prefix->set_map_select(VexEncoding::MAP_SELECT_0F);
      vex_prefix->set_mandatory_prefix(
          static_cast<VexEncoding::MandatoryPrefix>(
              GetBitRange(data_byte, 0, 2)));
    } break;
    default:
      return InvalidArgumentError(
          absl::StrFormat("Not a VEX prefix byte: %x", vex_prefix_byte));
  }
  return OkStatus();
}

Status InstructionParser::ConsumeEvexPrefix(
    absl::Span<const uint8_t>* encoded_instruction) {
  CHECK(encoded_instruction != nullptr);
  if (encoded_instruction->size() < 4) {
    return InvalidArgumentError("The EVEX prefix is incomplete.");
  }
  CHECK(IsEvexPrefixByte(encoded_instruction->front()));
  encoded_instruction->remove_prefix(1);

  EvexPrefix* const evex_prefix = instruction_.mutable_evex_prefix();
  const uint8_t first_data_byte = ConsumeFront(encoded_instruction);
  evex_prefix->set_not_r(IsNthBitSet(first_data_byte, 7) << 1 |
                         IsNthBitSet(first_data_byte, 4));
  evex_prefix->set_not_x(IsNthBitSet(first_data_byte, 6));
  evex_prefix->set_not_b(IsNthBitSet(first_data_byte, 5));
  evex_prefix->set_map_select(
      static_cast<VexEncoding::MapSelect>(GetBitRange(first_data_byte, 0, 2)));
  if (GetBitRange(first_data_byte, 2, 4) != 0) {
    return InvalidArgumentError(
        "Invalid EVEX prefix: the reserved bits in the first data byte are not "
        "set to 0.");
  }
  const uint8_t second_data_byte = ConsumeFront(encoded_instruction);
  evex_prefix->set_w(IsNthBitSet(second_data_byte, 7));
  uint32_t inverted_register_operand = GetBitRange(second_data_byte, 3, 7);
  evex_prefix->set_mandatory_prefix(static_cast<VexEncoding::MandatoryPrefix>(
      GetBitRange(second_data_byte, 0, 2)));
  if (!IsNthBitSet(second_data_byte, 2)) {
    return InvalidArgumentError(
        "Invalid EVEX prefix: the reserved bit in the second data byte is not "
        "set to 1.");
  }
  const uint8_t third_data_byte = ConsumeFront(encoded_instruction);
  evex_prefix->set_z(IsNthBitSet(third_data_byte, 7));
  evex_prefix->set_vector_length_or_rounding(
      GetBitRange(third_data_byte, 5, 7));
  evex_prefix->set_broadcast_or_control(IsNthBitSet(third_data_byte, 4));
  inverted_register_operand =
      inverted_register_operand | (IsNthBitSet(third_data_byte, 3) << 4);
  evex_prefix->set_inverted_register_operand(inverted_register_operand);
  evex_prefix->set_opmask_register(GetBitRange(third_data_byte, 0, 3));
  return OkStatus();
}

Status InstructionParser::ConsumeOpcode(
    absl::Span<const uint8_t>* encoded_instruction) {
  CHECK(encoded_instruction != nullptr);
  if (encoded_instruction->empty()) {
    return InvalidArgumentError("The opcode is missing.");
  }

  uint32_t opcode_value = ConsumeFront(encoded_instruction);
  if (instruction_.has_vex_prefix() || instruction_.has_evex_prefix()) {
    // VEX instructions have only one opcode byte, but additional bytes may be
    // encoded using the map_select field of the VEX prefix.
    const VexEncoding::MapSelect map_select =
        instruction_.has_vex_prefix() ? instruction_.vex_prefix().map_select()
                                      : instruction_.evex_prefix().map_select();
    switch (map_select) {
      case VexEncoding::MAP_SELECT_0F:
        opcode_value = 0x0f00 | opcode_value;
        break;
      case VexEncoding::MAP_SELECT_0F38:
        opcode_value = 0x0f3800 | opcode_value;
        break;
      case VexEncoding::MAP_SELECT_0F3A:
        opcode_value = 0x0f3a00 | opcode_value;
        break;
      default:
        return InvalidArgumentError(
            absl::StrCat("Invalid vex.map_select value ", map_select));
    }
  } else {
    // Legacy instructions may be using a multi-byte opcode schema. There are
    // three basic classes of multi-byte opcodes:
    // - the "regular" opcodes using 0F, 0F 38, and 0F 3A as opcode extension
    //   bytes.
    // - specialization of a more general instruction. In some cases, the last
    //   byte of the encoding specification in the SDM is a ModR/M byte with
    //   directly specified values. This is used in the SDM for instructions
    //   that have a version with implicit operands.
    // - "irregular" multi-byte opcodes. These are typically system management
    //   or virtualization instructions that do not take any operands and thus
    //   do not need the ModR/M byte.
    // In the parser, we gave up on the systematic approach and simply take the
    // longest possible sequence of bytes from the stream that is an opcode of
    // a legacy instruction. This gives natural precedence to the special cases
    // and irregular instructions over the more general versions.
    uint32_t extended_opcode = opcode_value;
    auto current_byte = encoded_instruction->begin();
    auto opcode_end = encoded_instruction->begin();
    while (current_byte != encoded_instruction->end() &&
           architecture_->IsLegacyOpcodePrefix(Opcode(extended_opcode))) {
      extended_opcode = (extended_opcode << 8) | *current_byte;
      instruction_.set_opcode(extended_opcode);
      specification_ = GetEncodingSpecification(extended_opcode, false);
      ++current_byte;
      if (specification_ != nullptr) {
        opcode_value = extended_opcode;
        opcode_end = current_byte;
      }
    }
    encoded_instruction->remove_prefix(opcode_end -
                                       encoded_instruction->begin());
  }
  instruction_.set_opcode(opcode_value);

  // Use the parsed opcode and the prefixes to look up the instruction encoding
  // specification for the instruction. At this point all we have is those two,
  // but those are enough to determine existence of the ModR/M byte. We lookup
  // for the exact specification after we parsed ModR/M byte, if there is no
  // ModR/M byte this lookup is final.
  specification_ = GetEncodingSpecification(opcode_value, false);
  if (specification_ == nullptr) {
    return NotFoundError(absl::StrCat("The instruction was not found: ",
                                      instruction_.ShortDebugString()));
  }

  return OkStatus();
}

const EncodingSpecification* InstructionParser::GetEncodingSpecification(
    const uint32_t opcode_value, bool check_modrm) {
  const EncodingSpecification* specification = nullptr;
  const X86InstructionIndex instruction_index =
      architecture_->GetInstructionIndex(instruction_, check_modrm);

  if (instruction_index == X86Architecture::kInvalidInstruction) {
    // Sometimes three least significant bits of the instruction are used to
    // encode an operand. In that case the database will have this opcodes with
    // these bits zeroed out, so let's try to search for it. We need to go over
    // all matching instructions, since some opcodes refer to different
    // instructions when combined with operands encoded. For example, 0x90
    // refers to both NOP and XCHG %eax, %eax.
    instruction_.set_opcode(opcode_value & 0xFFFFFFF8);
    const std::vector<X86InstructionIndex> instruction_indices =
        architecture_->GetInstructionIndices(instruction_, check_modrm);
    instruction_.set_opcode(opcode_value);

    if (instruction_indices.empty()) {
      return nullptr;
    }

    // Check that the instruction we found encodes an operand index in the
    // opcode.
    for (const X86InstructionIndex instruction_index : instruction_indices) {
      specification = &architecture_->encoding_specification(instruction_index);
      if (specification->operand_in_opcode() !=
          EncodingSpecification::NO_OPERAND_IN_OPCODE) {
        return specification;
      }
    }

    // We should've found an instruction with opcode specification and returned
    // in the previous loop, if we reached here it means we weren't successful.
    return nullptr;
  } else {
    specification = &architecture_->encoding_specification(instruction_index);
  }

  return specification;
}

Status InstructionParser::ConsumeModRmAndSIBIfNeeded(
    absl::Span<const uint8_t>* encoded_instruction) {
  CHECK(encoded_instruction != nullptr);
  DCHECK(specification_ != nullptr);

  const Opcode opcode(instruction_.opcode());
  if (specification_->modrm_usage() == EncodingSpecification::NO_MODRM_USAGE) {
    return OkStatus();
  }

  if (encoded_instruction->empty()) {
    return InvalidArgumentError("The ModR/M byte is missing.");
  }

  ModRm* const modrm = instruction_.mutable_modrm();
  const uint8_t modrm_byte = ConsumeFront(encoded_instruction);

  modrm->set_addressing_mode(
      static_cast<ModRm::AddressingMode>(GetBitRange(modrm_byte, 6, 8)));
  modrm->set_register_operand(GetBitRange(modrm_byte, 3, 6));
  modrm->set_rm_operand(GetBitRange(modrm_byte, 0, 3));

  // Determine whether the instruction uses the SIB byte. See
  // http://wiki.osdev.org/X86-64_Instruction_Encoding#ModR.2FM_and_SIB_bytes
  // for more details on the encoding of the ModR/M and SIB bytes.
  const bool has_sib =
      modrm->addressing_mode() != ModRm::DIRECT && modrm->rm_operand() == 0x4;
  if (has_sib) {
    if (encoded_instruction->empty()) {
      return InvalidArgumentError("The SIB byte is missing");
    }
    const uint8_t sib_byte = ConsumeFront(encoded_instruction);

    Sib* const sib = instruction_.mutable_sib();
    sib->set_scale(GetBitRange(sib_byte, 6, 8));
    sib->set_index(GetBitRange(sib_byte, 3, 6));
    sib->set_base(GetBitRange(sib_byte, 0, 3));
  }

  // TODO(ondrasej): There are some instructions that use modrm.rm, but that do
  // not allow either register operands or memory operands. We can use
  // InstructionOperand.addressing_mode to determine if this is the case.

  // Determine whether the instruction uses displacement bytes. See the OSDev
  // wiki for more details about the displacement bytes.
  int num_displacement_bytes = 0;
  switch (modrm->addressing_mode()) {
    case ModRm::DIRECT:
      // In the direct mode, the register value is the operand and there can't
      // be any displacement.
      break;
    case ModRm::INDIRECT:
      // In the indirect mode, the displacement presence is more complex to
      // determine:
      // 1. if the SIB byte is not present, there is a 32-bit displacement iff
      //    modrm_.rm == 5.
      // 2. if there is a SIB byte, a 32-bit displacement is used iff
      //    sib.base = 5.
      if (modrm->rm_operand() == 5 ||
          (has_sib && instruction_.sib().base() == 5)) {
        num_displacement_bytes = 4;
      }
      break;
    case ModRm::INDIRECT_WITH_8_BIT_DISPLACEMENT:
      num_displacement_bytes = 1;
      break;
    case ModRm::INDIRECT_WITH_32_BIT_DISPLACEMENT:
      num_displacement_bytes = 4;
      break;
    default:
      LOG(FATAL) << "Unknown addressing mode: " << modrm->addressing_mode();
      break;
  }

  // Decode the displacement. This gets a bit ugly with pointer casts, because
  // we need to convert the value from a stream of uint8_t's to a signed value
  // of a value that depends on the parameters. We do this by casting the
  // pointer to the data to a pointer to a value of the appropriate type. Note
  // that the size of the data is verified in advance, so this should not cause
  // any problems in this respect.
  if (encoded_instruction->size() < num_displacement_bytes) {
    return InvalidArgumentError(absl::StrCat(
        "Displacement bytes are missing - expected ", num_displacement_bytes,
        "found ", encoded_instruction->size()));
  }
  switch (num_displacement_bytes) {
    case 0:
      modrm->set_address_displacement(0);
      break;
    case 1:
      modrm->set_address_displacement(
          *reinterpret_cast<const int8_t*>(encoded_instruction->data()));
      break;
    case 4:
      modrm->set_address_displacement(
          *reinterpret_cast<const int32_t*>(encoded_instruction->data()));
      break;
    default:
      LOG(FATAL) << "Unexpected address displacement size: "
                 << num_displacement_bytes;
  }
  encoded_instruction->remove_prefix(num_displacement_bytes);

  // Reload the specification according to modrm and sib fields.
  specification_ = GetEncodingSpecification(instruction_.opcode(), true);
  if (specification_ == nullptr) {
    return NotFoundError(absl::StrCat("The instruction was not found: ",
                                      instruction_.ShortDebugString()));
  }

  return OkStatus();
}

Status InstructionParser::ConsumeImmediateValuesIfNeeded(
    absl::Span<const uint8_t>* encoded_instruction) {
  CHECK(encoded_instruction != nullptr);
  DCHECK(specification_ != nullptr);

  for (const uint32_t immediate_value_bytes :
       specification_->immediate_value_bytes()) {
    if (encoded_instruction->size() < immediate_value_bytes) {
      return InvalidArgumentError(
          "The immediate value is missing or incomplete");
    }
    instruction_.add_immediate_value(
        ConsumeBytes(encoded_instruction, immediate_value_bytes));
  }

  return OkStatus();
}

Status InstructionParser::ConsumeCodeOffsetIfNeeded(
    absl::Span<const uint8_t>* encoded_instruction) {
  CHECK(encoded_instruction != nullptr);
  DCHECK(specification_ != nullptr);

  if (specification_->code_offset_bytes()) {
    if (encoded_instruction->size() < specification_->code_offset_bytes()) {
      return InvalidArgumentError("The code offset is missing or incomplete");
    }
    *instruction_.mutable_code_offset() =
        ConsumeBytes(encoded_instruction, specification_->code_offset_bytes());
  }

  return OkStatus();
}

Status InstructionParser::ConsumeVexSuffixIfNeeded(
    absl::Span<const uint8_t>* encoded_instruction) {
  CHECK(encoded_instruction != nullptr);
  DCHECK(specification_ != nullptr);

  // NOTE(ondrasej): This code is also compatible with the EVEX prefix, because
  // EVEX uses the same proto for encoding specification.
  if (specification_->has_vex_prefix() &&
      specification_->vex_prefix().has_vex_operand_suffix()) {
    if (encoded_instruction->empty()) {
      return InvalidArgumentError("The VEX suffix is missing");
    }
    const uint8_t vex_suffix = ConsumeFront(encoded_instruction);
    instruction_.mutable_vex_prefix()->set_vex_suffix_value(vex_suffix);
  }

  return OkStatus();
}

Status InstructionParser::AddLockOrRepPrefix(
    LegacyEncoding::LockOrRepPrefix prefix) {
  if (instruction_.has_vex_prefix()) {
    return InvalidArgumentError(kErrorMessageVexAndLegacyPrefixes);
  }
  LegacyPrefixes* const legacy_prefixes =
      instruction_.mutable_legacy_prefixes();
  if (legacy_prefixes->lock_or_rep() != LegacyEncoding::NO_LOCK_OR_REP_PREFIX) {
    return InvalidArgumentError(absl::StrCat(
        "Multiple lock or repeat prefixes were found: ",
        LegacyEncoding::LockOrRepPrefix_Name(legacy_prefixes->lock_or_rep()),
        " and ", LegacyEncoding::LockOrRepPrefix_Name(prefix)));
  }
  legacy_prefixes->set_lock_or_rep(prefix);
  return OkStatus();
}

void InstructionParser::AddSegmentOverridePrefix(
    LegacyEncoding::SegmentOverridePrefix prefix) {
  if (instruction_.segment_override() != LegacyEncoding::NO_SEGMENT_OVERRIDE) {
    VLOG(1) << "Multiple segment override or branch prediction prefixes: "
            << LegacyEncoding::SegmentOverridePrefix_Name(
                   instruction_.segment_override())
            << " and " << LegacyEncoding::SegmentOverridePrefix_Name(prefix);
  }
  instruction_.set_segment_override(prefix);
}

Status InstructionParser::AddOperandSizeOverridePrefix() {
  if (instruction_.has_vex_prefix()) {
    return InvalidArgumentError(kErrorMessageVexAndLegacyPrefixes);
  }
  LegacyPrefixes* const legacy_prefixes =
      instruction_.mutable_legacy_prefixes();
  if (legacy_prefixes->operand_size_override() !=
      LegacyEncoding::NO_OPERAND_SIZE_OVERRIDE) {
    VLOG(1) << "Duplicate operand size override prefix: "
            << ToHumanReadableHexString(encoded_instruction_);
  }
  legacy_prefixes->set_operand_size_override(
      LegacyEncoding::OPERAND_SIZE_OVERRIDE);
  return OkStatus();
}

void InstructionParser::AddAddressSizeOverridePrefix() {
  if (instruction_.address_size_override() !=
      LegacyEncoding::NO_ADDRESS_SIZE_OVERRIDE) {
    VLOG(1) << "Duplicate address size override prefix: "
            << ToHumanReadableHexString(encoded_instruction_);
  }
  instruction_.set_address_size_override(LegacyEncoding::ADDRESS_SIZE_OVERRIDE);
}

}  // namespace x86
}  // namespace exegesis
