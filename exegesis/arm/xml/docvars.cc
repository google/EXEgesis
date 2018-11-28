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

#include "exegesis/arm/xml/docvars.h"

#include <string>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "exegesis/arm/xml/docvars.pb.h"
#include "exegesis/util/xml/xml_util.h"
#include "glog/logging.h"
#include "src/google/protobuf/descriptor.h"
#include "src/google/protobuf/message.h"
#include "src/google/protobuf/util/message_differencer.h"
#include "tinyxml2.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace arm {
namespace xml {

namespace {

using ::exegesis::arm::xml::dv::DocVars;
using ::exegesis::util::FailedPreconditionError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;
using ::exegesis::util::StatusOr;
using ::exegesis::util::UnimplementedError;
using ::exegesis::xml::FindChildren;
using ::exegesis::xml::ReadAttribute;
using ::google::protobuf::util::MessageDifferencer;
using ::tinyxml2::XMLElement;
using ::tinyxml2::XMLNode;

using KeyString = std::string;
using KeyProtoId = int;
using ValueString = std::string;
using ValueEnum = int;
using ValueStringToEnumMapping = absl::flat_hash_map<ValueString, ValueEnum>;

// Extracts all key/value pairs from a given <docvars> XML node into a multimap.
std::unordered_multimap<KeyString, ValueString> ReadDocVars(XMLNode* node) {
  CHECK(node != nullptr);
  std::unordered_multimap<KeyString, ValueString> result;
  for (XMLElement* element : FindChildren(node, "docvar")) {
    const KeyString key = ReadAttribute(element, "key");
    const ValueString value = ReadAttribute(element, "value");
    if (key.empty()) {
      LOG(WARNING) << "Skipping empty docvar key (value='" << value << "')";
      continue;
    }
    result.insert({key, value});
  }
  return result;
}

// Returns the mapping of DocVars key/value pairs to safe proto enum values.
const absl::flat_hash_map<KeyString,
                          std::pair<KeyProtoId, ValueStringToEnumMapping>>&
GetDocVarsEnumMapping() {
  static const auto* const kDocVarsEnumMapping = new absl::flat_hash_map<
      KeyString, std::pair<KeyProtoId, ValueStringToEnumMapping>>({
      {"address-form",
       {DocVars::kAddressFormFieldNumber,
        {
            {"literal", dv::AddressForm::LITERAL},
            {"base-register", dv::AddressForm::BASE_REGISTER},
            {"base-plus-offset", dv::AddressForm::BASE_PLUS_OFFSET},
            {"signed-scaled-offset", dv::AddressForm::SIGNED_SCALED_OFFSET},
            {"unsigned-scaled-offset", dv::AddressForm::UNSIGNED_SCALED_OFFSET},
            {"pre-indexed", dv::AddressForm::PRE_INDEXED},
            {"post-indexed", dv::AddressForm::POST_INDEXED},
        }}},
      {"address-form-reg-type",
       {DocVars::kAddressFormRegTypeFieldNumber,
        {
            {"literal-32-reg", dv::AddressFormRegType::LITERAL_32_REG},
            {"literal-32-fsreg", dv::AddressFormRegType::LITERAL_32_FSREG},
            {"literal-64-reg", dv::AddressFormRegType::LITERAL_64_REG},
            {"literal-64-fsreg", dv::AddressFormRegType::LITERAL_64_FSREG},
            {"literal-128-fsreg", dv::AddressFormRegType::LITERAL_128_FSREG},
            {"base-register-32-reg",
             dv::AddressFormRegType::BASE_REGISTER_32_REG},
            {"base-register-64-reg",
             dv::AddressFormRegType::BASE_REGISTER_64_REG},
            {"base-register-pair-32",
             dv::AddressFormRegType::BASE_REGISTER_PAIR_32},
            {"base-register-pair-64",
             dv::AddressFormRegType::BASE_REGISTER_PAIR_64},
            {"base-plus-offset-8-fsreg",
             dv::AddressFormRegType::BASE_PLUS_OFFSET_8_FSREG},
            {"base-plus-offset-16-fsreg",
             dv::AddressFormRegType::BASE_PLUS_OFFSET_16_FSREG},
            {"base-plus-offset-32-reg",
             dv::AddressFormRegType::BASE_PLUS_OFFSET_32_REG},
            {"base-plus-offset-32-fsreg",
             dv::AddressFormRegType::BASE_PLUS_OFFSET_32_FSREG},
            {"base-plus-offset-64-reg",
             dv::AddressFormRegType::BASE_PLUS_OFFSET_64_REG},
            {"base-plus-offset-64-fsreg",
             dv::AddressFormRegType::BASE_PLUS_OFFSET_64_FSREG},
            {"base-plus-offset-128-fsreg",
             dv::AddressFormRegType::BASE_PLUS_OFFSET_128_FSREG},
            {"unsigned-scaled-offset-8-fsreg",
             dv::AddressFormRegType::UNSIGNED_SCALED_OFFSET_8_FSREG},
            {"unsigned-scaled-offset-16-fsreg",
             dv::AddressFormRegType::UNSIGNED_SCALED_OFFSET_16_FSREG},
            {"unsigned-scaled-offset-32-reg",
             dv::AddressFormRegType::UNSIGNED_SCALED_OFFSET_32_REG},
            {"unsigned-scaled-offset-32-fsreg",
             dv::AddressFormRegType::UNSIGNED_SCALED_OFFSET_32_FSREG},
            {"unsigned-scaled-offset-64-reg",
             dv::AddressFormRegType::UNSIGNED_SCALED_OFFSET_64_REG},
            {"unsigned-scaled-offset-64-fsreg",
             dv::AddressFormRegType::UNSIGNED_SCALED_OFFSET_64_FSREG},
            {"unsigned-scaled-offset-128-fsreg",
             dv::AddressFormRegType::UNSIGNED_SCALED_OFFSET_128_FSREG},
            {"signed-scaled-offset-pair-32",
             dv::AddressFormRegType::SIGNED_SCALED_OFFSET_PAIR_32},
            {"signed-scaled-offset-pair-64",
             dv::AddressFormRegType::SIGNED_SCALED_OFFSET_PAIR_64},
            {"signed-scaled-offset-pair-words",
             dv::AddressFormRegType::SIGNED_SCALED_OFFSET_PAIR_WORDS},
            {"signed-scaled-offset-pair-doublewords",
             dv::AddressFormRegType::SIGNED_SCALED_OFFSET_PAIR_DOUBLEWORDS},
            {"signed-scaled-offset-pair-quadwords",
             dv::AddressFormRegType::SIGNED_SCALED_OFFSET_PAIR_QUADWORDS},
            {"pre-indexed-8-fsreg",
             dv::AddressFormRegType::PRE_INDEXED_8_FSREG},
            {"pre-indexed-16-fsreg",
             dv::AddressFormRegType::PRE_INDEXED_16_FSREG},
            {"pre-indexed-32-reg", dv::AddressFormRegType::PRE_INDEXED_32_REG},
            {"pre-indexed-32-fsreg",
             dv::AddressFormRegType::PRE_INDEXED_32_FSREG},
            {"pre-indexed-64-reg", dv::AddressFormRegType::PRE_INDEXED_64_REG},
            {"pre-indexed-64-fsreg",
             dv::AddressFormRegType::PRE_INDEXED_64_FSREG},
            {"pre-indexed-128-fsreg",
             dv::AddressFormRegType::PRE_INDEXED_128_FSREG},
            {"pre-indexed-pair-32",
             dv::AddressFormRegType::PRE_INDEXED_PAIR_32},
            {"pre-indexed-pair-64",
             dv::AddressFormRegType::PRE_INDEXED_PAIR_64},
            {"pre-indexed-pair-words",
             dv::AddressFormRegType::PRE_INDEXED_PAIR_WORDS},
            {"pre-indexed-pair-doublewords",
             dv::AddressFormRegType::PRE_INDEXED_PAIR_DOUBLEWORDS},
            {"pre-indexed-pair-quadwords",
             dv::AddressFormRegType::PRE_INDEXED_PAIR_QUADWORDS},
            {"post-indexed-8-fsreg",
             dv::AddressFormRegType::POST_INDEXED_8_FSREG},
            {"post-indexed-16-fsreg",
             dv::AddressFormRegType::POST_INDEXED_16_FSREG},
            {"post-indexed-32-reg",
             dv::AddressFormRegType::POST_INDEXED_32_REG},
            {"post-indexed-32-fsreg",
             dv::AddressFormRegType::POST_INDEXED_32_FSREG},
            {"post-indexed-64-reg",
             dv::AddressFormRegType::POST_INDEXED_64_REG},
            {"post-indexed-64-fsreg",
             dv::AddressFormRegType::POST_INDEXED_64_FSREG},
            {"post-indexed-128-fsreg",
             dv::AddressFormRegType::POST_INDEXED_128_FSREG},
            {"post-indexed-pair-32",
             dv::AddressFormRegType::POST_INDEXED_PAIR_32},
            {"post-indexed-pair-64",
             dv::AddressFormRegType::POST_INDEXED_PAIR_64},
            {"post-indexed-pair-words",
             dv::AddressFormRegType::POST_INDEXED_PAIR_WORDS},
            {"post-indexed-pair-doublewords",
             dv::AddressFormRegType::POST_INDEXED_PAIR_DOUBLEWORDS},
            {"post-indexed-pair-quadwords",
             dv::AddressFormRegType::POST_INDEXED_PAIR_QUADWORDS},
        }}},
      {"advsimd-datatype",
       {DocVars::kAdvsimdDatatypeFieldNumber,
        {
            {"sisd-half", dv::AdvsimdDatatype::SISD_HALF},
            {"simd-half", dv::AdvsimdDatatype::SIMD_HALF},
            {"sisd-single-and-double",
             dv::AdvsimdDatatype::SISD_SINGLE_AND_DOUBLE},
            {"simd-single-and-double",
             dv::AdvsimdDatatype::SIMD_SINGLE_AND_DOUBLE},
        }}},
      {"advsimd-reguse",
       {DocVars::kAdvsimdReguseFieldNumber,
        {
            {"2reg-scalar", dv::AdvsimdReguse::X__2REG_SCALAR},
            {"2reg-element", dv::AdvsimdReguse::X__2REG_ELEMENT},
            {"3reg-same", dv::AdvsimdReguse::X__3REG_SAME},
            {"3reg-diff", dv::AdvsimdReguse::X__3REG_DIFF},
        }}},
      {"advsimd-type",
       {DocVars::kAdvsimdTypeFieldNumber,
        {
            {"sisd", dv::AdvsimdType::SISD},
            {"simd", dv::AdvsimdType::SIMD},
        }}},
      {"asimdimm-datatype",
       {DocVars::kAsimdimmDatatypeFieldNumber,
        {
            {"doubleword", dv::AsimdimmDatatype::DOUBLEWORD},
            {"per-byte", dv::AsimdimmDatatype::PER_BYTE},
            {"per-double", dv::AsimdimmDatatype::PER_DOUBLE},
            {"per-doubleword", dv::AsimdimmDatatype::PER_DOUBLEWORD},
            {"per-half", dv::AsimdimmDatatype::PER_HALF},
            {"per-halfword", dv::AsimdimmDatatype::PER_HALFWORD},
            {"per-single", dv::AsimdimmDatatype::PER_SINGLE},
            {"per-word", dv::AsimdimmDatatype::PER_WORD},
        }}},
      {"asimdimm-immtype",
       {DocVars::kAsimdimmImmtypeFieldNumber,
        {
            {"immediate", dv::AsimdimmImmtype::IMMEDIATE},
            {"shifted-immediate", dv::AsimdimmImmtype::SHIFTED_IMMEDIATE},
            {"masked-immediate", dv::AsimdimmImmtype::MASKED_IMMEDIATE},
        }}},
      {"asimdimm-mask",
       {DocVars::kAsimdimmMaskFieldNumber,
        {
            {"byte-mask", dv::AsimdimmMask::BYTE_MASK},
            {"no-byte-mask", dv::AsimdimmMask::NO_BYTE_MASK},
        }}},
      {"asimdimm-type",
       {DocVars::kAsimdimmTypeFieldNumber,
        {
            {"doubleword-immediate", dv::AsimdimmType::DOUBLEWORD_IMMEDIATE},
            {"per-byte-immediate", dv::AsimdimmType::PER_BYTE_IMMEDIATE},
            {"per-doubleword-immediate",
             dv::AsimdimmType::PER_DOUBLEWORD_IMMEDIATE},
            {"per-halfword-shifted-immediate",
             dv::AsimdimmType::PER_HALFWORD_SHIFTED_IMMEDIATE},
            {"per-word-masked-immediate",
             dv::AsimdimmType::PER_WORD_MASKED_IMMEDIATE},
            {"per-word-shifted-immediate",
             dv::AsimdimmType::PER_WORD_SHIFTED_IMMEDIATE},
        }}},
      {"as-structure-index-source",
       {DocVars::kAsStructureIndexSourceFieldNumber,
        {
            {"post-index-imm", dv::AsStructureIndexSource::POST_INDEX_IMM},
            {"post-index-reg", dv::AsStructureIndexSource::POST_INDEX_REG},
        }}},
      {"as-structure-org",
       {DocVars::kAsStructureOrgFieldNumber,
        {
            {"of-bytes", dv::AsStructureOrg::OF_BYTES},
            {"of-halfwords", dv::AsStructureOrg::OF_HALFWORDS},
            {"of-words", dv::AsStructureOrg::OF_WORDS},
            {"of-doublewords", dv::AsStructureOrg::OF_DOUBLEWORDS},
            {"to-all-lanes", dv::AsStructureOrg::TO_ALL_LANES},
        }}},
      {"as-structure-post-index",
       {DocVars::kAsStructurePostIndexFieldNumber,
        {
            {"as-no-post-index", dv::AsStructurePostIndex::AS_NO_POST_INDEX},
            {"as-post-index", dv::AsStructurePostIndex::AS_POST_INDEX},
        }}},
      {"bitfield-fill",
       {DocVars::kBitfieldFillFieldNumber,
        {
            {"nofill", dv::BitfieldFill::NOFILL},
            {"zero-fill", dv::BitfieldFill::ZERO_FILL},
            {"signed-fill", dv::BitfieldFill::SIGNED_FILL},
        }}},
      {"branch-offset",
       {DocVars::kBranchOffsetFieldNumber,
        {
            {"br14", dv::BranchOffset::BR14},
            {"br19", dv::BranchOffset::BR19},
            {"br26", dv::BranchOffset::BR26},
        }}},
      {"compare-with",
       {DocVars::kCompareWithFieldNumber,
        {
            {"cmp-zero", dv::CompareWith::CMP_ZERO},
            {"cmp-nonzero", dv::CompareWith::CMP_NONZERO},
            {"cmp-cond", dv::CompareWith::CMP_COND},
            {"cmp-reg", dv::CompareWith::CMP_REG},
        }}},
      {"cond-setting",
       {DocVars::kCondSettingFieldNumber,
        {
            {"S", dv::CondSetting::S},
            {"no-s", dv::CondSetting::NO_S},
        }}},
      {"convert-type",
       {DocVars::kConvertTypeFieldNumber,
        {
            {"32-to-double", dv::ConvertType::X__32_TO_DOUBLE},
            {"32-to-half", dv::ConvertType::X__32_TO_HALF},
            {"32-to-single", dv::ConvertType::X__32_TO_SINGLE},
            {"64-to-double", dv::ConvertType::X__64_TO_DOUBLE},
            {"64-to-half", dv::ConvertType::X__64_TO_HALF},
            {"64-to-quadhi", dv::ConvertType::X__64_TO_QUADHI},
            {"64-to-single", dv::ConvertType::X__64_TO_SINGLE},
            {"double-to-32", dv::ConvertType::DOUBLE_TO_32},
            {"double-to-64", dv::ConvertType::DOUBLE_TO_64},
            {"double-to-fix32", dv::ConvertType::DOUBLE_TO_FIX32},
            {"double-to-fix64", dv::ConvertType::DOUBLE_TO_FIX64},
            {"double-to-half", dv::ConvertType::DOUBLE_TO_HALF},
            {"double-to-single", dv::ConvertType::DOUBLE_TO_SINGLE},
            {"fix32-to-double", dv::ConvertType::FIX32_TO_DOUBLE},
            {"fix32-to-half", dv::ConvertType::FIX32_TO_HALF},
            {"fix32-to-single", dv::ConvertType::FIX32_TO_SINGLE},
            {"fix64-to-double", dv::ConvertType::FIX64_TO_DOUBLE},
            {"fix64-to-half", dv::ConvertType::FIX64_TO_HALF},
            {"fix64-to-single", dv::ConvertType::FIX64_TO_SINGLE},
            {"half-to-32", dv::ConvertType::HALF_TO_32},
            {"half-to-64", dv::ConvertType::HALF_TO_64},
            {"half-to-double", dv::ConvertType::HALF_TO_DOUBLE},
            {"half-to-fix32", dv::ConvertType::HALF_TO_FIX32},
            {"half-to-fix64", dv::ConvertType::HALF_TO_FIX64},
            {"half-to-single", dv::ConvertType::HALF_TO_SINGLE},
            {"quadhi-to-64", dv::ConvertType::QUADHI_TO_64},
            {"single-to-32", dv::ConvertType::SINGLE_TO_32},
            {"single-to-64", dv::ConvertType::SINGLE_TO_64},
            {"single-to-double", dv::ConvertType::SINGLE_TO_DOUBLE},
            {"single-to-fix32", dv::ConvertType::SINGLE_TO_FIX32},
            {"single-to-fix64", dv::ConvertType::SINGLE_TO_FIX64},
            {"single-to-half", dv::ConvertType::SINGLE_TO_HALF},
        }}},
      {"datatype",
       {DocVars::kDatatypeFieldNumber,
        {
            {"32", dv::Datatype::X__32},
            {"64", dv::Datatype::X__64},
            {"half", dv::Datatype::HALF},
            {"single", dv::Datatype::SINGLE},
            {"double", dv::Datatype::DOUBLE},
            {"single-and-double", dv::Datatype::SINGLE_AND_DOUBLE},
        }}},
      {"datatype-reguse",
       {DocVars::kDatatypeReguseFieldNumber,
        {
            {"32-ext-reg", dv::DatatypeReguse::X__32_EXT_REG},
            {"32-shifted-reg", dv::DatatypeReguse::X__32_SHIFTED_REG},
            {"64-ext-reg", dv::DatatypeReguse::X__64_EXT_REG},
            {"64-shifted-reg", dv::DatatypeReguse::X__64_SHIFTED_REG},
        }}},
      {"feature",
       {DocVars::kFeatureFieldNumber,
        {
            {"crc", dv::Feature::CRC},
        }}},
      {"hint-variants",
       {DocVars::kHintVariantsFieldNumber,
        {
            {"hint-17-23", dv::HintVariants::HINT_17_23},
            {"hint-18-23", dv::HintVariants::HINT_18_23},
            {"hint-6-7", dv::HintVariants::HINT_6_7},
            {"hint-8-15-24-127", dv::HintVariants::HINT_8_15_24_127},
        }}},
      {"immediate-type",
       {DocVars::kImmediateTypeFieldNumber,
        {
            {"imm5u", dv::ImmediateType::IMM5U},
            {"imm8f", dv::ImmediateType::IMM8F},
            {"imm12u", dv::ImmediateType::IMM12U},
            {"imm12-bitfield", dv::ImmediateType::IMM12_BITFIELD},
            {"imm18-packed", dv::ImmediateType::IMM18_PACKED},
        }}},
      {"instr-class",
       {DocVars::kInstrClassFieldNumber,
        {
            {"general", dv::InstrClass::GENERAL},
            {"system", dv::InstrClass::SYSTEM},
            {"float", dv::InstrClass::FLOAT},
            {"fpsimd", dv::InstrClass::FPSIMD},
            {"advsimd", dv::InstrClass::ADVSIMD},
        }}},
      {"isa",
       {DocVars::kIsaFieldNumber,
        {
            {"A32", dv::Isa::A32},
            {"A64", dv::Isa::A64},
        }}},
      {"ld1-multiple-labels",
       {DocVars::kLd1MultipleLabelsFieldNumber,
        {
            {"post-index-imm-to-1reg",
             dv::Ld1MultipleLabels::POST_INDEX_IMM_TO_1REG},
            {"post-index-imm-to-2reg",
             dv::Ld1MultipleLabels::POST_INDEX_IMM_TO_2REG},
            {"post-index-imm-to-3reg",
             dv::Ld1MultipleLabels::POST_INDEX_IMM_TO_3REG},
            {"post-index-imm-to-4reg",
             dv::Ld1MultipleLabels::POST_INDEX_IMM_TO_4REG},
            {"post-index-reg-to-1reg",
             dv::Ld1MultipleLabels::POST_INDEX_REG_TO_1REG},
            {"post-index-reg-to-2reg",
             dv::Ld1MultipleLabels::POST_INDEX_REG_TO_2REG},
            {"post-index-reg-to-3reg",
             dv::Ld1MultipleLabels::POST_INDEX_REG_TO_3REG},
            {"post-index-reg-to-4reg",
             dv::Ld1MultipleLabels::POST_INDEX_REG_TO_4REG},
        }}},
      {"ld1-single-labels",
       {DocVars::kLd1SingleLabelsFieldNumber,
        {
            {"of-bytes-post-index-imm",
             dv::Ld1SingleLabels::OF_BYTES_POST_INDEX_IMM},
            {"of-bytes-post-index-reg",
             dv::Ld1SingleLabels::OF_BYTES_POST_INDEX_REG},
            {"of-doublewords-post-index-imm",
             dv::Ld1SingleLabels::OF_DOUBLEWORDS_POST_INDEX_IMM},
            {"of-doublewords-post-index-reg",
             dv::Ld1SingleLabels::OF_DOUBLEWORDS_POST_INDEX_REG},
            {"of-halfwords-post-index-imm",
             dv::Ld1SingleLabels::OF_HALFWORDS_POST_INDEX_IMM},
            {"of-halfwords-post-index-reg",
             dv::Ld1SingleLabels::OF_HALFWORDS_POST_INDEX_REG},
            {"of-words-post-index-imm",
             dv::Ld1SingleLabels::OF_WORDS_POST_INDEX_IMM},
            {"of-words-post-index-reg",
             dv::Ld1SingleLabels::OF_WORDS_POST_INDEX_REG},
            {"to-all-lanes-post-index-imm",
             dv::Ld1SingleLabels::TO_ALL_LANES_POST_INDEX_IMM},
            {"to-all-lanes-post-index-reg",
             dv::Ld1SingleLabels::TO_ALL_LANES_POST_INDEX_REG},
        }}},
      {"ldstruct-regcount",
       {DocVars::kLdstructRegcountFieldNumber,
        {
            {"to-1reg", dv::LdstructRegcount::TO_1REG},
            {"to-2reg", dv::LdstructRegcount::TO_2REG},
            {"to-3reg", dv::LdstructRegcount::TO_3REG},
            {"to-4reg", dv::LdstructRegcount::TO_4REG},
        }}},
      {"loadstore-bra",
       {DocVars::kLoadstoreBraFieldNumber,
        {
            {"key-a-zmod", dv::LoadstoreBra::KEY_A_ZMOD},
            {"key-a-regmod", dv::LoadstoreBra::KEY_A_REGMOD},
            {"key-b-zmod", dv::LoadstoreBra::KEY_B_ZMOD},
            {"key-b-regmod", dv::LoadstoreBra::KEY_B_REGMOD},
        }}},
      {"loadstore-order",
       {DocVars::kLoadstoreOrderFieldNumber,
        {
            {"acquire", dv::LoadstoreOrder::ACQUIRE},
            {"acquire-release", dv::LoadstoreOrder::ACQUIRE_RELEASE},
            {"no-order", dv::LoadstoreOrder::NO_ORDER},
            {"release", dv::LoadstoreOrder::RELEASE},
        }}},
      {"loadstore-order-reg-type",
       {DocVars::kLoadstoreOrderRegTypeFieldNumber,
        {
            {"acquire-32-reg", dv::LoadstoreOrderRegType::ACQUIRE_32_REG},
            {"acquire-64-reg", dv::LoadstoreOrderRegType::ACQUIRE_64_REG},
            {"acquire-pair-32", dv::LoadstoreOrderRegType::ACQUIRE_PAIR_32},
            {"acquire-pair-64", dv::LoadstoreOrderRegType::ACQUIRE_PAIR_64},
            {"acquire-release-32-reg",
             dv::LoadstoreOrderRegType::ACQUIRE_RELEASE_32_REG},
            {"acquire-release-64-reg",
             dv::LoadstoreOrderRegType::ACQUIRE_RELEASE_64_REG},
            {"acquire-release-pair-32",
             dv::LoadstoreOrderRegType::ACQUIRE_RELEASE_PAIR_32},
            {"acquire-release-pair-64",
             dv::LoadstoreOrderRegType::ACQUIRE_RELEASE_PAIR_64},
            {"no-order-32-reg", dv::LoadstoreOrderRegType::NO_ORDER_32_REG},
            {"no-order-64-reg", dv::LoadstoreOrderRegType::NO_ORDER_64_REG},
            {"no-order-pair-32", dv::LoadstoreOrderRegType::NO_ORDER_PAIR_32},
            {"no-order-pair-64", dv::LoadstoreOrderRegType::NO_ORDER_PAIR_64},
            {"release-32-reg", dv::LoadstoreOrderRegType::RELEASE_32_REG},
            {"release-64-reg", dv::LoadstoreOrderRegType::RELEASE_64_REG},
            {"release-pair-32", dv::LoadstoreOrderRegType::RELEASE_PAIR_32},
            {"release-pair-64", dv::LoadstoreOrderRegType::RELEASE_PAIR_64},
        }}},
      {"loadstore-pa",
       {DocVars::kLoadstorePaFieldNumber,
        {
            {"key-a-offs", dv::LoadstorePa::KEY_A_OFFS},
            {"key-a-preind", dv::LoadstorePa::KEY_A_PREIND},
            {"key-b-offs", dv::LoadstorePa::KEY_B_OFFS},
            {"key-b-preind", dv::LoadstorePa::KEY_B_PREIND},
        }}},
      {"move-what",
       {DocVars::kMoveWhatFieldNumber,
        {
            {"mov-bitmask", dv::MoveWhat::MOV_BITMASK},
            {"mov-register", dv::MoveWhat::MOV_REGISTER},
            {"mov-wideimm", dv::MoveWhat::MOV_WIDEIMM},
            {"mov-wideinv", dv::MoveWhat::MOV_WIDEINV},
            {"to-from-sp", dv::MoveWhat::TO_FROM_SP},
        }}},
      {"msr-sysreg-target",
       {DocVars::kMsrSysregTargetFieldNumber,
        {
            {"register-field", dv::MsrSysregTarget::REGISTER_FIELD},
            {"whole-register", dv::MsrSysregTarget::WHOLE_REGISTER},
        }}},
      {"no-reg-for-table",
       {DocVars::kNoRegForTableFieldNumber,
        {
            {"tbl1", dv::NoRegForTable::TBL1},
            {"tbl2", dv::NoRegForTable::TBL2},
            {"tbl3", dv::NoRegForTable::TBL3},
            {"tbl4", dv::NoRegForTable::TBL4},
        }}},
      {"offset-type",
       {DocVars::kOffsetTypeFieldNumber,
        {
            {"off-reg", dv::OffsetType::OFF_REG},
            {"off7s_s", dv::OffsetType::OFF7S_S},
            {"off8s_u", dv::OffsetType::OFF8S_U},
            {"off9s_u", dv::OffsetType::OFF9S_U},
            {"off12u_s", dv::OffsetType::OFF12U_S},
            {"off19s", dv::OffsetType::OFF19S},
        }}},
      {"reg-type",
       {DocVars::kRegTypeFieldNumber,
        {
            {"32-reg", dv::RegType::X__32_REG},
            {"64-reg", dv::RegType::X__64_REG},
            {"8-fsreg", dv::RegType::X__8_FSREG},
            {"16-fsreg", dv::RegType::X__16_FSREG},
            {"32-fsreg", dv::RegType::X__32_FSREG},
            {"64-fsreg", dv::RegType::X__64_FSREG},
            {"128-fsreg", dv::RegType::X__128_FSREG},
            {"pair-32", dv::RegType::PAIR_32},
            {"pair-64", dv::RegType::PAIR_64},
            {"pair-words", dv::RegType::PAIR_WORDS},
            {"pair-doublewords", dv::RegType::PAIR_DOUBLEWORDS},
            {"pair-quadwords", dv::RegType::PAIR_QUADWORDS},
        }}},
      {"reg-type-and-use",
       {DocVars::kRegTypeAndUseFieldNumber,
        {
            {"8-fsreg-ext-reg", dv::RegTypeAndUse::X__8_FSREG_EXT_REG},
            {"8-fsreg-shifted-reg", dv::RegTypeAndUse::X__8_FSREG_SHIFTED_REG},
        }}},
      {"reguse",
       {DocVars::kReguseFieldNumber,
        {
            {"shifted-reg", dv::Reguse::SHIFTED_REG},
            {"ext-reg", dv::Reguse::EXT_REG},
        }}},
      {"reguse-datatype",
       {DocVars::kReguseDatatypeFieldNumber,
        {
            {"2reg-element-half", dv::ReguseDatatype::X__2REG_ELEMENT_HALF},
            {"2reg-element-single-and-double",
             dv::ReguseDatatype::X__2REG_ELEMENT_SINGLE_AND_DOUBLE},
            {"2reg-scalar-half", dv::ReguseDatatype::X__2REG_SCALAR_HALF},
            {"2reg-scalar-single-and-double",
             dv::ReguseDatatype::X__2REG_SCALAR_SINGLE_AND_DOUBLE},
            {"3reg-same-half", dv::ReguseDatatype::X__3REG_SAME_HALF},
            {"3reg-same-single-and-double",
             dv::ReguseDatatype::X__3REG_SAME_SINGLE_AND_DOUBLE},
        }}},
      {"source-type",
       {DocVars::kSourceTypeFieldNumber,
        {
            {"src-is-immediate", dv::SourceType::SRC_IS_IMMEDIATE},
            {"src-is-register", dv::SourceType::SRC_IS_REGISTER},
        }}},
      {"sti-mult-labels",
       {DocVars::kStiMultLabelsFieldNumber,
        {
            {"from-1reg-post-index-imm",
             dv::StiMultLabels::FROM_1REG_POST_INDEX_IMM},
            {"from-1reg-post-index-reg",
             dv::StiMultLabels::FROM_1REG_POST_INDEX_REG},
            {"from-2reg-post-index-imm",
             dv::StiMultLabels::FROM_2REG_POST_INDEX_IMM},
            {"from-2reg-post-index-reg",
             dv::StiMultLabels::FROM_2REG_POST_INDEX_REG},
            {"from-3reg-post-index-imm",
             dv::StiMultLabels::FROM_3REG_POST_INDEX_IMM},
            {"from-3reg-post-index-reg",
             dv::StiMultLabels::FROM_3REG_POST_INDEX_REG},
            {"from-4reg-post-index-imm",
             dv::StiMultLabels::FROM_4REG_POST_INDEX_IMM},
            {"from-4reg-post-index-reg",
             dv::StiMultLabels::FROM_4REG_POST_INDEX_REG},
        }}},
      {"ststruct-regcount",
       {DocVars::kStstructRegcountFieldNumber,
        {
            {"from-1reg", dv::StstructRegcount::FROM_1REG},
            {"from-2reg", dv::StstructRegcount::FROM_2REG},
            {"from-3reg", dv::StstructRegcount::FROM_3REG},
            {"from-4reg", dv::StstructRegcount::FROM_4REG},
        }}},
      {"vector-xfer-type",
       {DocVars::kVectorXferTypeFieldNumber,
        {
            {"scalar-from-element", dv::VectorXferType::SCALAR_FROM_ELEMENT},
            {"element-from-element", dv::VectorXferType::ELEMENT_FROM_ELEMENT},
            {"general-from-element", dv::VectorXferType::GENERAL_FROM_ELEMENT},
            {"vector-from-element", dv::VectorXferType::VECTOR_FROM_ELEMENT},
            {"vector-from-vector", dv::VectorXferType::VECTOR_FROM_VECTOR},
            {"vector-from-general", dv::VectorXferType::VECTOR_FROM_GENERAL},
        }}},
  });
  return *kDocVarsEnumMapping;
}  // NOLINT(readability/fn_size)

}  // namespace

StatusOr<DocVars> ParseDocVars(XMLNode* node) {
  CHECK(node != nullptr);

  DocVars result;
  const auto* reflection = result.GetReflection();

  for (const auto& docvar : ReadDocVars(node)) {
    const KeyString& key = docvar.first;
    const ValueString& value = docvar.second;

    // First attempt to fill non-enum fields.
    if (key == "mnemonic") {
      result.set_mnemonic(absl::AsciiStrToUpper(value));
      continue;
    } else if (key == "alias_mnemonic") {
      result.set_alias_mnemonic(absl::AsciiStrToUpper(value));
      continue;
    }

    // The "atomic-ops" DocVar value always consists in a concatenation of both
    // "mnemonic" & "reg-type" DocVar values so it's not directly interesting.
    // However the fact that it is set (vs unmentioned) might be relevant.
    if (key == "atomic-ops") {
      result.set_atomic_ops(dv::AtomicOps::ATOMIC_OPS_SET);
      continue;
    }

    // Now handle regular DocVar enum values using reflection.
    const auto* mapping = gtl::FindOrNull(GetDocVarsEnumMapping(), key);
    if (!mapping) {
      return UnimplementedError(absl::StrCat("Unknown docvar key '", key, "'"));
    }
    const auto* desc = DocVars::descriptor()->FindFieldByNumber(mapping->first);
    const auto* enum_value = gtl::FindOrNull(mapping->second, value);
    if (!enum_value) {
      const auto& type = reflection->GetEnum(result, desc)->type()->name();
      return UnimplementedError(
          absl::StrCat("Bad value '", value, "' for ", type));
    }
    reflection->SetEnumValue(&result, desc, *enum_value);
  }

  return result;
}

Status DocVarsContains(const DocVars& docvars, const DocVars& subset) {
  MessageDifferencer differencer;
  differencer.set_scope(MessageDifferencer::PARTIAL);
  differencer.set_repeated_field_comparison(MessageDifferencer::AS_SET);
  std::string diff;
  differencer.ReportDifferencesToString(&diff);
  if (!differencer.Compare(subset, docvars)) {
    return FailedPreconditionError(
        absl::StrCat("DocVars subset mismatch:\n", diff));
  }
  return OkStatus();
}

}  // namespace xml
}  // namespace arm
}  // namespace exegesis
