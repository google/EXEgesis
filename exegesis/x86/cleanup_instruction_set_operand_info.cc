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

#include "exegesis/x86/cleanup_instruction_set_operand_info.h"

#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "exegesis/base/cleanup_instruction_set.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/proto/registers.pb.h"
#include "exegesis/proto/x86/encoding_specification.pb.h"
#include "exegesis/util/category_util.h"
#include "exegesis/util/instruction_syntax.h"
#include "exegesis/util/status_util.h"
#include "exegesis/x86/encoding_specification.h"
#include "glog/logging.h"
#include "re2/re2.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"
#include "util/task/status_macros.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::util::FailedPreconditionError;
using ::exegesis::util::InvalidArgumentError;
using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;

using EncodingMap =
    absl::flat_hash_map<std::string, InstructionOperand::Encoding>;
using AddressingModeMap =
    absl::flat_hash_map<std::string, InstructionOperand::AddressingMode>;
using ValueSizeMap = absl::flat_hash_map<std::string, uint32_t>;
using RegisterClassMap =
    absl::flat_hash_map<std::string, RegisterProto::RegisterClass>;

// Contains mapping from operand names to their encoding types. Note that this
// mapping is incomplete, because it contains the mapping only for the cases in
// which the mapping can be determined uniquely from the operand type. For all
// other cases, the encoding can't be determined without additional information.
//
// The following rules were used:
// 1. All operands that are named explicitly are implicit.
// 2. All immediate value operands are encoded directly in the instruction.
// 3. All memory and register/memory operands are encoded in modrm.rm.
// 3. As of 2015-09, all control registers are encoded in modrm.reg.
// 4. As of 2015-09, all ST(i) registers are encoded in modrm.reg.
// 5. As of 2015-09, all segment registers are encoded in modrm.reg.
const std::pair<const char*, InstructionOperand::Encoding> kEncodingMap[] = {
    {"AL", InstructionOperand::IMPLICIT_ENCODING},
    {"AX", InstructionOperand::IMPLICIT_ENCODING},
    {"EAX", InstructionOperand::IMPLICIT_ENCODING},
    {"RAX", InstructionOperand::IMPLICIT_ENCODING},
    {"CL", InstructionOperand::IMPLICIT_ENCODING},
    // NOTE(ondrasej): In the 2015-09 version of the manual, the control
    // registers CR0-CR8 and DR0-DR7 are always encoded in modrm.reg.
    {"CR0-CR7", InstructionOperand::MODRM_REG_ENCODING},
    {"CR8", InstructionOperand::MODRM_REG_ENCODING},
    {"DR0-DR7", InstructionOperand::MODRM_REG_ENCODING},
    {"CS", InstructionOperand::IMPLICIT_ENCODING},
    {"DS", InstructionOperand::IMPLICIT_ENCODING},
    {"ES", InstructionOperand::IMPLICIT_ENCODING},
    {"DX", InstructionOperand::IMPLICIT_ENCODING},
    {"FS", InstructionOperand::IMPLICIT_ENCODING},
    {"GS", InstructionOperand::IMPLICIT_ENCODING},
    {"SS", InstructionOperand::IMPLICIT_ENCODING},
    {"BYTE PTR [RSI]", InstructionOperand::IMPLICIT_ENCODING},
    {"WORD PTR [RSI]", InstructionOperand::IMPLICIT_ENCODING},
    {"DWORD PTR [RSI]", InstructionOperand::IMPLICIT_ENCODING},
    {"QWORD PTR [RSI]", InstructionOperand::IMPLICIT_ENCODING},
    {"BYTE PTR [RDI]", InstructionOperand::IMPLICIT_ENCODING},
    {"WORD PTR [RDI]", InstructionOperand::IMPLICIT_ENCODING},
    {"DWORD PTR [RDI]", InstructionOperand::IMPLICIT_ENCODING},
    {"QWORD PTR [RDI]", InstructionOperand::IMPLICIT_ENCODING},
    {"imm8", InstructionOperand::IMMEDIATE_VALUE_ENCODING},
    {"imm16", InstructionOperand::IMMEDIATE_VALUE_ENCODING},
    {"imm32", InstructionOperand::IMMEDIATE_VALUE_ENCODING},
    {"imm64", InstructionOperand::IMMEDIATE_VALUE_ENCODING},
    {"k2/m8", InstructionOperand::MODRM_RM_ENCODING},
    {"k2/m16", InstructionOperand::MODRM_RM_ENCODING},
    {"k2/m32", InstructionOperand::MODRM_RM_ENCODING},
    {"k2/m64", InstructionOperand::MODRM_RM_ENCODING},
    {"rel8", InstructionOperand::IMMEDIATE_VALUE_ENCODING},
    {"rel16", InstructionOperand::IMMEDIATE_VALUE_ENCODING},
    {"rel32", InstructionOperand::IMMEDIATE_VALUE_ENCODING},
    {"moffs8", InstructionOperand::IMMEDIATE_VALUE_ENCODING},
    {"m", InstructionOperand::MODRM_RM_ENCODING},
    {"m14byte", InstructionOperand::MODRM_RM_ENCODING},
    {"m14/28byte", InstructionOperand::MODRM_RM_ENCODING},
    {"m28byte", InstructionOperand::MODRM_RM_ENCODING},
    {"m16", InstructionOperand::MODRM_RM_ENCODING},
    {"m16&16", InstructionOperand::MODRM_RM_ENCODING},
    {"m16&32", InstructionOperand::MODRM_RM_ENCODING},
    {"m16&64", InstructionOperand::MODRM_RM_ENCODING},
    {"m16int", InstructionOperand::MODRM_RM_ENCODING},
    {"moffs16", InstructionOperand::IMMEDIATE_VALUE_ENCODING},
    {"m2byte", InstructionOperand::MODRM_RM_ENCODING},
    {"m14byte", InstructionOperand::MODRM_RM_ENCODING},
    {"m28byte", InstructionOperand::MODRM_RM_ENCODING},
    {"m32&32", InstructionOperand::MODRM_RM_ENCODING},
    {"moffs32", InstructionOperand::IMMEDIATE_VALUE_ENCODING},
    {"m32fp", InstructionOperand::MODRM_RM_ENCODING},
    {"m32int", InstructionOperand::MODRM_RM_ENCODING},
    {"moffs64", InstructionOperand::IMMEDIATE_VALUE_ENCODING},
    {"mem", InstructionOperand::MODRM_RM_ENCODING},
    // NOTE(ondrasej): Apart from string instructions, there are a couple of
    // "scalar" instructions that do accept an operand from modrm.rm, but they
    // do not allow it to be a register operand. Since we replace the operands
    // of the string instructions with different strings, we can depend on the
    // remaining m(8|16|32|64) to be an actuall ModR/M encoded operand.
    {"m64", InstructionOperand::MODRM_RM_ENCODING},
    {"m64fp", InstructionOperand::MODRM_RM_ENCODING},
    {"m64int", InstructionOperand::MODRM_RM_ENCODING},
    // NOTE(ondrasej): After removing operands of string instructions, all other
    // uses of m8 (as opposed to r/m8) are CLFLUSH and the PREFETCH*
    // instructions. All of these use modrm.rm encoding for the operand, and
    // they allow any addressing mode.
    {"m8", InstructionOperand::MODRM_RM_ENCODING},
    {"m80dec", InstructionOperand::MODRM_RM_ENCODING},
    {"m80bcd", InstructionOperand::MODRM_RM_ENCODING},
    {"m80fp", InstructionOperand::MODRM_RM_ENCODING},
    {"m128", InstructionOperand::MODRM_RM_ENCODING},
    {"m256", InstructionOperand::MODRM_RM_ENCODING},
    {"m512", InstructionOperand::MODRM_RM_ENCODING},
    {"m94byte", InstructionOperand::MODRM_RM_ENCODING},
    {"m94/108byte", InstructionOperand::MODRM_RM_ENCODING},
    {"m108byte", InstructionOperand::MODRM_RM_ENCODING},
    {"m512byte", InstructionOperand::MODRM_RM_ENCODING},
    {"mm/m32", InstructionOperand::MODRM_RM_ENCODING},
    {"mm/m64", InstructionOperand::MODRM_RM_ENCODING},
    {"mm2/m64", InstructionOperand::MODRM_RM_ENCODING},
    {"ptr16:16", InstructionOperand::IMMEDIATE_VALUE_ENCODING},
    {"ptr16:32", InstructionOperand::IMMEDIATE_VALUE_ENCODING},
    {"m16:16", InstructionOperand::MODRM_RM_ENCODING},
    {"m16:32", InstructionOperand::MODRM_RM_ENCODING},
    {"m16:64", InstructionOperand::MODRM_RM_ENCODING},
    {"r/m8", InstructionOperand::MODRM_RM_ENCODING},
    {"r/m16", InstructionOperand::MODRM_RM_ENCODING},
    {"r/m32", InstructionOperand::MODRM_RM_ENCODING},
    {"r/m64", InstructionOperand::MODRM_RM_ENCODING},
    {"r32/m8", InstructionOperand::MODRM_RM_ENCODING},
    {"r32/m16", InstructionOperand::MODRM_RM_ENCODING},
    {"r64/m8", InstructionOperand::MODRM_RM_ENCODING},
    {"r64/m16", InstructionOperand::MODRM_RM_ENCODING},
    {"reg/m16", InstructionOperand::MODRM_RM_ENCODING},
    {"reg/m32", InstructionOperand::MODRM_RM_ENCODING},
    {"reg/m8", InstructionOperand::MODRM_RM_ENCODING},
    // NOTE(ondrasej): In the 2015-09 version of the manual, segment registers
    // are always encoded using modrm.reg.
    {"Sreg", InstructionOperand::MODRM_REG_ENCODING},
    {"ST(0)", InstructionOperand::IMPLICIT_ENCODING},
    // NOTE(ondrasej): In the 2017-07 version of the manual, ST(i) registers
    // are always encoded in the opcode of the instruction, but it is always at
    // the end of second byte, so we generalize them to ModR/M bytes. Therefore
    // ST(i) registers get encoded in RM field.
    {"ST(i)", InstructionOperand::MODRM_RM_ENCODING},
    {"vm32x", InstructionOperand::VSIB_ENCODING},
    {"vm32y", InstructionOperand::VSIB_ENCODING},
    {"vm32z", InstructionOperand::VSIB_ENCODING},
    {"vm64x", InstructionOperand::VSIB_ENCODING},
    {"vm64y", InstructionOperand::VSIB_ENCODING},
    {"vm64z", InstructionOperand::VSIB_ENCODING},
    {"xmm/m8", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm/m16", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm/m32", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm/m64", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm/m128", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm1/m8", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm1/m16", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm1/m32", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm1/m64", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm1/m128", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm2/m8", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm2/m16", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm2/m32", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm2/m64", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm2/m64/m32bcst", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm2/m64/m64bcst", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm2/m128", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm2/m128/m32bcst", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm2/m128/m64bcst", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm3/m8", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm3/m16", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm3/m32", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm3/m64", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm3/m128", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm3/m128/m32bcst", InstructionOperand::MODRM_RM_ENCODING},
    {"xmm3/m128/m64bcst", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm/m8", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm/m16", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm/m32", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm/m64", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm/m128", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm/m256", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm1/m8", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm1/m16", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm1/m32", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm1/m64", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm1/m128", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm1/m256", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm2/m8", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm2/m16", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm2/m32", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm2/m64", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm2/m128", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm2/m256", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm3/m8", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm3/m16", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm3/m32", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm3/m64", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm3/m128", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm3/m256", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm3/m256/m32bcst", InstructionOperand::MODRM_RM_ENCODING},
    {"ymm3/m256/m64bcst", InstructionOperand::MODRM_RM_ENCODING},
    {"zmm1/m512", InstructionOperand::MODRM_RM_ENCODING},
    {"zmm2/m512", InstructionOperand::MODRM_RM_ENCODING},
    {"zmm3/m512", InstructionOperand::MODRM_RM_ENCODING},
    {"zmm2/m512/m32bcst", InstructionOperand::MODRM_RM_ENCODING},
    {"zmm2/m512/m64bcst", InstructionOperand::MODRM_RM_ENCODING},
    {"zmm3/m512/m32bcst", InstructionOperand::MODRM_RM_ENCODING},
    {"zmm3/m512/m64bcst", InstructionOperand::MODRM_RM_ENCODING},
    {"1", InstructionOperand::IMPLICIT_ENCODING},
    {"3", InstructionOperand::IMPLICIT_ENCODING}};

// Contains mapping from operand names to addressing modes they support. Note
// that where multiple addressing modes are supported, the most general category
// is chosen, and then we depend on another transform to fix it using additional
// information.
const std::pair<const char*, InstructionOperand::AddressingMode>
    kAddressingModeMap[] = {
        {"AL", InstructionOperand::DIRECT_ADDRESSING},
        {"AX", InstructionOperand::DIRECT_ADDRESSING},
        {"EAX", InstructionOperand::DIRECT_ADDRESSING},
        {"RAX", InstructionOperand::DIRECT_ADDRESSING},
        {"CL", InstructionOperand::DIRECT_ADDRESSING},
        {"CR0-CR7", InstructionOperand::DIRECT_ADDRESSING},
        {"CR8", InstructionOperand::DIRECT_ADDRESSING},
        {"DR0-DR7", InstructionOperand::DIRECT_ADDRESSING},
        {"CS", InstructionOperand::DIRECT_ADDRESSING},
        {"DS", InstructionOperand::DIRECT_ADDRESSING},
        {"ES", InstructionOperand::DIRECT_ADDRESSING},
        {"DX", InstructionOperand::DIRECT_ADDRESSING},
        {"FS", InstructionOperand::DIRECT_ADDRESSING},
        {"GS", InstructionOperand::DIRECT_ADDRESSING},
        {"SS", InstructionOperand::DIRECT_ADDRESSING},
        {"BYTE PTR [RSI]", InstructionOperand::INDIRECT_ADDRESSING_BY_RSI},
        {"WORD PTR [RSI]", InstructionOperand::INDIRECT_ADDRESSING_BY_RSI},
        {"DWORD PTR [RSI]", InstructionOperand::INDIRECT_ADDRESSING_BY_RSI},
        {"QWORD PTR [RSI]", InstructionOperand::INDIRECT_ADDRESSING_BY_RSI},
        {"BYTE PTR [RDI]", InstructionOperand::INDIRECT_ADDRESSING_BY_RDI},
        {"WORD PTR [RDI]", InstructionOperand::INDIRECT_ADDRESSING_BY_RDI},
        {"DWORD PTR [RDI]", InstructionOperand::INDIRECT_ADDRESSING_BY_RDI},
        {"QWORD PTR [RDI]", InstructionOperand::INDIRECT_ADDRESSING_BY_RDI},
        {"bnd", InstructionOperand::DIRECT_ADDRESSING},
        {"bnd0", InstructionOperand::DIRECT_ADDRESSING},
        {"bnd1", InstructionOperand::DIRECT_ADDRESSING},
        {"bnd2", InstructionOperand::DIRECT_ADDRESSING},
        {"bnd3", InstructionOperand::DIRECT_ADDRESSING},
        {"bnd1/m64",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"bnd1/m128",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"bnd2/m64",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"bnd2/m128",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"imm8", InstructionOperand::NO_ADDRESSING},
        {"imm16", InstructionOperand::NO_ADDRESSING},
        {"imm32", InstructionOperand::NO_ADDRESSING},
        {"imm64", InstructionOperand::NO_ADDRESSING},
        {"k1", InstructionOperand::DIRECT_ADDRESSING},
        {"k2", InstructionOperand::DIRECT_ADDRESSING},
        {"k3", InstructionOperand::DIRECT_ADDRESSING},
        {"k2/m8", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"k2/m16", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"k2/m32", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"k2/m64", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"rel8", InstructionOperand::NO_ADDRESSING},
        {"rel16", InstructionOperand::NO_ADDRESSING},
        {"rel32", InstructionOperand::NO_ADDRESSING},
        {"moffs8", InstructionOperand::NO_ADDRESSING},
        {"m", InstructionOperand::LOAD_EFFECTIVE_ADDRESS},
        {"m16", InstructionOperand::INDIRECT_ADDRESSING},
        {"m16&16", InstructionOperand::INDIRECT_ADDRESSING},
        {"m16&32", InstructionOperand::INDIRECT_ADDRESSING},
        {"m16&64", InstructionOperand::INDIRECT_ADDRESSING},
        {"m16int", InstructionOperand::INDIRECT_ADDRESSING},
        {"moffs16", InstructionOperand::NO_ADDRESSING},
        {"m2byte", InstructionOperand::INDIRECT_ADDRESSING},
        {"m14byte", InstructionOperand::INDIRECT_ADDRESSING},
        {"m14/28byte", InstructionOperand::INDIRECT_ADDRESSING},
        {"m28byte", InstructionOperand::INDIRECT_ADDRESSING},
        {"m32&32", InstructionOperand::INDIRECT_ADDRESSING},
        {"moffs32", InstructionOperand::NO_ADDRESSING},
        {"m32", InstructionOperand::INDIRECT_ADDRESSING},
        {"m32fp", InstructionOperand::INDIRECT_ADDRESSING},
        {"m32int", InstructionOperand::INDIRECT_ADDRESSING},
        {"moffs64", InstructionOperand::NO_ADDRESSING},
        {"mem", InstructionOperand::INDIRECT_ADDRESSING},
        // The manual mentions " a memory operand using SIB addressing form,
        // where the index register is not used in address calculation,
        // Scale is ignored. Only the base and displacement are used in
        // effective address calculation".
        {"mib",
         InstructionOperand::INDIRECT_ADDRESSING_WITH_BASE_AND_DISPLACEMENT},
        {"m64", InstructionOperand::INDIRECT_ADDRESSING},
        {"m64fp", InstructionOperand::INDIRECT_ADDRESSING},
        {"m64int", InstructionOperand::INDIRECT_ADDRESSING},
        {"m8", InstructionOperand::INDIRECT_ADDRESSING},
        {"m80dec", InstructionOperand::INDIRECT_ADDRESSING},
        {"m80bcd", InstructionOperand::INDIRECT_ADDRESSING},
        {"m80fp", InstructionOperand::INDIRECT_ADDRESSING},
        {"m128", InstructionOperand::INDIRECT_ADDRESSING},
        {"m256", InstructionOperand::INDIRECT_ADDRESSING},
        {"m512", InstructionOperand::INDIRECT_ADDRESSING},
        {"m94byte", InstructionOperand::INDIRECT_ADDRESSING},
        {"m94/108byte", InstructionOperand::INDIRECT_ADDRESSING},
        {"m108byte", InstructionOperand::INDIRECT_ADDRESSING},
        {"m512byte", InstructionOperand::INDIRECT_ADDRESSING},
        {"mm/m32", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"mm/m64", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"mm2/m64", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ptr16:16", InstructionOperand::NO_ADDRESSING},
        {"ptr16:32", InstructionOperand::NO_ADDRESSING},
        {"m16:16", InstructionOperand::INDIRECT_ADDRESSING},
        {"m16:32", InstructionOperand::INDIRECT_ADDRESSING},
        {"m16:64", InstructionOperand::INDIRECT_ADDRESSING},
        {"r/m8", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"r/m16", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"r/m32", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"r/m64", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"r32/m8", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"r32/m16", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"r64/m8", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"r64/m16", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"reg/m16", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"reg/m32", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"reg/m8", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"r8", InstructionOperand::DIRECT_ADDRESSING},
        {"r16", InstructionOperand::DIRECT_ADDRESSING},
        {"r32", InstructionOperand::DIRECT_ADDRESSING},
        {"r32a", InstructionOperand::DIRECT_ADDRESSING},
        {"r32b", InstructionOperand::DIRECT_ADDRESSING},
        {"r64", InstructionOperand::DIRECT_ADDRESSING},
        {"r64a", InstructionOperand::DIRECT_ADDRESSING},
        {"r64b", InstructionOperand::DIRECT_ADDRESSING},
        {"xmm", InstructionOperand::DIRECT_ADDRESSING},
        {"xmm0", InstructionOperand::DIRECT_ADDRESSING},
        {"xmm1", InstructionOperand::DIRECT_ADDRESSING},
        {"xmm2", InstructionOperand::DIRECT_ADDRESSING},
        {"xmm3", InstructionOperand::DIRECT_ADDRESSING},
        {"xmm4", InstructionOperand::DIRECT_ADDRESSING},
        {"ymm0", InstructionOperand::DIRECT_ADDRESSING},
        {"ymm1", InstructionOperand::DIRECT_ADDRESSING},
        {"ymm2", InstructionOperand::DIRECT_ADDRESSING},
        {"ymm3", InstructionOperand::DIRECT_ADDRESSING},
        {"ymm4", InstructionOperand::DIRECT_ADDRESSING},
        {"zmm0", InstructionOperand::DIRECT_ADDRESSING},
        {"zmm1", InstructionOperand::DIRECT_ADDRESSING},
        {"zmm2", InstructionOperand::DIRECT_ADDRESSING},
        {"zmm3", InstructionOperand::DIRECT_ADDRESSING},
        {"zmm4", InstructionOperand::DIRECT_ADDRESSING},
        {"mm", InstructionOperand::DIRECT_ADDRESSING},
        {"mm1", InstructionOperand::DIRECT_ADDRESSING},
        {"mm2", InstructionOperand::DIRECT_ADDRESSING},
        {"Sreg", InstructionOperand::DIRECT_ADDRESSING},
        {"ST(0)", InstructionOperand::DIRECT_ADDRESSING},
        {"ST(i)", InstructionOperand::DIRECT_ADDRESSING},
        {"vm32x", InstructionOperand::INDIRECT_ADDRESSING_WITH_VSIB},
        {"vm32y", InstructionOperand::INDIRECT_ADDRESSING_WITH_VSIB},
        {"vm32z", InstructionOperand::INDIRECT_ADDRESSING_WITH_VSIB},
        {"vm64x", InstructionOperand::INDIRECT_ADDRESSING_WITH_VSIB},
        {"vm64y", InstructionOperand::INDIRECT_ADDRESSING_WITH_VSIB},
        {"vm64z", InstructionOperand::INDIRECT_ADDRESSING_WITH_VSIB},
        {"xmm/m8", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm/m16", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm/m32", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm/m64", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm/m128",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm1/m8", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm1/m16",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm1/m32",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm1/m64",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm1/m128",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm2/m8", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm2/m16",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm2/m32",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm2/m64",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm2/m64/m32bcst",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm2/m64/m64bcst",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm2/m128",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm2/m128/m32bcst",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm2/m128/m64bcst",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm3/m8", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm3/m16",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm3/m32",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm3/m64",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm3/m128",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm3/m128/m32bcst",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm3/m128/m64bcst",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"xmm2+3", InstructionOperand::BLOCK_DIRECT_ADDRESSING},
        {"ymm/m8", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm/m16", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm/m32", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm/m64", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm/m128",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm/m256",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm1/m8", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm1/m16",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm1/m32",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm1/m64",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm1/m128",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm1/m256",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm2/m8", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm2/m16",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm2/m32",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm2/m64",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm2/m128",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm2/m256",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm2/m256/m32bcst",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm2/m256/m64bcst",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm2+3", InstructionOperand::BLOCK_DIRECT_ADDRESSING},
        {"ymm3/m8", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm3/m16",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm3/m32",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm3/m64",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm3/m128",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm3/m256",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm3/m256/m32bcst",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"ymm3/m256/m64bcst",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm1/m8", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm1/m16",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm1/m32",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm1/m64",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm1/m128",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm1/m256",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm1/m512",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm1/m512/m32bcst",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm1/m512/m64bcst",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm2/m8", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm2/m16",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm2/m32",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm2/m64",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm2/m128",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm2/m256",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm2/m512",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm2/m512/m32bcst",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm2/m512/m64bcst",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm2+3", InstructionOperand::BLOCK_DIRECT_ADDRESSING},
        {"zmm3/m8", InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm3/m16",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm3/m32",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm3/m64",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm3/m128",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm3/m256",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm3/m512",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm3/m512/m32bcst",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"zmm3/m512/m64bcst",
         InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS},
        {"1", InstructionOperand::NO_ADDRESSING},
        {"3", InstructionOperand::NO_ADDRESSING}};

// Contains mapping from operand names to the size of the values in bits. This
// map contains the value sizes only when the value is clearly determined by the
// operand. Also note that in case of indirect addressing, this map contains the
// size of the value, not the address.
const std::pair<const char*, uint32_t> kOperandValueSizeBitsMap[] = {
    {"AL", 8},
    {"AX", 16},
    {"EAX", 32},
    {"RAX", 64},
    {"CL", 8},
    {"CR0-CR7", 64},
    {"CR8", 64},
    {"DR0-DR7", 64},
    {"CS", 16},
    {"DS", 16},
    {"ES", 16},
    {"DX", 16},
    {"FS", 16},
    {"GS", 16},
    {"SS", 16},
    {"BYTE PTR [RSI]", 8},
    {"WORD PTR [RSI]", 16},
    {"DWORD PTR [RSI]", 32},
    {"QWORD PTR [RSI]", 64},
    {"BYTE PTR [RDI]", 8},
    {"WORD PTR [RDI]", 16},
    {"DWORD PTR [RDI]", 32},
    {"QWORD PTR [RDI]", 64},
    {"bnd", 128},
    {"bnd1", 128},
    {"bnd2", 128},
    {"imm8", 8},
    {"imm16", 16},
    {"imm32", 32},
    {"imm64", 64},
    {"k1", 64},
    {"k2", 64},
    {"k3", 64},
    {"k2/m8", 8},
    {"k2/m16", 16},
    {"k2/m32", 32},
    {"k2/m64", 64},
    {"moffs8", 8},
    {"m16", 16},
    {"m16&16", 32},
    {"m16&32", 48},
    {"m16&64", 80},
    {"m16int", 16},
    {"moffs16", 16},
    {"m2byte", 16},
    {"m14byte", 14 * 8},
    {"m14/28byte", 28 * 8},
    {"m28byte", 28 * 8},
    {"m32&32", 64},
    {"moffs32", 32},
    {"m32", 32},
    {"m32fp", 32},
    {"m32int", 32},
    {"moffs64", 64},
    {"m64", 64},
    {"m64fp", 64},
    {"m64int", 64},
    {"m8", 8},
    {"m80dec", 80},
    {"m80bcd", 80},
    {"m80fp", 80},
    {"m128", 128},
    {"m256", 256},
    {"m512", 512},
    {"m94byte", 94 * 8},
    {"m94/108byte", 108 * 8},
    {"m108byte", 108 * 8},
    {"m512byte", 512 * 8},
    {"mm/m32", 32},
    {"mm/m64", 64},
    {"mm2/m64", 64},
    {"m16:16", 32},
    {"m16:32", 48},
    {"m16:64", 80},
    {"mib", 128},
    {"rel8", 8},
    {"rel16", 16},
    {"rel32", 32},
    {"r/m8", 8},
    {"r/m16", 16},
    {"r/m32", 32},
    {"r/m64", 64},
    {"r32/m8", 8},
    {"r32/m16", 16},
    {"r64/m8", 8},
    {"r64/m16", 16},
    {"reg/m16", 16},
    {"reg/m32", 32},
    {"reg/m8", 8},
    {"r8", 8},
    {"r16", 16},
    {"r32", 32},
    {"r32a", 32},
    {"r32b", 32},
    {"r64", 64},
    {"r64a", 64},
    {"r64b", 64},
    {"xmm", 128},
    {"xmm0", 128},
    {"xmm1", 128},
    {"xmm2", 128},
    {"xmm3", 128},
    {"xmm4", 128},
    {"ymm0", 256},
    {"ymm1", 256},
    {"ymm2", 256},
    {"ymm3", 256},
    {"ymm4", 256},
    {"zmm0", 512},
    {"zmm1", 512},
    {"zmm2", 512},
    {"zmm3", 512},
    {"zmm4", 512},
    {"mm", 64},
    {"mm1", 64},
    {"mm2", 64},
    {"Sreg", 16},
    {"ST(0)", 80},
    {"ST(i)", 80},
    {"xmm/m8", 8},
    {"xmm/m16", 16},
    {"xmm/m32", 32},
    {"xmm/m64", 64},
    {"xmm/m128", 128},
    {"xmm1/m8", 8},
    {"xmm1/m16", 16},
    {"xmm1/m32", 32},
    {"xmm1/m64", 64},
    {"xmm1/m128", 128},
    {"xmm2/m8", 8},
    {"xmm2/m16", 16},
    {"xmm2/m32", 32},
    {"xmm2/m64", 64},
    {"xmm2/m128", 128},
    {"xmm3/m8", 8},
    {"xmm3/m16", 16},
    {"xmm3/m32", 32},
    {"xmm3/m64", 64},
    {"xmm3/m128", 128},
    {"xmm2+3", 512},
    {"ymm/m8", 8},
    {"ymm/m16", 16},
    {"ymm/m32", 32},
    {"ymm/m64", 64},
    {"ymm/m128", 128},
    {"ymm/m256", 256},
    {"ymm1/m8", 8},
    {"ymm1/m16", 16},
    {"ymm1/m32", 32},
    {"ymm1/m64", 64},
    {"ymm1/m128", 128},
    {"ymm1/m256", 256},
    {"ymm2/m8", 8},
    {"ymm2/m16", 16},
    {"ymm2/m32", 32},
    {"ymm2/m64", 64},
    {"ymm2/m128", 128},
    {"ymm2/m256", 256},
    {"ymm2+3", 1024},
    {"ymm3/m8", 8},
    {"ymm3/m16", 16},
    {"ymm3/m32", 32},
    {"ymm3/m64", 64},
    {"ymm3/m128", 128},
    {"ymm3/m256", 256},
    {"zmm1/m8", 8},
    {"zmm1/m16", 16},
    {"zmm1/m32", 32},
    {"zmm1/m64", 64},
    {"zmm1/m128", 128},
    {"zmm1/m256", 256},
    {"zmm1/m512", 512},
    {"zmm2/m8", 8},
    {"zmm2/m16", 16},
    {"zmm2/m32", 32},
    {"zmm2/m64", 64},
    {"zmm2/m128", 128},
    {"zmm2/m256", 256},
    {"zmm2/m512", 512},
    {"zmm3/m8", 8},
    {"zmm3/m16", 16},
    {"zmm3/m32", 32},
    {"zmm3/m64", 64},
    {"zmm3/m128", 128},
    {"zmm3/m256", 256},
    {"zmm3/m512", 512},
    {"zmm2+3", 2048},
};

constexpr const std::pair<const char*, RegisterProto::RegisterClass>
    kRegisterClassMap[] = {
        {"AL", RegisterProto::GENERAL_PURPOSE_REGISTER_8_BIT},
        {"CL", RegisterProto::GENERAL_PURPOSE_REGISTER_8_BIT},
        {"AX", RegisterProto::GENERAL_PURPOSE_REGISTER_16_BIT},
        {"DX", RegisterProto::GENERAL_PURPOSE_REGISTER_16_BIT},
        {"EAX", RegisterProto::GENERAL_PURPOSE_REGISTER_32_BIT},
        {"RAX", RegisterProto::GENERAL_PURPOSE_REGISTER_64_BIT},

        {"r8", RegisterProto::GENERAL_PURPOSE_REGISTER_8_BIT},
        {"r16", RegisterProto::GENERAL_PURPOSE_REGISTER_16_BIT},
        {"r32", RegisterProto::GENERAL_PURPOSE_REGISTER_32_BIT},
        {"r64", RegisterProto::GENERAL_PURPOSE_REGISTER_64_BIT},

        {"m", RegisterProto::INVALID_REGISTER_CLASS},
        {"mem", RegisterProto::INVALID_REGISTER_CLASS},

        {"m8", RegisterProto::INVALID_REGISTER_CLASS},
        {"m16", RegisterProto::INVALID_REGISTER_CLASS},
        {"m32", RegisterProto::INVALID_REGISTER_CLASS},
        {"m64", RegisterProto::INVALID_REGISTER_CLASS},
        {"m128", RegisterProto::INVALID_REGISTER_CLASS},
        {"m256", RegisterProto::INVALID_REGISTER_CLASS},
        {"m512", RegisterProto::INVALID_REGISTER_CLASS},

        {"mib", RegisterProto::INVALID_REGISTER_CLASS},

        {"mm", RegisterProto::MMX_STACK_REGISTER},
        {"mm1", RegisterProto::MMX_STACK_REGISTER},
        {"mm2", RegisterProto::MMX_STACK_REGISTER},

        {"ST(0)", RegisterProto::FLOATING_POINT_STACK_REGISTER},
        {"ST(i)", RegisterProto::FLOATING_POINT_STACK_REGISTER},

        {"m16:16", RegisterProto::INVALID_REGISTER_CLASS},
        {"m16:32", RegisterProto::INVALID_REGISTER_CLASS},
        {"m16:64", RegisterProto::INVALID_REGISTER_CLASS},

        {"m16&16", RegisterProto::INVALID_REGISTER_CLASS},
        {"m16&32", RegisterProto::INVALID_REGISTER_CLASS},
        {"m16&64", RegisterProto::INVALID_REGISTER_CLASS},
        {"m32&32", RegisterProto::INVALID_REGISTER_CLASS},

        {"m32fp", RegisterProto::INVALID_REGISTER_CLASS},
        {"m64fp", RegisterProto::INVALID_REGISTER_CLASS},

        {"m16int", RegisterProto::INVALID_REGISTER_CLASS},
        {"m32int", RegisterProto::INVALID_REGISTER_CLASS},
        {"m64int", RegisterProto::INVALID_REGISTER_CLASS},

        {"m80fp", RegisterProto::INVALID_REGISTER_CLASS},

        {"imm8", RegisterProto::INVALID_REGISTER_CLASS},
        {"imm16", RegisterProto::INVALID_REGISTER_CLASS},
        {"imm32", RegisterProto::INVALID_REGISTER_CLASS},
        {"imm64", RegisterProto::INVALID_REGISTER_CLASS},

        {"moffs8", RegisterProto::INVALID_REGISTER_CLASS},
        {"moffs16", RegisterProto::INVALID_REGISTER_CLASS},
        {"moffs32", RegisterProto::INVALID_REGISTER_CLASS},
        {"moffs64", RegisterProto::INVALID_REGISTER_CLASS},

        {"xmm", RegisterProto::VECTOR_REGISTER_128_BIT},
        {"xmm0", RegisterProto::VECTOR_REGISTER_128_BIT},
        {"xmm1", RegisterProto::VECTOR_REGISTER_128_BIT},
        {"xmm2", RegisterProto::VECTOR_REGISTER_128_BIT},
        {"xmm3", RegisterProto::VECTOR_REGISTER_128_BIT},
        {"xmm4", RegisterProto::VECTOR_REGISTER_128_BIT},

        {"xmm2+3", RegisterProto::REGISTER_BLOCK_128_BIT},

        {"ymm", RegisterProto::VECTOR_REGISTER_256_BIT},
        {"ymm1", RegisterProto::VECTOR_REGISTER_256_BIT},
        {"ymm2", RegisterProto::VECTOR_REGISTER_256_BIT},
        {"ymm3", RegisterProto::VECTOR_REGISTER_256_BIT},
        {"ymm4", RegisterProto::VECTOR_REGISTER_256_BIT},

        {"ymm2+3", RegisterProto::REGISTER_BLOCK_256_BIT},

        {"zmm", RegisterProto::VECTOR_REGISTER_512_BIT},
        {"zmm1", RegisterProto::VECTOR_REGISTER_512_BIT},
        {"zmm2", RegisterProto::VECTOR_REGISTER_512_BIT},
        {"zmm3", RegisterProto::VECTOR_REGISTER_512_BIT},

        {"zmm2+3", RegisterProto::REGISTER_BLOCK_512_BIT},

        {"k", RegisterProto::MASK_REGISTER},
        {"k1", RegisterProto::MASK_REGISTER},
        {"k2", RegisterProto::MASK_REGISTER},
        {"k3", RegisterProto::MASK_REGISTER},

        {"bnd", RegisterProto::SPECIAL_REGISTER_MPX_BOUNDS},
        {"bnd1", RegisterProto::SPECIAL_REGISTER_MPX_BOUNDS},
        {"bnd2", RegisterProto::SPECIAL_REGISTER_MPX_BOUNDS},

        {"BYTE PTR [RSI]", RegisterProto::INVALID_REGISTER_CLASS},
        {"BYTE PTR [RDI]", RegisterProto::INVALID_REGISTER_CLASS},
        {"WORD PTR [RSI]", RegisterProto::INVALID_REGISTER_CLASS},
        {"WORD PTR [RDI]", RegisterProto::INVALID_REGISTER_CLASS},
        {"DWORD PTR [RSI]", RegisterProto::INVALID_REGISTER_CLASS},
        {"DWORD PTR [RDI]", RegisterProto::INVALID_REGISTER_CLASS},
        {"QWORD PTR [RSI]", RegisterProto::INVALID_REGISTER_CLASS},
        {"QWORD PTR [RDI]", RegisterProto::INVALID_REGISTER_CLASS},

        {"rel8", RegisterProto::INVALID_REGISTER_CLASS},
        {"rel16", RegisterProto::INVALID_REGISTER_CLASS},
        {"rel32", RegisterProto::INVALID_REGISTER_CLASS},

        {"CR0-CR7", RegisterProto::SPECIAL_REGISTER_CONTROL},

        {"DR0-DR7", RegisterProto::SPECIAL_REGISTER_DEBUG},

        {"FS", RegisterProto::SPECIAL_REGISTER_SEGMENT},
        {"GS", RegisterProto::SPECIAL_REGISTER_SEGMENT},
        {"Sreg", RegisterProto::SPECIAL_REGISTER_SEGMENT},

        {"vm32x", RegisterProto::INVALID_REGISTER_CLASS},
        {"vm32y", RegisterProto::INVALID_REGISTER_CLASS},
        {"vm32z", RegisterProto::INVALID_REGISTER_CLASS},
        {"vm64x", RegisterProto::INVALID_REGISTER_CLASS},
        {"vm64y", RegisterProto::INVALID_REGISTER_CLASS},
        {"vm64z", RegisterProto::INVALID_REGISTER_CLASS},

        // Some operands are nameless.
        {"", RegisterProto::INVALID_REGISTER_CLASS},

        // There were no specifications for the ones below in the manual
        // versions I looked at, but they are still used with a few
        // instructions. Assumed what they are.
        {"m80bcd", RegisterProto::INVALID_REGISTER_CLASS},

        {"r32a", RegisterProto::GENERAL_PURPOSE_REGISTER_32_BIT},
        {"r32b", RegisterProto::GENERAL_PURPOSE_REGISTER_32_BIT},
        {"r64a", RegisterProto::GENERAL_PURPOSE_REGISTER_64_BIT},
        {"r64b", RegisterProto::GENERAL_PURPOSE_REGISTER_64_BIT},

        {"m2byte", RegisterProto::INVALID_REGISTER_CLASS},
        {"m14byte", RegisterProto::INVALID_REGISTER_CLASS},
        {"m28byte", RegisterProto::INVALID_REGISTER_CLASS},
        {"m94byte", RegisterProto::INVALID_REGISTER_CLASS},
        {"m108byte", RegisterProto::INVALID_REGISTER_CLASS},
        {"m512byte", RegisterProto::INVALID_REGISTER_CLASS},

        {"1", RegisterProto::INVALID_REGISTER_CLASS},
        {"3", RegisterProto::INVALID_REGISTER_CLASS}};

}  // namespace

namespace {

bool IsImplicitOperandEncoding(const InstructionOperand::Encoding& encoding) {
  return InCategory(encoding, InstructionOperand::IMPLICIT_ENCODING);
}

// Tries to remove one occurrence of the operand encoding of 'operand' from
// 'available_encodings'. If it is removed, returns Status::OK. If
// 'available_encodings' does not contain such an encoding, returns an error
// status with an appropriate error message.
Status EraseOperandEncoding(
    const InstructionProto& instruction, const InstructionOperand& operand,
    InstructionOperandEncodingMultiset* available_encodings) {
  const InstructionOperand::Encoding encoding = operand.encoding();
  Status status = OkStatus();
  if (!IsImplicitOperandEncoding(encoding)) {
    const auto iterator = available_encodings->find(encoding);
    if (iterator == available_encodings->end()) {
      status = InvalidArgumentError(absl::StrCat(
          "Operand '", operand.name(), "' encoded using ",
          InstructionOperand::Encoding_Name(encoding),
          " is not specified in the encoding specification: ",
          instruction.raw_encoding_specification(),
          ", mnemonic: ", GetAnyVendorSyntaxOrDie(instruction).mnemonic()));
      LOG(WARNING) << status;
    } else {
      available_encodings->erase(iterator);
    }
  }
  return status;
}

// Assigns addressing mode to all operands of the instruction, and encoding and
// value size to operands where the encoding is uniquely determined by the
// operand. This is the case for example for operands that can be a memory
// reference, or that are immediate values.
//
// Adds the indices of all unassigned operands to 'operands_with_no_encoding',
// and checks that the uniquely determined encodings are all in
// 'available_encodings'. The function also removes all encodings it uses from
// 'available_encodings'.
//
// Returns an error if the addressing mode for an operand is not known, or the
// uniquely determined enoding does not appear in 'available_encodings'.
Status AssignOperandPropertiesWhereUniquelyDetermined(
    const InstructionProto& instruction, InstructionFormat* vendor_syntax,
    InstructionOperandEncodingMultiset* available_encodings,
    std::vector<int>* operands_with_no_encoding) {
  CHECK(vendor_syntax != nullptr);
  CHECK(available_encodings != nullptr);
  CHECK(operands_with_no_encoding != nullptr);
  static const AddressingModeMap* const addressing_mode_map =
      new AddressingModeMap(std::begin(kAddressingModeMap),
                            std::end(kAddressingModeMap));
  static const EncodingMap* const encoding_map =
      new EncodingMap(std::begin(kEncodingMap), std::end(kEncodingMap));
  static const ValueSizeMap* const value_size_map = new ValueSizeMap(
      std::begin(kOperandValueSizeBitsMap), std::end(kOperandValueSizeBitsMap));

  Status status = OkStatus();
  for (int operand_index = 0; operand_index < vendor_syntax->operands_size();
       ++operand_index) {
    InstructionOperand* const operand =
        vendor_syntax->mutable_operands(operand_index);

    if (operand->addressing_mode() == InstructionOperand::ANY_ADDRESSING_MODE) {
      InstructionOperand::AddressingMode addressing_mode;
      if (!gtl::FindCopy(*addressing_mode_map, operand->name(),
                         &addressing_mode)) {
        status = InvalidArgumentError(absl::StrCat(
            "Could not determine addressing mode of operand: ", operand->name(),
            ", instruction ", vendor_syntax->mnemonic(), " (",
            instruction.raw_encoding_specification(), ")"));
        LOG(ERROR) << status;
        continue;
      }
      operand->set_addressing_mode(addressing_mode);
    }

    uint32_t value_size_bits = 0;
    if (gtl::FindCopy(*value_size_map, operand->name(), &value_size_bits)) {
      operand->set_value_size_bits(value_size_bits);
    }

    if (operand->encoding() != InstructionOperand::ANY_ENCODING) {
      UpdateStatus(&status, EraseOperandEncoding(instruction, *operand,
                                                 available_encodings));
    } else {
      // If there is only one way how an operand can be encoded, we assign this
      // encoding to the operand and remove it from the list of available
      // encodings. Then we'll need to assign encodings to the remaining
      // operands only from those "remaining" encodings.
      InstructionOperand::Encoding operand_encoding;
      if (gtl::FindCopy(*encoding_map, operand->name(), &operand_encoding)) {
        operand->set_encoding(operand_encoding);
        UpdateStatus(&status, EraseOperandEncoding(instruction, *operand,
                                                   available_encodings));
      } else {
        operands_with_no_encoding->push_back(operand_index);
      }
    }
  }
  return status;
}

// Assigns the encoding 'encoding' to 'operand' if the encoding is present in
// 'available_encodings'. If successful, removes one copy of the encoding from
// 'available_encodings' and returns true. Otherwise, returns false.
inline bool AssignEncodingIfAvailable(
    InstructionOperand* operand, InstructionOperand::Encoding encoding,
    InstructionOperandEncodingMultiset* available_encodings) {
  if (gtl::ContainsKey(*available_encodings, encoding)) {
    operand->set_encoding(encoding);
    available_encodings->erase(encoding);
    return true;
  }
  return false;
}

// Assigns encoding to operands based on the encoding_scheme string. This string
// is specified in the Intel manual, and it often contains as many characters as
// there are operands, and the characters in the string correspond to the actual
// way how the operands are encoded. This function uses this string as a
// heuristic, and if it can find a match, it assigns the encoding to the
// operand. Otherwise, it simply leaves the operands unassigned.
// The following characters used in the encoding scheme usually have a clear
// interpretation:
// 0 - implicit XMM0,
// I - immediate value,
// M - modrm.rm,
// R - modrm.reg or VEX suffix operand; if the VEX suffix operand is used,
//     it is typically the last operand of the instruction,
// V - vex.vvvv.
// X - modrm.reg (a special case, used only for VMOVSS and VMOVSD).
//
// TODO(ondrasej): The manual actually contains a more detailed definition of
// each encoding scheme, but they are instruction specific and we do not have
// this information available in a machine-readable format. Ideally, our
// assignments should be based on this information. However, for now it looks
// like the heuristics are good enough to let us assign the operands as we need
// them, and so far, we do not need to know the exact matching of operand
// positions and encodings, only what encodings are used.
Status AssignEncodingByEncodingScheme(
    const InstructionProto& instruction,
    const std::vector<int>& operands_with_no_encoding,
    InstructionFormat* vendor_syntax,
    InstructionOperandEncodingMultiset* available_encodings) {
  CHECK(vendor_syntax != nullptr);
  const std::string& encoding_scheme = instruction.encoding_scheme();
  if (encoding_scheme.length() >= vendor_syntax->operands_size()) {
    for (const int operand_index : operands_with_no_encoding) {
      InstructionOperand* const operand =
          vendor_syntax->mutable_operands(operand_index);
      switch (encoding_scheme[operand_index]) {
        case 'M':
          AssignEncodingIfAvailable(operand,
                                    InstructionOperand::MODRM_RM_ENCODING,
                                    available_encodings);
          break;
        case 'R':
          if (!AssignEncodingIfAvailable(operand,
                                         InstructionOperand::MODRM_REG_ENCODING,
                                         available_encodings)) {
            AssignEncodingIfAvailable(operand,
                                      InstructionOperand::VEX_SUFFIX_ENCODING,
                                      available_encodings);
          }
          break;
        case 'V':
          AssignEncodingIfAvailable(operand, InstructionOperand::VEX_V_ENCODING,
                                    available_encodings);
          break;
        case 'X':
          AssignEncodingIfAvailable(operand,
                                    InstructionOperand::MODRM_REG_ENCODING,
                                    available_encodings);
          break;
        default:
          LOG(WARNING) << "Unknown encoding scheme : \n"
                       << instruction.DebugString();
      }
    }
  }
  return OkStatus();
}

// Assigns the remaining available encodings to the remaining unassigned
// operands on a first come first served basis. Assumes that there are enough
// available encodings for all remaining operands.
Status AssignEncodingRandomlyFromAvailableEncodings(
    const InstructionProto& instruction, InstructionFormat* vendor_syntax,
    InstructionOperandEncodingMultiset* available_encodings) {
  CHECK(vendor_syntax != nullptr);
  for (InstructionOperand& operand : *vendor_syntax->mutable_operands()) {
    if (operand.encoding() == InstructionOperand::ANY_ENCODING) {
      if (available_encodings->empty()) {
        return InvalidArgumentError(
            absl::StrCat("No available encodings for instruction:\n",
                         instruction.DebugString()));
      }
      operand.set_encoding(*available_encodings->begin());
      available_encodings->erase(available_encodings->begin());
    }
  }
  return OkStatus();
}

}  // namespace

Status AddVmxOperandInfo(InstructionSetProto* instruction_set) {
  static constexpr LazyRE2 kRegex = {R"(.*/[r0-7])"};

  CHECK(instruction_set != nullptr);
  for (auto& instruction : *instruction_set->mutable_instructions()) {
    // We only need to add the trailing /r to VMX instructions that have at
    // least one argument and whose encoding_spec doesn't end in "/[0-7]".
    if (instruction.feature_name() == "VMX" &&
        GetVendorSyntaxWithMostOperandsOrDie(instruction).operands_size() > 0 &&
        !RE2::FullMatch(instruction.raw_encoding_specification(), *kRegex)) {
      *instruction.mutable_raw_encoding_specification() =
          absl::StrCat(instruction.raw_encoding_specification(), " /r");
    }
  }
  return util::OkStatus();
}
// We want this to run early so that VMX instructions' operands will benefit
// from other cleanups.
REGISTER_INSTRUCTION_SET_TRANSFORM(AddVmxOperandInfo, 999);

Status FixVmFuncOperandInfo(InstructionSetProto* instruction_set) {
  static constexpr char kDescriptionForVmfunc[] = "VM Function to be invoked.";
  static constexpr char kVmfuncOpcode[] = "NP 0F 01 D4";
  CHECK(instruction_set != nullptr);
  for (auto& instruction : *instruction_set->mutable_instructions()) {
    if (instruction.raw_encoding_specification() == kVmfuncOpcode) {
      DCHECK_EQ("VMX", instruction.feature_name());
      auto& vendor_syntax = GetVendorSyntaxWithMostOperandsOrDie(instruction);
      DCHECK_EQ("VMFUNC", vendor_syntax.mnemonic());
      DCHECK_EQ(1, instruction.vendor_syntax_size());
      DCHECK_EQ(0, instruction.vendor_syntax(0).operands_size());
      auto* const operand =
          instruction.mutable_vendor_syntax(0)->add_operands();
      operand->set_name("EAX");
      operand->set_usage(InstructionOperand::USAGE_READ);
      operand->set_addressing_mode(
          InstructionOperand::ANY_ADDRESSING_WITH_FIXED_REGISTERS);
      operand->set_encoding(InstructionOperand::X86_REGISTER_EAX);
      operand->set_description(kDescriptionForVmfunc);
      break;
    }
  }
  return util::OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(FixVmFuncOperandInfo, 998);

Status AddMovdir64BOperandInfo(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  constexpr absl::string_view kMovdir64B = "66 0F 38 F8 /r";
  constexpr int kExpectedNumOperands = 2;
  constexpr absl::string_view kDestinationOperandName = "r16/r32/r64";
  for (auto& instruction : *instruction_set->mutable_instructions()) {
    if (instruction.raw_encoding_specification() != kMovdir64B) continue;
    InstructionFormat& vendor_syntax =
        *GetOrAddUniqueVendorSyntaxOrDie(&instruction);
    if (vendor_syntax.operands_size() != kExpectedNumOperands) {
      return InvalidArgumentError(
          absl::StrCat("Unexpected number of operands of MOVDIR64B: ",
                       vendor_syntax.operands_size()));
    }
    InstructionOperand& destination_operand =
        *vendor_syntax.mutable_operands(0);
    if (destination_operand.name() != kDestinationOperandName) {
      return InvalidArgumentError(
          absl::StrCat("Unexpected MOVDIR64B destination operand name: ",
                       destination_operand.name()));
    }
    destination_operand.set_name("m64");
    destination_operand.set_addressing_mode(
        InstructionOperand::INDIRECT_ADDRESSING_WITH_BASE);
    destination_operand.set_value_size_bits(512);
    destination_operand.set_register_class(
        RegisterProto::INVALID_REGISTER_CLASS);
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddMovdir64BOperandInfo, 999);

Status AddUmonitorOperandInfo(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  constexpr absl::string_view kUmonitorEncoding = "F3 0F AE /6";
  constexpr absl::string_view kUmonitorMnemonic = "UMONITOR";
  constexpr int kExpectedNumOperands = 1;
  constexpr absl::string_view kOperandName = "r16/r32/r64";
  for (auto& instruction : *instruction_set->mutable_instructions()) {
    if (instruction.raw_encoding_specification() != kUmonitorEncoding) continue;
    InstructionFormat& vendor_syntax =
        *GetOrAddUniqueVendorSyntaxOrDie(&instruction);
    // In the October 2019 version of the SDM, UMONITOR has exactly the same
    // encoding as CLRSSBSY; we need to use the mnemonic to distinguish between
    // them.
    if (vendor_syntax.mnemonic() != kUmonitorMnemonic) continue;
    if (vendor_syntax.operands_size() != kExpectedNumOperands) {
      return InvalidArgumentError(
          absl::StrCat("Unexpected number of operands of UMONITOR: ",
                       vendor_syntax.operands_size()));
    }
    InstructionOperand& destination_operand =
        *vendor_syntax.mutable_operands(0);
    if (destination_operand.name() != kOperandName) {
      return InvalidArgumentError(absl::StrCat(
          "Unexpected UMONITOR operand name: ", destination_operand.name()));
    }
    destination_operand.set_name("mem");
    destination_operand.set_addressing_mode(
        InstructionOperand::INDIRECT_ADDRESSING_WITH_BASE);
    destination_operand.set_value_size_bits(8);
    destination_operand.set_register_class(
        RegisterProto::INVALID_REGISTER_CLASS);
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddUmonitorOperandInfo, 999);

Status AddRegisterClassToOperands(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  const RegisterClassMap register_class_map(std::begin(kRegisterClassMap),
                                            std::end(kRegisterClassMap));
  for (auto& instruction : *instruction_set->mutable_instructions()) {
    for (InstructionFormat& vendor_syntax :
         *instruction.mutable_vendor_syntax()) {
      for (InstructionOperand& operand : *vendor_syntax.mutable_operands()) {
        const RegisterProto::RegisterClass* const register_class =
            gtl::FindOrNull(register_class_map, operand.name());
        if (register_class == nullptr) {
          return InvalidArgumentError(
              absl::StrCat("Unexpected operand name:", operand.name(),
                           "\nInstruction:", instruction.DebugString()));
        } else {
          operand.set_register_class(*register_class);
        }
      }
    }
  }
  return OkStatus();
}

// We are running this after alternatives transform has ran. Because an
// operand with name r/m32 is ambigious. It can use both a 32 bit general
// purpose register with direct addressing or use a 64 bit general purpose
// register to perform indirect accessing.
REGISTER_INSTRUCTION_SET_TRANSFORM(AddRegisterClassToOperands, 7000);

Status AddOperandInfoToSyntax(const InstructionProto& instruction,
                              InstructionFormat* vendor_syntax) {
  CHECK(vendor_syntax != nullptr);
  if (!instruction.has_x86_encoding_specification()) {
    return FailedPreconditionError(absl::StrCat(
        "Instruction does not have a parsed encoding specification: ",
        instruction.DebugString()));
  }
  InstructionOperandEncodingMultiset available_encodings =
      GetAvailableEncodings(instruction.x86_encoding_specification());

  // First assign the addressing modes and the encodings that can be determined
  // from the operand itself.
  std::vector<int> operands_with_no_encoding;
  RETURN_IF_ERROR(AssignOperandPropertiesWhereUniquelyDetermined(
      instruction, vendor_syntax, &available_encodings,
      &operands_with_no_encoding));

  if (!operands_with_no_encoding.empty()) {
    // There are some operands that were not assigned the encoding just from the
    // name of the operand. We need to use a more sophisticated process.
    if (operands_with_no_encoding.size() == 1 &&
        available_encodings.size() == 1) {
      // There is just one operand where we need to assign the encoding, and
      // only one available encoding, so we simply match them. In theory, the
      // following branch should catch this case, but it doesn't work correctly
      // because some instructions of this type do not use the usual
      // encoding_scheme conventions, but we can correctly handle them using
      // this heuristic.
      InstructionOperand* const operand =
          vendor_syntax->mutable_operands(operands_with_no_encoding.front());
      operand->set_encoding(*available_encodings.begin());
    } else if (operands_with_no_encoding.size() <= available_encodings.size()) {
      // We have enough available encodings to assign to the remaining
      // operands. First try to use the encoding scheme as a guide, and if
      // that fails, we just assign the remaining available encodings to the
      // remaining operands randomly.
      RETURN_IF_ERROR(
          AssignEncodingByEncodingScheme(instruction, operands_with_no_encoding,
                                         vendor_syntax, &available_encodings));
      RETURN_IF_ERROR(AssignEncodingRandomlyFromAvailableEncodings(
          instruction, vendor_syntax, &available_encodings));
    } else {
      VLOG(1) << "operands_with_no_encoding:";
      for (const int index : operands_with_no_encoding) {
        VLOG(1) << "  " << index;
      }
      VLOG(1) << "available_encodings:";
      for (const InstructionOperand::Encoding available_encoding :
           available_encodings) {
        VLOG(1) << "  "
                << InstructionOperand::Encoding_Name(available_encoding);
      }
      // We don't have enough available encodings to encode all the
      // operands.
      const Status status = InvalidArgumentError(absl::StrCat(
          "There are more operands remaining than available encodings: ",
          instruction.DebugString()));
      LOG(ERROR) << status;
      return status;
    }
  }
  return OkStatus();
}

Status AddOperandInfo(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  for (auto& instruction : *instruction_set->mutable_instructions()) {
    for (InstructionFormat& vendor_syntax :
         *instruction.mutable_vendor_syntax()) {
      RETURN_IF_ERROR(AddOperandInfoToSyntax(instruction, &vendor_syntax));
    }
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddOperandInfo, 4000);

Status AddMissingOperandUsageToOperand(const InstructionProto& instruction,
                                       int operand_pos,
                                       InstructionOperand* operand) {
  CHECK(operand != nullptr);
  if (operand->usage() != InstructionOperand::USAGE_UNKNOWN) {
    // Nothing to do.
    return OkStatus();
  }
  if (operand->encoding() == InstructionOperand::IMMEDIATE_VALUE_ENCODING) {
    // An immediate can only be read from.
    operand->set_usage(InstructionOperand::USAGE_READ);
  } else if (operand->encoding() == InstructionOperand::VEX_V_ENCODING) {
    // A VEX encoded operand is always a source unless explicitly marked as a
    // destination. See table table 2-9 of the SDM volume 2 for details.
    if (operand_pos == 0) {
      return InvalidArgumentError(absl::StrCat(
          "Unexpected VEX.vvvv operand without usage specification "
          "at position 0:\n",
          instruction.DebugString()));
    }
    operand->set_usage(InstructionOperand::USAGE_READ);
  } else if (operand->encoding() == InstructionOperand::IMPLICIT_ENCODING &&
             operand->addressing_mode() ==
                 InstructionOperand::DIRECT_ADDRESSING) {
    // A few instructions have implicit source or destination registers,
    // typically AND AX, imm8.
    if (operand_pos == 0) {
      operand->set_usage(InstructionOperand::USAGE_WRITE);
    } else {
      operand->set_usage(InstructionOperand::USAGE_READ);
    }
  } else if (operand->encoding() == InstructionOperand::IMPLICIT_ENCODING &&
             operand->addressing_mode() == InstructionOperand::NO_ADDRESSING) {
    // The operand is an implicit immediate value.
    operand->set_usage(InstructionOperand::USAGE_READ);
  }
  // TODO(courbet): Add usage information for X87.
  return OkStatus();
}

Status AddMissingOperandUsage(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  for (auto& instruction : *instruction_set->mutable_instructions()) {
    for (InstructionFormat& vendor_syntax :
         *instruction.mutable_vendor_syntax()) {
      for (int operand_pos = 0; operand_pos < vendor_syntax.operands_size();
           ++operand_pos) {
        RETURN_IF_ERROR(AddMissingOperandUsageToOperand(
            instruction, operand_pos,
            vendor_syntax.mutable_operands(operand_pos)));
      }
    }
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddMissingOperandUsage, 8000);

Status AddMissingOperandUsageToVblendInstructions(
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  static const LazyRE2 kVblendRegexp = {"VP?BLENDV?P?[DSB]"};
  for (auto& instruction : *instruction_set->mutable_instructions()) {
    for (InstructionFormat& vendor_syntax :
         *instruction.mutable_vendor_syntax()) {
      if (!RE2::FullMatch(vendor_syntax.mnemonic(), *kVblendRegexp)) continue;
      InstructionOperand& last_operand =
          *vendor_syntax.mutable_operands()->rbegin();
      if (last_operand.usage() == InstructionOperand::USAGE_UNKNOWN) {
        last_operand.set_usage(InstructionOperand::USAGE_READ);
      }
    }
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddMissingOperandUsageToVblendInstructions,
                                   8000);

Status AddMissingVexVOperandUsage(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  for (auto& instruction : *instruction_set->mutable_instructions()) {
    EncodingSpecification* const encoding_specification =
        instruction.mutable_x86_encoding_specification();
    if (!encoding_specification->has_vex_prefix()) continue;
    VexPrefixEncodingSpecification* const vex_specification =
        encoding_specification->mutable_vex_prefix();
    if (vex_specification->vex_operand_usage() != UNDEFINED_VEX_OPERAND_USAGE) {
      continue;
    }

    const InstructionFormat& vendor_syntax =
        GetVendorSyntaxWithMostOperandsOrDie(instruction);
    const InstructionOperand* vex_operand = nullptr;
    for (const InstructionOperand& operand : vendor_syntax.operands()) {
      if (operand.encoding() == InstructionOperand::VEX_V_ENCODING) {
        vex_operand = &operand;
        break;
      }
    }
    if (vex_operand == nullptr) continue;
    switch (vex_operand->usage()) {
      case InstructionOperand::USAGE_UNKNOWN:
        // The usage is unknown - we mark the VEX operand as the destination
        // register. This is an arbitrarily chosen value, whose main purpose is
        // not being NO_VEX_OPERAND_USAGE.
        LOG(WARNING) << "Unknown VEX operand usage in "
                     << instruction.raw_encoding_specification();
        ABSL_FALLTHROUGH_INTENDED;
      case InstructionOperand::USAGE_READ:
        vex_specification->set_vex_operand_usage(
            vendor_syntax.operands(0).usage() ==
                    InstructionOperand::USAGE_READ_WRITE
                ? VEX_OPERAND_IS_SECOND_SOURCE_REGISTER
                : VEX_OPERAND_IS_FIRST_SOURCE_REGISTER);
        break;
      case InstructionOperand::USAGE_WRITE:
      case InstructionOperand::USAGE_READ_WRITE:
        vex_specification->set_vex_operand_usage(
            VEX_OPERAND_IS_DESTINATION_REGISTER);
        break;
      default:
        // The remaining values are sentinels and the number of operands. None
        // of them can appear in the proto.
        LOG(FATAL) << "Unexpected VEX operand usage: " << vex_operand->usage();
    }
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddMissingVexVOperandUsage, 3900);

}  // namespace x86
}  // namespace exegesis
