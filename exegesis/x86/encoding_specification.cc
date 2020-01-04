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

// Contains implementation of the Intel instruction encoding specification
// language.

#include "exegesis/x86/encoding_specification.h"

#include <cstdint>
#include <functional>
#include <iterator>
#include <string>
#include <utility>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "exegesis/proto/x86/instruction_encoding.pb.h"
#include "exegesis/x86/instruction_encoding_constants.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"
#include "util/task/status_macros.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;
using ::exegesis::util::StatusOr;

// The parser for the instruction encoding specification language used in the
// Intel manuals.
class EncodingSpecificationParser {
 public:
  // Initializes the instruction in a safe way. Call ParseFromString to fill the
  // structure with useful data.
  EncodingSpecificationParser();

  // Disallow copy and assign.
  EncodingSpecificationParser(const EncodingSpecificationParser&) = delete;
  EncodingSpecificationParser& operator=(const EncodingSpecificationParser&) =
      delete;

  // Parses the instruction data from string.
  StatusOr<EncodingSpecification> ParseFromString(
      absl::string_view specification);

 private:
  // Resets the instruction to the initial state.
  void Reset();

  // Methods for parsing the prefixes of the instructions. There are two
  // separate methods - one parses VEX prefixes and the other parses the legacy
  // prefixes. Upon success, both methods advance 'specification' to the first
  // non-prefix byte. If the methods return a failure, the state of
  // 'specification' is undefined.
  Status ParseLegacyPrefixes(absl::string_view* specification);
  Status ParseVexOrEvexPrefix(absl::string_view* specification);

  // Parses the opcode of the instruction and its suffixes. Returns OK if the
  // opcode and the suffixes were parsed correctly, and if the specification
  // did not contain any additional data.
  // Expects that all prefixes were already consumed.
  Status ParseOpcodeAndSuffixes(absl::string_view specification);

  // The current state of the parser.
  EncodingSpecification specification_;

  // Hash maps mapping tokens of the instruction encoding specification language
  // to enum values of the encoding protos.
  const absl::flat_hash_map<std::string, VexOperandUsage>
      vex_operand_usage_tokens_;
  const absl::flat_hash_map<std::string, VexVectorSize> vector_size_tokens_;
  const absl::flat_hash_map<std::string, VexEncoding::MandatoryPrefix>
      mandatory_prefix_tokens_;
  const absl::flat_hash_map<std::string,
                            VexPrefixEncodingSpecification::VexWUsage>
      vex_w_usage_tokens_;
  const absl::flat_hash_map<uint32_t, VexEncoding::MapSelect>
      map_select_tokens_;
};

// Definitions of maps from tokens to the enum values used in the instruction
// encoding specification proto. These values are used to initialize the hash
// maps used in the parser object.
const std::pair<const char*, VexOperandUsage> kVexOperandUsageTokens[] = {
    {"", UNDEFINED_VEX_OPERAND_USAGE},
    {"NDS", VEX_OPERAND_IS_FIRST_SOURCE_REGISTER},
    {"NDD", VEX_OPERAND_IS_DESTINATION_REGISTER},
    {"DDS", VEX_OPERAND_IS_SECOND_SOURCE_REGISTER}};
const std::pair<const char*, VexVectorSize> kVectorSizeTokens[] = {
    {"LZ", VEX_VECTOR_SIZE_BIT_IS_ZERO},
    // The two following are undocumented. We assume that L0 is equivalent
    // to LZ, and extend the semantics to L1 naturally to mean "L must be
    // 1".
    {"L0", VEX_VECTOR_SIZE_BIT_IS_ZERO},
    {"L1", VEX_VECTOR_SIZE_BIT_IS_ONE},
    {"128", VEX_VECTOR_SIZE_128_BIT},
    {"256", VEX_VECTOR_SIZE_256_BIT},
    {"512", VEX_VECTOR_SIZE_512_BIT},
    {"LIG", VEX_VECTOR_SIZE_IS_IGNORED},
    {"LIG.128", VEX_VECTOR_SIZE_128_BIT}};
const std::pair<const char*, VexEncoding::MandatoryPrefix>
    kMandatoryPrefixTokens[] = {
        {"", VexEncoding::NO_MANDATORY_PREFIX},
        {"66", VexEncoding::MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE},
        {"F2", VexEncoding::MANDATORY_PREFIX_REPNE},
        {"F3", VexEncoding::MANDATORY_PREFIX_REPE}};
const std::pair<const char*, VexPrefixEncodingSpecification::VexWUsage>
    kVexWUsageTokens[] = {
        {"", VexPrefixEncodingSpecification::VEX_W_IS_IGNORED},
        {"W0", VexPrefixEncodingSpecification::VEX_W_IS_ZERO},
        {"W1", VexPrefixEncodingSpecification::VEX_W_IS_ONE},
        {"WIG", VexPrefixEncodingSpecification::VEX_W_IS_IGNORED}};
const std::pair<uint32_t, VexEncoding::MapSelect> kMapSelectTokens[] = {
    {0x0f, VexEncoding::MAP_SELECT_0F},
    {0x0f3a, VexEncoding::MAP_SELECT_0F3A},
    {0x0f38, VexEncoding::MAP_SELECT_0F38}};

EncodingSpecificationParser::EncodingSpecificationParser()
    : vex_operand_usage_tokens_(std::begin(kVexOperandUsageTokens),
                                std::end(kVexOperandUsageTokens)),
      vector_size_tokens_(std::begin(kVectorSizeTokens),
                          std::end(kVectorSizeTokens)),
      mandatory_prefix_tokens_(std::begin(kMandatoryPrefixTokens),
                               std::end(kMandatoryPrefixTokens)),
      vex_w_usage_tokens_(std::begin(kVexWUsageTokens),
                          std::end(kVexWUsageTokens)),
      map_select_tokens_(std::begin(kMapSelectTokens),
                         std::end(kMapSelectTokens)) {}

StatusOr<EncodingSpecification> EncodingSpecificationParser::ParseFromString(
    absl::string_view specification) {
  specification_.Clear();
  if (absl::StartsWith(specification, "VEX.") ||
      absl::StartsWith(specification, "EVEX")) {
    RETURN_IF_ERROR(ParseVexOrEvexPrefix(&specification));
  } else {
    RETURN_IF_ERROR(ParseLegacyPrefixes(&specification));
  }
  RETURN_IF_ERROR(ParseOpcodeAndSuffixes(specification));
  return std::move(specification_);
}

Status EncodingSpecificationParser::ParseLegacyPrefixes(
    absl::string_view* specification) {
  CHECK(specification != nullptr);
  // A regexp for parsing the legacy prefixes. For more details on the format,
  // see Intel 64 and IA-32 Architectures Software Developer's Manual, Volume 2:
  // Instruction Set Reference, A-Z, Section 3.1.1.1 (page 3.2).
  // The parser matches all the possible prefixes and removes them from the
  // specification. When the string does not match anymore, it assues that this
  // is the beginning of the opcode and switches to parsing the opcode.
  // NOTE(ondrasej): kLegacyPrefixRegex must be static - removing it causes an
  // internal compiler error on gcc 4.8.
  static constexpr char kLegacyPrefixRegex[] =
      // Optional whitespace before the prefix.
      " *(?:"
      // The meta-prefix stating that the instruction does not allow any
      // additional prefixes. There are two options: NP means that neither of
      // the 66/F2/F3 prefixes is allowed, while NFx means that only the F2/F3
      // prefixes are not allowed.
      "(NP|NFx)|"
      // The operand size override prefix.
      "(66)|"
      // THe address size override prefix.
      "(67)|"
      // The REPNE prefix.
      "(F2)|"
      // The REPE prefix.
      "(F3)|"
      // The REX prefix. The manual uses this prefix in three forms:
      // * REX.W means that the REX.W bit must be set to 1.
      // * REX.R means that the REX.R bit must be set to 1. This is used only in
      //   cases that are consistent with the general rules for the use of the
      //   REX.R bit, and thus the specification can be safely ignored.
      // * REX which probably means that the instruction may be used with the
      //   extended registers (R8-R15). However, this is the case for all legacy
      //   instructions using general purpose registers, and the use of this
      //   prefix specification is not very consistent.
      // In practice, we're interested only in the REX.W specification, because
      // the others follow from the general rules for the use of the REX prefix
      // on legacy instructions. The parser ignores the REX and REX.R prefix
      // specifications.
      "(REX(?:\\.(?:R|W))?))"
      // Consume also any whitespace at the end.
      "(?: *\\+ *)?";
  static constexpr char kRexWPrefix[] = "REX.W";
  static constexpr char kNoAdditionalPrefixes[] = "NP";
  static constexpr char kNoFxPrefixes[] = "NFx";
  static const LazyRE2 legacy_prefix_parser = {kLegacyPrefixRegex};
  std::string no_additional_prefixes;
  std::string operand_size_override_prefix;
  std::string address_size_override_prefix;
  std::string repne_prefix;
  std::string repe_prefix;
  std::string rex_prefix;
  bool has_no_additional_prefixes = false;
  bool has_no_fx_prefixes = false;
  bool has_mandatory_address_size_override_prefix = false;
  bool has_mandatory_operand_size_override_prefix = false;
  bool has_mandatory_repe_prefix = false;
  bool has_mandatory_repne_prefix = false;
  bool has_mandatory_rex_prefix = false;
  while (RE2::Consume(specification, *legacy_prefix_parser,
                      &no_additional_prefixes, &operand_size_override_prefix,
                      &address_size_override_prefix, &repne_prefix,
                      &repe_prefix, &rex_prefix)) {
    has_no_additional_prefixes |=
        no_additional_prefixes == kNoAdditionalPrefixes;
    has_no_fx_prefixes |= no_additional_prefixes == kNoFxPrefixes;
    has_mandatory_operand_size_override_prefix |=
        !operand_size_override_prefix.empty();
    has_mandatory_address_size_override_prefix |=
        !address_size_override_prefix.empty();
    has_mandatory_repe_prefix |= !repe_prefix.empty();
    has_mandatory_repne_prefix |= !repne_prefix.empty();
    has_mandatory_rex_prefix |= rex_prefix == kRexWPrefix;
  }
  // Note that just calling mutable_legacy_prefixes will create an empty
  // legacy_prefixes field of the specification. This is desirable, because it
  // lets us make a difference between legacy instructions and VEX-encoded
  // instructions.
  LegacyPrefixEncodingSpecification* const legacy_prefixes =
      specification_.mutable_legacy_prefixes();
  if (has_no_additional_prefixes) {
    // We simply set all prefixes to PREFIX_IS_NOT_PERMITTED at the beginning,
    // and then overwrite them with any prefixes that did appear in the encoding
    // specification.
    legacy_prefixes->set_operand_size_override_prefix(
        LegacyEncoding::PREFIX_IS_NOT_PERMITTED);
  }
  // TODO(user): Add support for the REPE/REPNE prefixes when they are
  // converted to use LegacyEncoding::PrefixUsage. The state of these prefixes
  // must also take into account the value of has_no_fx_prefixes.
  if (has_mandatory_operand_size_override_prefix) {
    legacy_prefixes->set_operand_size_override_prefix(
        LegacyEncoding::PREFIX_IS_REQUIRED);
  }
  legacy_prefixes->set_has_mandatory_address_size_override_prefix(
      has_mandatory_address_size_override_prefix);
  legacy_prefixes->set_has_mandatory_repe_prefix(has_mandatory_repe_prefix);
  legacy_prefixes->set_has_mandatory_repne_prefix(has_mandatory_repne_prefix);
  if (has_mandatory_rex_prefix) {
    legacy_prefixes->set_rex_w_prefix(LegacyEncoding::PREFIX_IS_REQUIRED);
  }
  return OkStatus();
}

Status EncodingSpecificationParser::ParseVexOrEvexPrefix(
    absl::string_view* specification) {
  CHECK(specification != nullptr);
  // A regexp for parsing the VEX prefix specification. For more details on the
  // format see Intel 64 and IA-32 Architectures Software Developer's Manual,
  // Volume 2: Instruction Set Reference, A-Z, Section 3.1.1.2 (page 3.3).
  // NOTE(ondrasej): Note that some of the fields do not affect the size of the
  // instruction encoding, so we just check that they have a valid value, but we
  // do not extract this value out of the regexp.
  // NOTE(ondrasej): kVexPrefixRegexp must be static - removing it causes an
  // internal compiler error on gcc 4.8.
  static constexpr char kVexPrefixRegexp[] =
      "(E?VEX)"                    // The VEX prefix.
      "(?: *\\. *(NDS|NDD|DDS))?"  // The directionality of the operand(s).
      "(?: *\\. *(LIG|LZ|L0|L1|LIG\\.128|128|256|512))?"  // Interpretation of
                                                          // the VEX and EVEX
                                                          // L/L' bits.
      "(?: *\\. *(66|F2|F3))?"     // The mandatory prefixes.
      " *\\. *(0F|0F3A|0F38)"      // The opcode prefix based on VEX.mmmmm.
      "(?: *\\. *(W0|W1|WIG))? ";  // Interpretation of the VEX.W bit.
  VexPrefixEncodingSpecification* const vex_prefix =
      specification_.mutable_vex_prefix();

  static const LazyRE2 vex_prefix_parser = {kVexPrefixRegexp};
  std::string prefix_type_str;
  std::string vex_operand_directionality;
  std::string vex_l_usage_str;
  std::string mandatory_prefix_str;
  uint32_t opcode_map;
  std::string vex_w_str;
  if (!RE2::Consume(specification, *vex_prefix_parser, &prefix_type_str,
                    &vex_operand_directionality, &vex_l_usage_str,
                    &mandatory_prefix_str, RE2::Hex(&opcode_map), &vex_w_str)) {
    return InvalidArgumentError(
        absl::StrCat("Could not parse the VEX prefix: '", *specification, "'"));
  }

  // Parse the fields of the VEX prefix specification.
  // Note that we use the regexp to filter out invalid values of these fields,
  // so FindOrDie never dies unless the regexp and the token definitions at the
  // top of this file are out of sync.
  const VexPrefixType prefix_type =
      prefix_type_str == "EVEX" ? EVEX_PREFIX : VEX_PREFIX;
  const VexVectorSize vector_size =
      gtl::FindOrDie(vector_size_tokens_, vex_l_usage_str);
  vex_prefix->set_prefix_type(prefix_type);
  vex_prefix->set_vex_operand_usage(
      gtl::FindOrDie(vex_operand_usage_tokens_, vex_operand_directionality));
  vex_prefix->set_vector_size(vector_size);
  if (vector_size == VEX_VECTOR_SIZE_512_BIT && prefix_type != EVEX_PREFIX) {
    return InvalidArgumentError(
        "The 512 bit vector size can be used only in an EVEX prefix");
  }
  vex_prefix->set_mandatory_prefix(
      gtl::FindOrDie(mandatory_prefix_tokens_, mandatory_prefix_str));
  vex_prefix->set_vex_w_usage(gtl::FindOrDie(vex_w_usage_tokens_, vex_w_str));
  vex_prefix->set_map_select(gtl::FindOrDie(map_select_tokens_, opcode_map));

  // NOTE(ondrasej): The string specification of the opcode map is an equivalent
  // of opcode prefixes in the legacy encoding, and not the actual value used in
  // the VEX.mmmmm bits. This works to our advantage here, because we can simply
  // add it to the opcode.
  specification_.set_opcode(opcode_map);

  return OkStatus();
}

Status EncodingSpecificationParser::ParseOpcodeAndSuffixes(
    absl::string_view specification) {
  VLOG(1) << "Parsing opcode and suffixes: " << specification;
  // We've already dealt with all possible prefixes. The rest are either
  // 1. a sequence of bytes (separated by space) of the opcode, in uppercase
  //    hex format, or
  // 2. information about the ModR/M bytes and immediate values.
  // The ModR/M info and immediate values have a fixed position, but
  // both of these are easy to tell from each other, so we can just parse them
  // in a for loop.
  static const LazyRE2 opcode_byte_parser = {
      " *([0-9A-F]{2})(?: *\\+ *(i|rb|rw|rd|ro))?"};
  int opcode_byte = 0;
  int num_opcode_bytes = 0;
  std::string opcode_encoded_register;
  uint32_t opcode = specification_.opcode();
  while (RE2::Consume(&specification, *opcode_byte_parser,
                      RE2::Hex(&opcode_byte), &opcode_encoded_register)) {
    ++num_opcode_bytes;
    opcode = (opcode << 8) | opcode_byte;
    if (!opcode_encoded_register.empty()) {
      if (opcode_encoded_register[0] == 'i') {
        specification_.set_operand_in_opcode(
            EncodingSpecification::FP_STACK_REGISTER_IN_OPCODE);
      } else {
        specification_.set_operand_in_opcode(
            EncodingSpecification::GENERAL_PURPOSE_REGISTER_IN_OPCODE);
      }
    }
  }
  specification_.set_opcode(opcode);
  if (num_opcode_bytes == 0) {
    return InvalidArgumentError("The instruction did not have an opcode byte.");
  }
  if (specification_.has_vex_prefix() && num_opcode_bytes != 1) {
    return InvalidArgumentError(
        "Unexpected number of opcode bytes in a VEX-encoded instruction.");
  }

  if (specification.empty()) {
    // There is neither ModR/M byte nor an immediate value.
    return OkStatus();
  }

  VLOG(1) << "Parsing suffixes: " << specification;
  // The variable that receives the value must be string. If we used chars
  // directly, the matching would fail because RE2 doesn't know how to convert
  // an empty matching group to a char.
  std::string is4_suffix_str;
  std::string modrm_suffix_str;
  std::string vsib_suffix_str;
  std::string immediate_value_size_str;
  std::string code_offset_size_str;
  // Notes on the suffix regexp:
  // * There might be a m64/m128 suffix that is not explained in the Intel
  //   manuals, but that most likely means that the operand in the ModR/M byte
  //   must be a memory operand. In practice, I've never seen them without
  //   another ModR/M suffix, so we just ignore them here.
  static const LazyRE2 modrm_and_imm_parser = {
      " *(?:"
      "(\\/is4)|"     // is4
      "i([bwdo])|"    // immediate
      "/0?([r0-9])|"  // modrm
      "(/vsib)|"      // vsib
      "(?:m(?:64|128|256))|"
      "c([bwdpot]))"};  // code offset size
  while (RE2::Consume(&specification, *modrm_and_imm_parser, &is4_suffix_str,
                      &immediate_value_size_str, &modrm_suffix_str,
                      &vsib_suffix_str, &code_offset_size_str)) {
    VLOG(1) << "modrm_suffix = " << modrm_suffix_str;
    VLOG(1) << "immediate_value_size_str = " << immediate_value_size_str;
    VLOG(1) << "code_offset_size = " << code_offset_size_str;
    // Only one of the following if statements will actually be evaluated,
    // because RE2 clears both strings at the beginning of the call to Consume.
    if (!modrm_suffix_str.empty()) {
      // If there was a ModR/M specifier, parse the usage of the MODRM.reg
      // value.
      const char modrm_suffix = modrm_suffix_str[0];
      if (modrm_suffix == 'r') {
        specification_.set_modrm_usage(EncodingSpecification::FULL_MODRM);
      } else {
        specification_.set_modrm_usage(
            EncodingSpecification::OPCODE_EXTENSION_IN_MODRM);
        specification_.set_modrm_opcode_extension(modrm_suffix - '0');
      }
    } else if (!immediate_value_size_str.empty()) {
      // If there was an immediate value specifier, parse the size of the
      // immediate value.
      switch (immediate_value_size_str[0]) {
        case 'b':
          specification_.add_immediate_value_bytes(1);
          break;
        case 'w':
          specification_.add_immediate_value_bytes(2);
          break;
        case 'd':
          specification_.add_immediate_value_bytes(4);
          break;
        case 'o':
          specification_.add_immediate_value_bytes(8);
          break;
        default:
          return InvalidArgumentError(absl::StrCat(
              "Invalid immediate value size: ", immediate_value_size_str));
      }
    } else if (!code_offset_size_str.empty()) {
      switch (code_offset_size_str[0]) {
        case 'b':
          specification_.set_code_offset_bytes(1);
          break;
        case 'w':
          specification_.set_code_offset_bytes(2);
          break;
        case 'd':
          specification_.set_code_offset_bytes(4);
          break;
        case 'p':
          specification_.set_code_offset_bytes(6);
          break;
        case 'o':
          specification_.set_code_offset_bytes(8);
          break;
        case 't':
          specification_.set_code_offset_bytes(10);
          break;
        default:
          return InvalidArgumentError(absl::StrCat("Invalid code offset size: ",
                                                   immediate_value_size_str));
      }
    } else if (!is4_suffix_str.empty()) {
      CHECK_EQ("/is4", is4_suffix_str);
      if (!specification_.has_vex_prefix()) {
        return InvalidArgumentError(
            "The VEX operand suffix /is4 is specified for an instruction that "
            "does not use the VEX prefix.");
      }
      specification_.mutable_vex_prefix()->set_has_vex_operand_suffix(true);
    } else if (!vsib_suffix_str.empty()) {
      CHECK_EQ("/vsib", vsib_suffix_str);
      if (!specification_.has_vex_prefix()) {
        return InvalidArgumentError(
            "The VEX operand suffix /vsib is specified for an instruction that "
            "does not use the VEX prefix.");
      }
      specification_.mutable_vex_prefix()->set_vsib_usage(
          VexPrefixEncodingSpecification::VSIB_USED);
    }
  }

  // VSIB implies that ModRM is used: ModRM.rm has to be 0b100, and ModRM.reg
  // can be used to encode either an extra operand or an opcode extension.
  if (specification_.vex_prefix().vsib_usage() ==
          VexPrefixEncodingSpecification::VSIB_USED &&
      specification_.modrm_usage() == EncodingSpecification::NO_MODRM_USAGE) {
    specification_.set_modrm_usage(EncodingSpecification::FULL_MODRM);
  }

  specification = absl::StripAsciiWhitespace(specification);
  return specification.empty() ? OkStatus()
                               : InvalidArgumentError(absl::StrCat(
                                     "The specification was not fully parsed: ",
                                     std::string(specification)));
}

std::string GenerateLegacyPrefixEncodingSpec(
    const LegacyPrefixEncodingSpecification& prefixes) {
  std::string raw_encoding_spec;

  if (prefixes.rex_w_prefix() == LegacyEncoding::PREFIX_IS_REQUIRED) {
    raw_encoding_spec.append("REX.W + ");
  }
  if (prefixes.has_mandatory_repne_prefix()) {
    absl::StrAppendFormat(&raw_encoding_spec, "%02X ", kRepNePrefixByte);
  }
  if (prefixes.has_mandatory_repe_prefix()) {
    absl::StrAppendFormat(&raw_encoding_spec, "%02X ", kRepPrefixByte);
  }
  if (prefixes.has_mandatory_address_size_override_prefix()) {
    absl::StrAppendFormat(&raw_encoding_spec, "%02X ",
                          kAddressSizeOverrideByte);
  }
  if (prefixes.operand_size_override_prefix() ==
      LegacyEncoding::PREFIX_IS_REQUIRED) {
    absl::StrAppendFormat(&raw_encoding_spec, "%02X ",
                          kOperandSizeOverrideByte);
  }

  return raw_encoding_spec;
}

std::string GenerateVexPrefixEncodingSpec(
    const VexPrefixEncodingSpecification& vex_prefix, uint32_t* opcode) {
  std::string raw_encoding_spec;

  switch (vex_prefix.prefix_type()) {
    case VEX_PREFIX:
      raw_encoding_spec.append("VEX");
      break;
    case EVEX_PREFIX:
      raw_encoding_spec.append("EVEX");
      break;
    default:
      LOG(FATAL) << "Unreachable";
  }

  switch (vex_prefix.vector_size()) {
    case VEX_VECTOR_SIZE_IS_IGNORED:
      raw_encoding_spec.append(".LIG");
      break;
    case VEX_VECTOR_SIZE_BIT_IS_ZERO:
      raw_encoding_spec.append(".L0");
      break;
    case VEX_VECTOR_SIZE_BIT_IS_ONE:
      raw_encoding_spec.append(".L1");
      break;
    case VEX_VECTOR_SIZE_128_BIT:
      raw_encoding_spec.append(".128");
      break;
    case VEX_VECTOR_SIZE_256_BIT:
      raw_encoding_spec.append(".256");
      break;
    case VEX_VECTOR_SIZE_512_BIT:
      raw_encoding_spec.append(".512");
      break;
    default:
      LOG(FATAL) << "invalid vector_size value";
  }

  switch (vex_prefix.mandatory_prefix()) {
    case VexEncoding::MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE:
      raw_encoding_spec.append(".66");
      break;
    case VexEncoding::MANDATORY_PREFIX_REPNE:
      raw_encoding_spec.append(".F2");
      break;
    case VexEncoding::MANDATORY_PREFIX_REPE:
      raw_encoding_spec.append(".F3");
      break;
    case VexEncoding::NO_MANDATORY_PREFIX:
      break;
    default:
      LOG(FATAL) << "invalid mandatory_prefix value";
  }

  // These bytes appear in the opcode as well. We strip them here so we
  // don't write them into the raw encoding spec later.
  switch (vex_prefix.map_select()) {
    case VexEncoding::MAP_SELECT_0F:
      raw_encoding_spec.append(".0F");
      CHECK_EQ(0x0F, *opcode >> 8);
      *opcode &= 0xFF;
      break;
    case VexEncoding::MAP_SELECT_0F38:
      raw_encoding_spec.append(".0F38");
      CHECK_EQ(0x0F38, *opcode >> 8);
      *opcode &= 0xFF;
      break;
    case VexEncoding::MAP_SELECT_0F3A:
      raw_encoding_spec.append(".0F3A");
      CHECK_EQ(0x0F3A, *opcode >> 8);
      *opcode &= 0xFF;
      break;
    default:
      LOG(FATAL) << "invalid map_select value";
  }

  switch (vex_prefix.vex_w_usage()) {
    case VexPrefixEncodingSpecification::VEX_W_IS_ZERO:
      raw_encoding_spec.append(".W0");
      break;
    case VexPrefixEncodingSpecification::VEX_W_IS_ONE:
      raw_encoding_spec.append(".W1");
      break;
    case VexPrefixEncodingSpecification::VEX_W_IS_IGNORED:
      raw_encoding_spec.append(".WIG");
      break;
    default:
      LOG(FATAL) << "invalid vex_w_usage value";
  }

  raw_encoding_spec.push_back(' ');
  return raw_encoding_spec;
}

}  // namespace

StatusOr<EncodingSpecification> ParseEncodingSpecification(
    const std::string& specification) {
  EncodingSpecificationParser parser;
  return parser.ParseFromString(specification);
}

std::string GenerateEncodingSpec(const InstructionFormat& instruction,
                                 const EncodingSpecification& encoding_spec) {
  std::string raw_encoding_spec;

  uint32_t opcode = encoding_spec.opcode();

  switch (encoding_spec.prefix_case()) {
    case EncodingSpecification::PREFIX_NOT_SET:
      break;
    case EncodingSpecification::kLegacyPrefixes: {
      raw_encoding_spec =
          GenerateLegacyPrefixEncodingSpec(encoding_spec.legacy_prefixes());
      break;
    }
    case EncodingSpecification::kVexPrefix: {
      // Pass in the opcode so it can strip any leading opcode bytes that are
      // implied by map_select bits in the prefix.
      raw_encoding_spec =
          GenerateVexPrefixEncodingSpec(encoding_spec.vex_prefix(), &opcode);
      break;
    }
  }

  // Opcodes should be a maximum of 3 bytes long.
  CHECK_EQ(0, opcode >> 24);

  // Default to length 1, for when opcode = 0.
  int opcode_length = 1;
  for (int n = 2; n >= 0; n--) {
    uint8_t b = (opcode >> (n * 8)) & 0xFF;
    if (b != 0) {
      opcode_length = n + 1;
      break;
    }
  }

  for (int n = opcode_length - 1; n >= 0; n--) {
    uint8_t b = (opcode >> (n * 8)) & 0xFF;
    if (!raw_encoding_spec.empty() && raw_encoding_spec.back() != ' ') {
      raw_encoding_spec.push_back(' ');
    }
    absl::StrAppendFormat(&raw_encoding_spec, "%02X", b);
  }

  switch (encoding_spec.operand_in_opcode()) {
    case EncodingSpecification::NO_OPERAND_IN_OPCODE:
      break;
    case EncodingSpecification::GENERAL_PURPOSE_REGISTER_IN_OPCODE: {
      const auto operand_in_opcode = absl::c_find_if(
          instruction.operands(), [](const InstructionOperand& operand) {
            return operand.encoding() == InstructionOperand::OPCODE_ENCODING;
          });
      CHECK(operand_in_opcode != instruction.operands().end());

      const int width = operand_in_opcode->data_type().bit_width();
      switch (width) {
        case 8:
          raw_encoding_spec.append(" +rb");
          break;
        case 16:
          raw_encoding_spec.append(" +rw");
          break;
        case 32:
          raw_encoding_spec.append(" +rd");
          break;
        case 64:
          raw_encoding_spec.append(" +ro");
          break;
        default:
          LOG(FATAL) << "Unknown width: " << width;
      }
      break;
    }
    case EncodingSpecification::FP_STACK_REGISTER_IN_OPCODE:
      raw_encoding_spec.append(" +i");
      break;
    default:
      LOG(FATAL) << "invalid operand_in_opcode value";
  }

  if (encoding_spec.modrm_usage() ==
      EncodingSpecification::OPCODE_EXTENSION_IN_MODRM) {
    absl::StrAppend(&raw_encoding_spec, " /",
                    encoding_spec.modrm_opcode_extension());
  } else if (encoding_spec.modrm_usage() == EncodingSpecification::FULL_MODRM) {
    raw_encoding_spec.append(" /r");
  }

  for (int imm_size : encoding_spec.immediate_value_bytes()) {
    static const auto* const kImmediateValues =
        new absl::flat_hash_map<int, const char*>{
            {1, " ib"}, {2, " iw"}, {4, " id"}, {8, " io"}};
    raw_encoding_spec.append(gtl::FindOrDie(*kImmediateValues, imm_size));
  }

  return raw_encoding_spec;
}

InstructionOperandEncodingMultiset GetAvailableEncodings(
    const EncodingSpecification& encoding_specification) {
  InstructionOperandEncodingMultiset available_encodings;
  // If the instruction uses ModR/M byte, the operands might be encoded using
  // some of the ModR/M byte fields.
  switch (encoding_specification.modrm_usage()) {
    case EncodingSpecification::FULL_MODRM:
      available_encodings.insert(InstructionOperand::MODRM_REG_ENCODING);
      available_encodings.insert(InstructionOperand::MODRM_RM_ENCODING);
      break;
    case EncodingSpecification::OPCODE_EXTENSION_IN_MODRM:
      available_encodings.insert(InstructionOperand::MODRM_RM_ENCODING);
      break;
    default:
      break;
  }
  // If the instruction uses opcode bits to encode the operands, the operand
  // might be encoded using the opcode bits.
  if (encoding_specification.operand_in_opcode() !=
      EncodingSpecification::NO_OPERAND_IN_OPCODE) {
    available_encodings.insert(InstructionOperand::OPCODE_ENCODING);
  }
  // If the instruction uses the VEX prefix, the operands might be encoded in
  // the VEX.vvvv bits.
  if (encoding_specification.has_vex_prefix()) {
    const VexPrefixEncodingSpecification& vex_prefix =
        encoding_specification.vex_prefix();
    if (vex_prefix.vex_operand_usage() != VEX_OPERAND_IS_NOT_USED) {
      available_encodings.insert(InstructionOperand::VEX_V_ENCODING);
    }
    if (vex_prefix.has_vex_operand_suffix()) {
      available_encodings.insert(InstructionOperand::VEX_SUFFIX_ENCODING);
    }
    if (vex_prefix.vsib_usage() !=
        VexPrefixEncodingSpecification::VSIB_UNUSED) {
      available_encodings.insert(InstructionOperand::VSIB_ENCODING);
      // See comment in ParseOpcodeAndSuffixes().
      CHECK_NE(encoding_specification.modrm_usage(),
               EncodingSpecification::NO_MODRM_USAGE)
          << encoding_specification.DebugString();
      // VSIB requires ModRM.rm to be 0b100, so it cannot be used to encoed an
      // operand.
      available_encodings.erase(InstructionOperand::MODRM_RM_ENCODING);
    }
  }
  // Add implicit encodings for implicit operands.
  const int num_implicit_operands =
      encoding_specification.immediate_value_bytes_size() +
      (encoding_specification.code_offset_bytes() > 0 ? 1 : 0);
  for (int i = 0; i < num_implicit_operands; ++i) {
    available_encodings.insert(InstructionOperand::IMMEDIATE_VALUE_ENCODING);
  }
  return available_encodings;
}

}  // namespace x86
}  // namespace exegesis
