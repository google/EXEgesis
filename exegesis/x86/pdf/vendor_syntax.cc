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

#include "exegesis/x86/pdf/vendor_syntax.h"

#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "exegesis/util/strings.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "util/gtl/map_util.h"

namespace exegesis {
namespace x86 {
namespace pdf {
namespace {

constexpr const size_t kMaxInstructionIdSize = 60;

// The list of operand names from the Intel encoding specification that are
// accepted by the converter.
// TODO(courbet): Generate these automatically.
const char* const kValidOperandTypes[] = {
    // Implicit values, used for shifts and interrupts.
    "0",
    "1",
    "3",
    // 8-bit general-purpose registers.
    "r8",
    // 16-bit general-purpose registers.
    "r16",
    // 32-bit general-purpose registers.
    "r32",
    "r32a",
    "r32b",
    // 64-bit general-purpose registers.
    "r64",
    "r64a",
    "r64b",
    // General-purpose registers/memory addressed through ModR/M.
    "r/m8",
    "r/m16",
    "r/m32",
    "r/m64",
    "r32/m8",
    "r16/m16",
    "r32/m16",
    "r64/m16",
    "r16/r32/m16",
    "r16/r32/r64",
    // Any general-purpose register.
    "reg",
    // Any general-purpose register/memory.
    "reg/m8",
    "reg/m16",
    "reg/m32",
    // Specific general-purpose registers.
    "AL",
    "AX",
    "CL",
    "DX",
    "EAX",
    "RAX",
    // Control registers.
    "CR0-CR7",
    "CR8",
    "CS",
    "DR0-DR7",
    // Segment registers.
    "Sreg",
    "DS",
    "ES",
    "FS",
    "GS",
    "SS",
    // Immediate values.
    "imm8",
    "imm16",
    "imm32",
    "imm64",
    // Memory addresses
    "m",
    "mem",
    // Addresses pointing to 8-bit values.
    "m8",
    // Addresses pointing to 16-bit integers.
    "m16",
    "m16int",
    // Addresses pointing to 32-bit integers or floats.
    "m32",
    "m32fp",
    "m32int",
    // Addresses pointing to 64-bit integers or double floats.
    "m64",
    "m64fp",
    "m64int",
    // Addresses pointing to 80-bit long doubles.
    "m80bcd",
    "m80dec",
    "m80fp",
    // Addresses pointing to 128-bit XMM/MMX values.
    "m128",
    // Addresses pointing to 256-bit YMM values.
    "m256",
    // Addresses pointing to 512-bit ZMM values.
    "m512",
    // Addresses pointing to state storage.
    "m2byte",
    "m14byte",
    "m14/28byte",
    "m28byte",
    "m94byte",
    "m94/108byte",
    "m108byte",
    "m512byte",
    // Indirect far pointers
    "m16:16",
    "m16:32",
    "m16:64",
    // Addresses pointing to pairs of integers.
    "m16&16",
    "m16&32",
    "m16&64",
    "m32&32",
    // Immediate far pointers.
    "ptr16:16",
    "ptr16:32",
    // Memory offsets.
    "moffs8",
    "moffs16",
    "moffs32",
    "moffs64",
    // Relative branch values.
    "rel8",
    "rel16",
    "rel32",
    // Floating-point stack registers.
    "ST",
    "ST(0)",
    "ST(i)",
    // MMX registers.
    "mm",
    "mm1",
    "mm2",
    // MMX registers/memory addressed through ModR/M.
    "mm/m32",
    "mm/m64",
    "mm2/m64",
    // XMM registers, <XMM0> is implicit.
    "<XMM0>",
    "xmm",
    "xmm0",
    "xmm1",
    "xmm2",
    "xmm3",
    "xmm4",
    // XMM registers/memory addressed through ModR/M.
    "xmm2/m8",
    "xmm2/m16",
    "xmm/m32",
    "xmm1/m32",
    "xmm2/m32",
    "xmm3/m32",
    "xmm/m64",
    "xmm1/m16",
    "xmm1/m64",
    "xmm2/m64",
    "xmm3/m64",
    "xmm/m128",
    "xmm1/m128",
    "xmm2/m128",
    "xmm3/m128",
    // XMM registers/memory/vector addressed through ModR/M or EVEX.
    "xmm2/m64/m32bcst",
    "xmm2/m128/m32bcst",
    "xmm2/m128/m64bcst",
    "xmm3/m128/m32bcst",
    "xmm3/m128/m64bcst",
    // XMM registers addressed through the block scheme.
    "xmm2+3",
    // YMM registers.
    "ymm0",
    "ymm1",
    "ymm2",
    "ymm3",
    "ymm4",
    // YMM registers/memory addressed through ModR/M.
    "ymm1/m256",
    "ymm2/m256",
    "ymm3/m256",
    // YMM registers/memory/vector addressed through ModR/M or EVEX.
    "ymm2/m256/m32bcst",
    "ymm2/m256/m64bcst",
    "ymm3/m256/m32bcst",
    "ymm3/m256/m64bcst",
    // YMM registers addressed through the block scheme.
    "ymm2+3",
    // ZMM registers.
    "zmm0",
    "zmm1",
    "zmm2",
    "zmm3",
    "zmm4",
    // ZMM registers/memory addressed through ModR/M.
    "zmm0/m512",
    "zmm1/m512",
    "zmm2/m512",
    "zmm3/m512",
    // ZMM registers/memory/vector addressed through ModR/M or EVEX.
    "zmm2/m512/m32bcst",
    "zmm2/m512/m64bcst",
    "zmm3/m512/m32bcst",
    "zmm3/m512/m64bcst",
    // ZMM registers addressed through the block scheme.
    "zmm2+3",
    // AVX vector addresses.
    "vm32x",
    "vm32y",
    "vm32z",
    "vm64x",
    "vm64y",
    "vm64z",
    // MPX registers.
    "bnd",
    "bnd0",
    "bnd1",
    "bnd2",
    "bnd3",
    "bnd0/m64",
    "bnd0/m128",
    "bnd1/m64",
    "bnd1/m128",
    "bnd2/m64",
    "bnd2/m128",
    "bnd3/m64",
    "bnd3/m128",
    "mib",
    // Opmask registers.
    "k0",
    "k1",
    "k2",
    "k3",
    "k4",
    "k5",
    "k6",
    "k7",
    // Opmask registers/memory.
    "k2/m8",
    "k2/m16",
    "k2/m32",
    "k2/m64",
};

std::string FixOperandName(const std::string& operand_name) {
  // List of substitutions in operand names. Note that these substitutions are
  // only used to fix obvious typos and formatting errors in the manual.
  // Systematic inconsistencies are fixed by the transforms library.
  static const auto* const kOperandNameSubstitutions =
      new absl::flat_hash_map<std::string, std::string>({
          {"imm8/r", "imm8"},
          {"r32/m161", "r32/m16"},
          {"r32/m32", "r/m32"},
          {"r64/m64", "r/m64"},
          {"xmm2/ m128", "xmm2/m128"},
          {"xmm3 /m128", "xmm3/m128"},
          {"ymm3/.m256", "ymm3/m256"},
          {"ymm3 /m256", "ymm3/m256"},
          {"zmm3 /m512", "zmm3/m512"},
      });
  return ::exegesis::gtl::FindWithDefault(*kOperandNameSubstitutions,
                                          operand_name, operand_name);
}

// The vendor syntax has always the format [prefix] mnemonic op1, op2[, op3].
// An operand can optionally be followed by up to two tags (e.g. "{k1}").
const LazyRE2 kMnemonicRegexp = {R"(\s*((?:REPN?[EZ]?\s+)?[A-Z0-9x]+)\s*)"};
const LazyRE2 kOperandRegexp = {
    R"(([^,{]+)(?:{([a-z0-9]+)})?\s*(?:{([a-z0-9]+)})?\s*,?\s*)"};

}  // namespace

bool ParseVendorSyntax(std::string content,
                       InstructionFormat* instruction_format) {
  static const auto* const kValidIntelOperandTypes =
      new absl::flat_hash_set<std::string>(std::begin(kValidOperandTypes),
                                           std::end(kValidOperandTypes));
  // Remove any asterisks (typically artifacts from notes).
  content.erase(std::remove(content.begin(), content.end(), '*'),
                content.end());
  instruction_format->Clear();
  absl::string_view input(content);

  if (!RE2::Consume(&input, *kMnemonicRegexp,
                    instruction_format->mutable_mnemonic())) {
    LOG(ERROR) << "Cannot parse instruction in vendor syntax '" << content
               << "'";
    return false;
  }

  std::string operand_name;
  std::string tag1;
  std::string tag2;
  while (RE2::Consume(&input, *kOperandRegexp, &operand_name, &tag1, &tag2)) {
    absl::StripAsciiWhitespace(&operand_name);
    absl::StripAsciiWhitespace(&tag1);
    absl::StripAsciiWhitespace(&tag2);
    operand_name = FixOperandName(operand_name);
    if (!kValidIntelOperandTypes->contains(operand_name)) {
      LOG(ERROR) << "Unknown operand '" << operand_name << "' while parsing '"
                 << content << "'";
      operand_name = kUnknown;
    }
    auto* const operand = instruction_format->add_operands();
    operand->set_name(operand_name);
    if (!tag1.empty()) {
      operand->add_tags()->set_name(tag1);
    }
    if (!tag2.empty()) {
      operand->add_tags()->set_name(tag2);
    }
  }

  if (!input.empty()) {
    LOG(ERROR) << "Did not consume all input in vendor syntax '" << content
               << "' remains '" << input << "'";
    return false;
  }
  return true;
}

std::string NormalizeName(std::string text) {
  static constexpr absl::string_view kRemovedChars = "\n ∗*";
  RemoveAllChars(&text, kRemovedChars);

  if (text.size() > kMaxInstructionIdSize) {
    text.resize(kMaxInstructionIdSize);
  }
  return text;
}

}  // namespace pdf
}  // namespace x86
}  // namespace exegesis
