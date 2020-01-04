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

#include "exegesis/x86/operand_translator.h"

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "exegesis/util/instruction_syntax.h"
#include "glog/logging.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"

namespace exegesis {
namespace x86 {

namespace {

#define LABEL_OPERAND(x) "Label\n.rept " #x "\nNOP\n.endr\nLabel: NOP"
// NOTE(ondrasej): Using indirect addressing by a register is preferable here.
// When we use only a displacement, the compiler sometimes has a choice between
// one encoding based on ModR/M and one based on immediate values, and it
// usually picks the bad one. In case of CALL, it picks one that does not even
// compile and crashes LLVM on an assertion.
#define ADDRESS " ptr[RSI]"
#define OFFSET_ADDRESS " ptr DS:[RSI]"

// Returns an example of operand value for a given operand specification,
// e.g. '0x11' for 'imm8', or 'xmm5' for 'xmm'.
std::string TranslateOperand(const std::string& operand) {
  static const absl::flat_hash_map<std::string, std::string>* const
      kOperandTranslation = new absl::flat_hash_map<std::string, std::string>({
          {"CR0-CR7", "CR0"},
          {"DR0-DR7", "DR0"},
          {"<XMM0>", ""},
          {"ST(i)", "ST(2)"},
          {"bnd", "bnd2"},
          // All rel*, m, and mem are fishy.
          // NOTE(ondrasej): Some instructions use an imm8 as an additional
          // control value for the operation they perform, and might place
          // additional constraints on this value. For example the EVEX version
          // of VCMPDD requires that the value of the immediate uses only the
          // four least significant bits.
          {"imm8", "0x11"},
          {"imm16", "0x7ffe"},
          {"imm32", "0x7ffffffe"},
          {"imm64", "0x400000000002d06d"},
          {"rel8", LABEL_OPERAND(64)},
          {"rel16", LABEL_OPERAND(0x100)},
          {"rel32", LABEL_OPERAND(0x10000)},
          {"m8", "byte" ADDRESS},
          {"mib", "qword" ADDRESS},
          {"moffs8", "byte" OFFSET_ADDRESS},
          {"m", "word" ADDRESS},
          {"m16", "word" ADDRESS},
          {"m16&16", "word" ADDRESS},
          {"m16&64", "qword" ADDRESS},
          {"m16int", "word" ADDRESS},
          {"moffs16", "word" OFFSET_ADDRESS},
          {"m2byte", "word" ADDRESS},
          {"m14byte", "dword" ADDRESS},  // LLVM differs from the Intel spec.
          {"m28byte", "dword" ADDRESS},  // LLVM differs from the Intel spec.
          {"m32", "dword" ADDRESS},
          {"m32&32", "dword" ADDRESS},
          {"moffs32", "dword" OFFSET_ADDRESS},
          {"m32fp", "dword" ADDRESS},
          {"m32int", "dword" ADDRESS},
          {"m64", "qword" ADDRESS},
          {"moffs64", "qword" OFFSET_ADDRESS},
          {"mem", "xmmword" ADDRESS},
          {"m64fp", "qword" ADDRESS},
          {"m64int", "dword" ADDRESS},
          {"m80dec", "xword" ADDRESS},
          {"m80bcd", "xword" ADDRESS},
          {"m80fp", "xword" ADDRESS},
          {"m128", "xmmword" ADDRESS},
          {"m256", "ymmword" ADDRESS},
          {"m512", "zmmword" ADDRESS},
          {"m94byte", "dword" ADDRESS},   // LLVM differs from the Intel spec.
          {"m108byte", "dword" ADDRESS},  // LLVM differs from the Intel spec.
          {"m512byte", "opaque" ADDRESS},
          {"ptr16:16", "0x7f16:0x7f16"},
          {"ptr16:32", "0x3039:0x30393039"},
          {"m16:16", "word" ADDRESS},
          {"m16:32", "dword" ADDRESS},
          {"m16:64", "qword" ADDRESS},
          {"xmm", "xmm5"},
          {"mm", "mm6"},
          {"Sreg", "cs"},
          {"ST(i)", "ST(3)"},
          {"vm32x", "[rsp + 4* xmm9]"},
          {"vm32y", "[rsp + 4* ymm10]"},
          {"vm64x", "[rsp + 8* xmm11]"},
          {"vm64y", "[rsp + 8* ymm12]"},
          {"vm64z", "[rsp + 8* zmm13]"},
      });
  return ::exegesis::gtl::FindWithDefault(*kOperandTranslation, operand,
                                          operand);
}

#undef LABEL_OPERAND
#undef ADDRESS
#undef OFFSET_ADDRESS

std::string TranslateGPR(const std::string& operand) {
  // Note: keep in sync with clobbered registers in AddItineraries.
  static const absl::flat_hash_map<std::string, std::string>* const
      kGPRLegacyTranslation =
          new absl::flat_hash_map<std::string, std::string>({
              {"r8", "ch"},
              {"r16", "cx"},
              {"r32", "ecx"},
              {"r32a", "eax"},
              {"r32b", "ebx"},
              {"r64", "rcx"},
              {"r64a", "rax"},
              {"r64b", "rbx"},
              // Warning: valid for r64 and r32
              {"reg", "rdx"},
          });

  return ::exegesis::gtl::FindWithDefault(*kGPRLegacyTranslation, operand,
                                          operand);
}

std::string TranslateREX(const std::string& operand) {
  // Note: keep in sync with clobbered registers in AddItineraries.
  static const absl::flat_hash_map<std::string, std::string>* const
      kGPRREXTranslation = new absl::flat_hash_map<std::string, std::string>({
          {"r8", "r8b"},
          {"r16", "r10w"},
          {"r32", "r10d"},
          {"r32a", "r8d"},
          {"r32b", "r9d"},
          {"r64", "r10"},
          {"r64a", "r8"},
          {"r64b", "r9"},
          // Warning: valid for r64 and r32
          {"reg", "r11"},
      });
  return ::exegesis::gtl::FindWithDefault(*kGPRREXTranslation, operand,
                                          operand);
}

InstructionOperand::Tag TranslateOperandTag(
    const InstructionOperand::Tag& tag) {
  static const auto* const kTagTranslation =
      new absl::flat_hash_map<std::string, std::string>({{"er", "rn-sae"}});
  InstructionOperand::Tag code_tag;
  code_tag.set_name(::exegesis::gtl::FindWithDefault(*kTagTranslation,
                                                     tag.name(), tag.name()));
  return code_tag;
}

}  // namespace

InstructionFormat InstantiateOperands(const InstructionProto& instruction) {
  InstructionFormat result;
  // Deal with the fact that the LLVM assembler cannot assemble MOV r64,imm64.
  const InstructionFormat& vendor_syntax =
      GetVendorSyntaxWithMostOperandsOrDie(instruction);
  const bool is_movabs = vendor_syntax.mnemonic() == "MOV" &&
                         vendor_syntax.operands(1).name() == "imm64";
  result.set_mnemonic(is_movabs ? "MOVABS" : vendor_syntax.mnemonic());
  for (const auto& operand : vendor_syntax.operands()) {
    std::string code_operand = TranslateOperand(operand.name());
    if (code_operand == operand.name()) {
      code_operand = instruction.legacy_instruction()
                         ? TranslateGPR(operand.name())
                         : TranslateREX(operand.name());
    }
    // NOTE(ondrasej): We need to allow empty operand names with tags to support
    // AVX-512 instructions, where {sae} and the embedded rounding tags are
    // separated from other operands by a comma.
    if (!code_operand.empty() || !operand.tags().empty()) {
      InstructionOperand* const instantiated_operand = result.add_operands();
      instantiated_operand->set_name(code_operand);
      for (const InstructionOperand::Tag& tag : operand.tags()) {
        *instantiated_operand->add_tags() = TranslateOperandTag(tag);
      }
    } else {
      CHECK_EQ(operand.name(), "<XMM0>")
          << "\"" << operand.name() << "\" could not be translated.";
    }
  }
  return result;
}

}  // namespace x86
}  // namespace exegesis
