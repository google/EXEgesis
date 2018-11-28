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

#include "exegesis/x86/instruction_encoder.h"

#include <cstdint>
#include <string>
#include <utility>

#include "absl/strings/str_cat.h"
#include "exegesis/util/bits.h"
#include "exegesis/x86/instruction_encoding.h"
#include "glog/logging.h"
#include "src/google/protobuf/repeated_field.h"
#include "util/task/canonical_errors.h"
#include "util/task/status_macros.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::util::InternalError;
using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;
using ::exegesis::util::StatusOr;

// Implements the instruction encoder. The encoding functionality is split into
// several methods, and they all emit to the buffer managed by this class.
class InstructionEncoder {
 public:
  InstructionEncoder(const EncodingSpecification* specification,
                     const DecodedInstruction* decoded_instruction)
      : specification_(CHECK_NOTNULL(specification)),
        decoded_instruction_(CHECK_NOTNULL(decoded_instruction)) {}

  // Disallow copy and assign.
  InstructionEncoder(const InstructionEncoder&) = delete;
  InstructionEncoder& operator=(const InstructionEncoder&) = delete;

  // Validates the instruction data. Returns Status::OK if everything is OK,
  // otherwise returns an error status and a description of the problem.
  Status Validate() const;

  // Encodes the instruction and returns its binary encoding. Assumes that the
  // instruction is valid with respect to InstructionEncoder::Validate().
  std::vector<uint8_t> Encode();

 private:
  static constexpr int kMaxInstructionBytes = 17;

  Status ValidatePrefix() const;
  void EncodePrefix();

  // Methods for validating and encoding the legacy prefixes.
  Status ValidateLegacyPrefixes() const;
  Status ValidateRexPrefix() const;
  Status ValidateLockOrRepPrefix() const;
  Status ValidateOperandSizeOverridePrefix() const;
  Status ValidateAddressSizeOverridePrefix() const;
  void EncodeLegacyPrefixes();
  void EncodeRexPrefixIfNeeded();
  void EncodeLockOrRepPrefixIfNeeded();
  void EncodeSegmentOverridePrefixIfNeeded();
  void EncodeOperandSizeOverridePrefixIfNeeded();
  void EncodeAddressSizeOverridePrefixIfNeeded();

  // Methods for validating and encoding VEX and EVEX instructions.
  Status ValidateVexPrefix() const;
  Status ValidateEvexPrefix() const;
  Status ValidateVexSuffix() const;
  void EncodeVexPrefix();
  void EncodeEvexPrefix();
  void EncodeVexSuffixIfNeeded();

  // Methods for validating and encoding the opcode.
  Status ValidateOpcode() const;
  void EncodeOpcode();

  // Methods for validating and encoding the ModR/M and SIB bytes and the
  // corresponding displacement values.
  Status ValidateModRM() const;
  void EncodeModRMIfNeeded();

  // Methods for validating and encoding the immediate values of the function.
  Status ValidateImmediateValues() const;
  void EncodeImmediateValues();

  // Functions for emitting values.
  void EmitByte(uint8_t byte) { instruction_buffer_.push_back(byte); }
  void EmitDWord(uint32_t value) {
    EmitByte(value & 0xff);
    EmitByte((value >> 8) & 0xff);
    EmitByte((value >> 16) & 0xff);
    EmitByte(value >> 24);
  }
  void EmitString(const std::string& data) {
    for (const char byte : data) {
      EmitByte(static_cast<uint8_t>(byte));
    }
  }

  std::vector<uint8_t> instruction_buffer_;

  const EncodingSpecification* const specification_;
  const DecodedInstruction* const decoded_instruction_;
};

constexpr int InstructionEncoder::kMaxInstructionBytes;

Status InstructionEncoder::Validate() const {
  RETURN_IF_ERROR(ValidatePrefix());
  RETURN_IF_ERROR(ValidateOpcode());
  RETURN_IF_ERROR(ValidateModRM());
  RETURN_IF_ERROR(ValidateImmediateValues());
  RETURN_IF_ERROR(ValidateVexSuffix());
  return OkStatus();
}

std::vector<uint8_t> InstructionEncoder::Encode() {
  instruction_buffer_.clear();
  instruction_buffer_.reserve(kMaxInstructionBytes);
  EncodePrefix();
  EncodeOpcode();
  EncodeModRMIfNeeded();
  EncodeImmediateValues();
  EncodeVexSuffixIfNeeded();
  return std::move(instruction_buffer_);
}

Status InstructionEncoder::ValidatePrefix() const {
  if (specification_->has_legacy_prefixes()) {
    return ValidateLegacyPrefixes();
  } else if (specification_->has_vex_prefix()) {
    switch (specification_->vex_prefix().prefix_type()) {
      case VEX_PREFIX:
        return ValidateVexPrefix();
      case EVEX_PREFIX:
        return ValidateEvexPrefix();
      default:
        return InternalError(
            absl::StrCat("The type of the VEX/EVEX prefix is not valid: ",
                         specification_->vex_prefix().prefix_type()));
    }
  }
  return OkStatus();
}

void InstructionEncoder::EncodePrefix() {
  EncodeSegmentOverridePrefixIfNeeded();
  EncodeAddressSizeOverridePrefixIfNeeded();
  if (specification_->has_legacy_prefixes()) {
    EncodeLegacyPrefixes();
  } else if (decoded_instruction_->has_vex_prefix()) {
    // NOTE(ondrasej): We have already ensured that instruction_data_ and
    // specification_ are in sync through ValidatePrefix. If they were not, we
    // wouldn't be executing this code.
    EncodeVexPrefix();
  } else if (decoded_instruction_->has_evex_prefix()) {
    EncodeEvexPrefix();
  }
}

Status InstructionEncoder::ValidateLegacyPrefixes() const {
  CHECK(specification_->has_legacy_prefixes());
  if (decoded_instruction_->has_vex_prefix()) {
    return InvalidArgumentError(
        "The encoding specification prescribes legacy prefixes but the "
        "instruction data used a VEX prefix.");
  }

  RETURN_IF_ERROR(ValidateRexPrefix());
  RETURN_IF_ERROR(ValidateLockOrRepPrefix());
  RETURN_IF_ERROR(ValidateOperandSizeOverridePrefix());
  RETURN_IF_ERROR(ValidateAddressSizeOverridePrefix());
  return OkStatus();
}

void InstructionEncoder::EncodeLegacyPrefixes() {
  EncodeRexPrefixIfNeeded();
  EncodeLockOrRepPrefixIfNeeded();
  EncodeOperandSizeOverridePrefixIfNeeded();
}

Status InstructionEncoder::ValidateRexPrefix() const {
  const RexPrefix& rex = decoded_instruction_->legacy_prefixes().rex();
  if (!PrefixMatchesSpecification(
          specification_->legacy_prefixes().rex_w_prefix(), rex.w())) {
    return InvalidArgumentError(
        "The REX.W prefix does not match the specification.");
  }
  return OkStatus();
}

void InstructionEncoder::EncodeRexPrefixIfNeeded() {
  const RexPrefix& rex = decoded_instruction_->legacy_prefixes().rex();
  // We use the fact that Boolean true is always converted to an integer as 1,
  // and boolean false is always converted as 0. With this conversion logic, all
  // we need to do is to bit-shift the ones to the right position.
  const uint8_t rex_b = rex.b();
  const uint8_t rex_r = rex.r();
  const uint8_t rex_w = rex.w();
  const uint8_t rex_x = rex.x();
  if (rex_b || rex_r || rex_w || rex_x) {
    EmitByte(0x40 | rex_b | (rex_r << 2) | (rex_w << 3) | (rex_x << 1));
  }
}

Status InstructionEncoder::ValidateLockOrRepPrefix() const {
  const LegacyPrefixEncodingSpecification& specification_prefixes =
      specification_->legacy_prefixes();
  const LegacyPrefixes& instruction_prefixes =
      decoded_instruction_->legacy_prefixes();
  if (specification_prefixes.has_mandatory_repe_prefix() &&
      instruction_prefixes.lock_or_rep() != LegacyEncoding::REP_PREFIX) {
    return InvalidArgumentError(
        "The encoding specification prescribes a REP/REPE prefix but the "
        "instruction does not use it.");
  }
  if (specification_prefixes.has_mandatory_repne_prefix() &&
      instruction_prefixes.lock_or_rep() != LegacyEncoding::REPNE_PREFIX) {
    return InvalidArgumentError(
        "The encoding specification prescribes a REP/REPNE prefix but the "
        "instruction does not use it.");
  }
  return OkStatus();
}

void InstructionEncoder::EncodeLockOrRepPrefixIfNeeded() {
  // The order in the values is such that the array can be indexed by the values
  // of the enum LegacyEncoding::LockOrRepPrefix.
  constexpr uint8_t kLockOrRepPrefixByte[] = {0, kLockPrefixByte,
                                              kRepPrefixByte, kRepNePrefixByte};
  const LegacyEncoding::LockOrRepPrefix prefix =
      decoded_instruction_->legacy_prefixes().lock_or_rep();
  if (prefix != LegacyEncoding::NO_LOCK_OR_REP_PREFIX) {
    EmitByte(kLockOrRepPrefixByte[prefix]);
  }
}

void InstructionEncoder::EncodeSegmentOverridePrefixIfNeeded() {
  // The order of the values is such that the array can be indexed by the values
  // of the enum LegacyEncoding::SegmentOverridePrefix.
  constexpr uint8_t kSegmentOverridePrefixByte[] = {0,
                                                    kCsOverrideByte,
                                                    kSsOverrideByte,
                                                    kDsOverrideByte,
                                                    kEsOverrideByte,
                                                    kFsOverrideByte,
                                                    kGsOverrideByte};
  const LegacyEncoding::SegmentOverridePrefix prefix =
      decoded_instruction_->segment_override();
  if (prefix != LegacyEncoding::NO_SEGMENT_OVERRIDE) {
    EmitByte(kSegmentOverridePrefixByte[prefix]);
  }
}

Status InstructionEncoder::ValidateOperandSizeOverridePrefix() const {
  const bool has_operand_size_override =
      decoded_instruction_->legacy_prefixes().operand_size_override() ==
      LegacyEncoding::OPERAND_SIZE_OVERRIDE;
  if (!PrefixMatchesSpecification(
          specification_->legacy_prefixes().operand_size_override_prefix(),
          has_operand_size_override)) {
    return InvalidArgumentError(
        "The operand size override prefix does not match the specification.");
  }
  return OkStatus();
}

void InstructionEncoder::EncodeOperandSizeOverridePrefixIfNeeded() {
  if (decoded_instruction_->legacy_prefixes().operand_size_override() ==
      LegacyEncoding::OPERAND_SIZE_OVERRIDE) {
    EmitByte(kOperandSizeOverrideByte);
  }
}

Status InstructionEncoder::ValidateAddressSizeOverridePrefix() const {
  if (specification_->legacy_prefixes()
          .has_mandatory_address_size_override_prefix() &&
      decoded_instruction_->address_size_override() ==
          LegacyEncoding::NO_ADDRESS_SIZE_OVERRIDE) {
    return InvalidArgumentError(
        "The encoding specification prescribes an operand size override prefix "
        "but the instruction does not use it.");
  }
  return OkStatus();
}

void InstructionEncoder::EncodeAddressSizeOverridePrefixIfNeeded() {
  if (decoded_instruction_->address_size_override() ==
      LegacyEncoding::ADDRESS_SIZE_OVERRIDE) {
    EmitByte(kAddressSizeOverrideByte);
  }
}

Status InstructionEncoder::ValidateVexPrefix() const {
  CHECK(specification_->has_vex_prefix());
  const VexPrefixEncodingSpecification& vex_specification =
      specification_->vex_prefix();
  CHECK_EQ(vex_specification.prefix_type(), VEX_PREFIX);
  if (!decoded_instruction_->has_vex_prefix()) {
    return InvalidArgumentError(
        "The encoding specification prescribes a VEX prefix but the "
        "instruction does not have it");
  }

  const VexPrefix& vex = decoded_instruction_->vex_prefix();

  // Validate the fields of the VEX prefix.
  RETURN_IF_ERROR(ValidateVexRegisterOperandBits(
      vex_specification, vex.inverted_register_operand()));
  RETURN_IF_ERROR(ValidateVectorSizeBits(vex_specification.vector_size(),
                                         vex.use_256_bit_vector_length(),
                                         VEX_PREFIX));
  RETURN_IF_ERROR(ValidateVexWBit(vex_specification.vex_w_usage(), vex.w()));
  RETURN_IF_ERROR(ValidateMandatoryPrefixBits(vex_specification, vex));
  RETURN_IF_ERROR(ValidateMapSelectBits(vex_specification, vex));
  return OkStatus();
}

Status InstructionEncoder::ValidateEvexPrefix() const {
  CHECK(specification_->has_vex_prefix());
  const VexPrefixEncodingSpecification& evex_specification =
      specification_->vex_prefix();
  CHECK_EQ(evex_specification.prefix_type(), EVEX_PREFIX);
  if (!decoded_instruction_->has_evex_prefix()) {
    return InvalidArgumentError(
        "The encoding specification prescribes an EVEX prefix but the "
        "instruction does not have it");
  }
  const EvexPrefix& evex = decoded_instruction_->evex_prefix();

  // Validate the fields of the EVEX prefix.
  RETURN_IF_ERROR(ValidateVexRegisterOperandBits(
      evex_specification, evex.inverted_register_operand()));
  RETURN_IF_ERROR(ValidateVectorSizeBits(evex_specification.vector_size(),
                                         evex.vector_length_or_rounding(),
                                         EVEX_PREFIX));
  RETURN_IF_ERROR(ValidateVexWBit(evex_specification.vex_w_usage(), evex.w()));
  RETURN_IF_ERROR(ValidateMandatoryPrefixBits(evex_specification, evex));
  RETURN_IF_ERROR(ValidateMapSelectBits(evex_specification, evex));
  RETURN_IF_ERROR(ValidateEvexBBit(evex_specification, *decoded_instruction_));
  RETURN_IF_ERROR(
      ValidateEvexOpmask(evex_specification, *decoded_instruction_));
  return OkStatus();
}

void InstructionEncoder::EncodeVexPrefix() {
  const VexPrefixEncodingSpecification& vex_specification =
      specification_->vex_prefix();
  const VexPrefix& vex = decoded_instruction_->vex_prefix();

  // We allow 0 for the inverted register operand when it is not used, because
  // it is equivalent to the field not being set to the proto. However, in the
  // encoding, we must always use the inverted zero value.
  const uint32_t inverted_register_operand =
      vex_specification.vex_operand_usage() == VEX_OPERAND_IS_NOT_USED
          ? 15
          : vex.inverted_register_operand();
  // Use the two-byte form of the prefix whenever possible.
  const bool can_use_two_byte_form =
      vex.not_x() && vex.not_b() && !vex.w() && vex.map_select() == 1;
  // The bit compositions use the fact that boolean true always converts to an
  // integer as 1, and boolean false always converts as 0. Using this fact, we
  // can just shift the ones into the right position.
  if (can_use_two_byte_form) {
    EmitByte(kTwoByteVexPrefixEscapeByte);
    EmitByte((vex.not_r() << 7) |
             (GetBitRange(inverted_register_operand, 0, 4) << 3) |
             (vex.use_256_bit_vector_length() << 2) |
             GetBitRange(vex.mandatory_prefix(), 0, 2));
  } else {
    EmitByte(kThreeByteVexPrefixEscapeByte);
    EmitByte((vex.not_r() << 7) | (vex.not_x() << 6) | (vex.not_b() << 5) |
             GetBitRange(vex.map_select(), 0, 5));
    EmitByte((vex.w() << 7) |
             (GetBitRange(inverted_register_operand, 0, 4) << 3) |
             (vex.use_256_bit_vector_length() << 2) |
             GetBitRange(vex.mandatory_prefix(), 0, 2));
  }
}

void InstructionEncoder::EncodeEvexPrefix() {
  const VexPrefixEncodingSpecification& evex_specification =
      specification_->vex_prefix();
  const EvexPrefix& evex = decoded_instruction_->evex_prefix();

  // We allow 0 for the inverted register operand when it is not used, because
  // it is equivalent to the field not being set to the proto. However, in the
  // encoding, we must always use the inverted zero value.
  const uint32_t inverted_register_operand =
      evex_specification.vex_operand_usage() == VEX_OPERAND_IS_NOT_USED
          ? 31
          : evex.inverted_register_operand();

  EmitByte(kEvexPrefixEscapeByte);
  EmitByte((IsNthBitSet(evex.not_r(), 0) << 7) | (evex.not_x() << 6) |
           (evex.not_b() << 5) | (IsNthBitSet(evex.not_r(), 1) << 4) |
           (GetBitRange(evex.map_select(), 0, 2)));
  EmitByte((evex.w() << 7) |
           (GetBitRange(inverted_register_operand, 0, 4) << 3) | (1 << 2) |
           GetBitRange(evex.mandatory_prefix(), 0, 2));
  EmitByte((evex.z() << 7) |
           (GetBitRange(evex.vector_length_or_rounding(), 0, 2) << 5) |
           (evex.broadcast_or_control() << 4) |
           (IsNthBitSet(evex.inverted_register_operand(), 4) << 3) |
           GetBitRange(evex.opmask_register(), 0, 3));
}

Status InstructionEncoder::ValidateVexSuffix() const {
  if (!specification_->has_vex_prefix()) {
    return OkStatus();
  }
  const VexPrefixEncodingSpecification& vex_specification =
      specification_->vex_prefix();
  if (!vex_specification.has_vex_operand_suffix() &&
      decoded_instruction_->vex_prefix().vex_suffix_value() > 0) {
    return InvalidArgumentError(
        "The instruction does not use the VEX suffix but the data was "
        "provided.");
  }
  return OkStatus();
}

void InstructionEncoder::EncodeVexSuffixIfNeeded() {
  const VexPrefixEncodingSpecification& vex_specification =
      specification_->vex_prefix();
  if (vex_specification.has_vex_operand_suffix()) {
    EmitByte(decoded_instruction_->vex_prefix().vex_suffix_value());
  }
}

Status InstructionEncoder::ValidateOpcode() const {
  // The opcode will be filled in by the encoder.
  if (decoded_instruction_->opcode() == 0) return OkStatus();
  bool is_valid = false;
  if (specification_->operand_in_opcode() ==
      EncodingSpecification::NO_OPERAND_IN_OPCODE) {
    // There is no operand encoded in the opcde; we can do an exact match.
    is_valid = decoded_instruction_->opcode() == specification_->opcode();
  } else {
    // The least significant three bits of the opcode contain a register index.
    // In the specification, these bits are always set to 0; however, in
    // decoded_instruction_, they will contain the actual register index used by
    // the instruction.
    const uint32_t opcode_base =
        ClearBitRange(decoded_instruction_->opcode(), 0, 3);
    is_valid = opcode_base == specification_->opcode();
  }

  if (!is_valid) {
    return InvalidArgumentError(
        absl::StrCat("The opcode in the binary encoding specification (",
                     specification_->opcode(),
                     ") does not match the opcode in the instruction data (",
                     decoded_instruction_->opcode(), ")."));
  }
  return OkStatus();
}

void InstructionEncoder::EncodeOpcode() {
  // Instructions with the VEX prefix use only one operand byte, but for
  // simplicity, we add the translated map_select value to the opcode in our
  // data structures. We need to remove it again during the encoding.
  const uint32_t raw_opcode = decoded_instruction_->opcode() > 0
                                  ? decoded_instruction_->opcode()
                                  : specification_->opcode();
  // NOTE(ondrasej): specification_->has_vex_prefix() covers both the VEX prefix
  // and the EVEX prefix.
  const uint32_t opcode =
      specification_->has_vex_prefix() ? raw_opcode & 0xff : raw_opcode;
  if (opcode > 0xffffff) EmitByte(static_cast<uint8_t>(opcode >> 24));
  if (opcode > 0xffff) EmitByte(static_cast<uint8_t>(opcode >> 16));
  if (opcode > 0xff) EmitByte(static_cast<uint8_t>(opcode >> 8));
  EmitByte(static_cast<uint8_t>(opcode));
}

Status InstructionEncoder::ValidateModRM() const {
  const bool specification_requires_modrm_byte =
      specification_->modrm_usage() != EncodingSpecification::NO_MODRM_USAGE;
  if (specification_requires_modrm_byte != decoded_instruction_->has_modrm()) {
    return InvalidArgumentError(
        "Instruction is missing a required ModR/M byte.");
  }
  if (!specification_requires_modrm_byte && decoded_instruction_->has_sib()) {
    return InvalidArgumentError(
        "There is a mismatch in the usage of the ModR/M and SIB bytes.");
  }
  if (!specification_requires_modrm_byte) {
    return OkStatus();
  }
  const ModRm& modrm = decoded_instruction_->modrm();
  if (specification_->modrm_usage() ==
          EncodingSpecification::OPCODE_EXTENSION_IN_MODRM &&
      specification_->modrm_opcode_extension() != modrm.register_operand()) {
    return InvalidArgumentError(
        "There is a mismatch in the use of opcode extension in the ModR/M "
        "byte.");
  }

  const bool requires_sib = ModRmRequiresSib(modrm);
  if (requires_sib != decoded_instruction_->has_sib()) {
    return InvalidArgumentError("The presence of the SIB byte is not correct.");
  }
  return OkStatus();
}

// Composes a byte from a two-bit and two three-bit values from parameters:
//   7                                                            0
// +---------------+-----------------------+------------------------+
// | two_bit_value | first_three_bit_value | second_three_bit_value |
// +---------------+-----------------------+------------------------+
//
// This same structure is used by the ModR/M and the SIB bytes. See
// http://wiki.osdev.org/X86-64_Instruction_Encoding#ModR.2FM_and_SIB_bytes for
// more details on the encoding of these bytes.
inline uint8_t Compose233BitValues(uint8_t two_bit_value,
                                   uint8_t first_three_bit_value,
                                   uint8_t second_three_bit_value) {
  return ((two_bit_value & 3) << 6) | ((first_three_bit_value & 7) << 3) |
         (second_three_bit_value & 7);
}

void InstructionEncoder::EncodeModRMIfNeeded() {
  const bool specification_requires_modrm_byte =
      specification_->modrm_usage() != EncodingSpecification::NO_MODRM_USAGE;
  if (!specification_requires_modrm_byte) {
    return;
  }
  const ModRm& modrm = decoded_instruction_->modrm();
  EmitByte(Compose233BitValues(modrm.addressing_mode(),
                               modrm.register_operand(), modrm.rm_operand()));

  const Sib& sib = decoded_instruction_->sib();
  const bool requires_sib = ModRmRequiresSib(modrm);
  if (requires_sib) {
    EmitByte(Compose233BitValues(sib.scale(), sib.index(), sib.base()));
  }
  const int num_displacement_bytes = NumModRmDisplacementBytes(modrm, sib);
  // NOTE(ondrasej): The displacement is always a signed value, so we want to be
  // sure that we get the correct signed conversion...
  // TODO(ondrasej): We might want to check that the bounds are correct.
  switch (num_displacement_bytes) {
    case 0:
      break;
    case 1:
      EmitByte(static_cast<int8_t>(modrm.address_displacement()));
      break;
    case 4:
      EmitDWord(static_cast<int32_t>(modrm.address_displacement()));
      break;
    default:
      LOG(DFATAL) << "Unexpected displacement size: " << num_displacement_bytes;
  }
}

Status InstructionEncoder::ValidateImmediateValues() const {
  if (specification_->immediate_value_bytes_size() !=
      decoded_instruction_->immediate_value_size()) {
    return InvalidArgumentError(absl::StrCat(
        "The number of immediate values in the specification and in the "
        "instruction data is different: ",
        specification_->immediate_value_bytes_size(), " vs ",
        decoded_instruction_->immediate_value_size()));
  }
  for (int i = 0; i < specification_->immediate_value_bytes_size(); ++i) {
    const int expected_size = specification_->immediate_value_bytes(i);
    const std::string& immediate_value =
        decoded_instruction_->immediate_value(i);
    if (expected_size != immediate_value.size()) {
      return InvalidArgumentError(
          absl::StrCat("Unexpected size of immediate value: ", expected_size,
                       " vs ", immediate_value.size()));
    }
  }

  if (specification_->code_offset_bytes() !=
      decoded_instruction_->code_offset().size()) {
    return InvalidArgumentError(
        absl::StrCat("Unexpected size of the code offset: ",
                     specification_->code_offset_bytes(), " vs ",
                     decoded_instruction_->immediate_value().size()));
  }
  return OkStatus();
}

void InstructionEncoder::EncodeImmediateValues() {
  for (const std::string& immediate_value :
       decoded_instruction_->immediate_value()) {
    EmitString(immediate_value);
  }
  EmitString(decoded_instruction_->code_offset());
}

}  // namespace

// TODO(ondrasej): We might want a separate function that does just the
// validation.
StatusOr<std::vector<uint8_t>> EncodeInstruction(
    const EncodingSpecification& specification,
    const DecodedInstruction& decoded_instruction) {
  InstructionEncoder encoder(&specification, &decoded_instruction);
  RETURN_IF_ERROR(encoder.Validate());
  return encoder.Encode();
}

}  // namespace x86
}  // namespace exegesis
