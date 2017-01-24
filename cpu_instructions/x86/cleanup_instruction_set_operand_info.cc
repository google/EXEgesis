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

#include "cpu_instructions/x86/cleanup_instruction_set_operand_info.h"

#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include "strings/string.h"
#include <unordered_map>
#include <utility>
#include <vector>

#include "glog/logging.h"
#include "strings/str_cat.h"
#include "cpu_instructions/base/cleanup_instruction_set.h"
#include "cpu_instructions/proto/instructions.pb.h"
#include "cpu_instructions/proto/x86/encoding_specification.pb.h"
#include "cpu_instructions/x86/encoding_specification.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"
#include "util/task/status_macros.h"
#include "util/task/statusor.h"

namespace cpu_instructions {
namespace x86 {
namespace {

using ::cpu_instructions::util::InvalidArgumentError;
using ::cpu_instructions::util::Status;
using ::cpu_instructions::util::StatusOr;

using EncodingMap = std::unordered_map<string, InstructionOperand::Encoding>;
using AddressingModeMap =
    std::unordered_map<string, InstructionOperand::AddressingMode>;
using ValueSizeMap = std::unordered_map<string, uint32_t>;

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
    // NOTE(user): In the 2015-09 version of the manual, the control
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
    // NOTE(user): Apart from string instructions, there are a couple of
    // "scalar" instructions that do accept an operand from modrm.rm, but they
    // do not allow it to be a register operand. Since we replace the operands
    // of the string instructions with different strings, we can depend on the
    // remaining m(8|16|32|64) to be an actuall ModR/M encoded operand.
    {"m64", InstructionOperand::MODRM_RM_ENCODING},
    {"m64fp", InstructionOperand::MODRM_RM_ENCODING},
    {"m64int", InstructionOperand::MODRM_RM_ENCODING},
    // NOTE(user): After removing operands of string instructions, all other
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
    // NOTE(user): In the 2015-09 version of the manual, segment registers
    // are always encoded using modrm.reg.
    {"Sreg", InstructionOperand::MODRM_REG_ENCODING},
    {"ST(0)", InstructionOperand::IMPLICIT_ENCODING},
    // NOTE(user): In the 2015-09 version of the manual, ST(i) registers
    // are always encoded in the opcode of the instruction.
    {"ST(i)", InstructionOperand::OPCODE_ENCODING},
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
        {"m", InstructionOperand::INDIRECT_ADDRESSING},
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
        {"vm32x", InstructionOperand::INDIRECT_ADDRESSING},
        {"vm32y", InstructionOperand::INDIRECT_ADDRESSING},
        {"vm32z", InstructionOperand::INDIRECT_ADDRESSING},
        {"vm64x", InstructionOperand::INDIRECT_ADDRESSING},
        {"vm64y", InstructionOperand::INDIRECT_ADDRESSING},
        {"vm64z", InstructionOperand::INDIRECT_ADDRESSING},
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
    {"imm8", 8},
    {"imm16", 16},
    {"imm32", 32},
    {"imm64", 64},
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
};

}  // namespace

namespace {

// Updates the value of 'status': if 'status' was not OK, its old value is kept.
// Otherwise, it is replaced with the value of 'new_status'.
void UpdateStatus(Status* status, const Status& new_status) {
  CHECK(status != nullptr);
  if (status->ok() && !new_status.ok()) *status = new_status;
}

// Tries to remove one occurence of the operand encoding of 'operand' from
// 'available_encodings'. If it is removed, returns Status::OK. If
// 'available_encodings' does not contain such an encoding, returns an error
// status with an appropriate error message.
Status EraseOperandEncoding(
    const InstructionProto& instruction, const InstructionOperand& operand,
    InstructionOperandEncodingMultiset* available_encodings) {
  const InstructionOperand::Encoding encoding = operand.encoding();
  Status status = Status::OK;
  if (encoding != InstructionOperand::IMPLICIT_ENCODING) {
    const auto iterator = available_encodings->find(encoding);
    if (iterator == available_encodings->end()) {
      status = InvalidArgumentError(
          StrCat("Operand '", operand.name(), "' encoded using ",
                 InstructionOperand::Encoding_Name(encoding),
                 " is not specified in the encoding specification: ",
                 instruction.raw_encoding_specification()));
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
    const AddressingModeMap& addressing_mode_map,
    const EncodingMap& encoding_map, const ValueSizeMap& value_size_map,
    InstructionProto* instruction,
    InstructionOperandEncodingMultiset* available_encodings,
    std::vector<int>* operands_with_no_encoding) {
  CHECK(instruction != nullptr);
  CHECK(available_encodings != nullptr);
  CHECK(operands_with_no_encoding != nullptr);
  InstructionFormat* const vendor_syntax = instruction->mutable_vendor_syntax();
  Status status = Status::OK;
  for (int operand_index = 0; operand_index < vendor_syntax->operands_size();
       ++operand_index) {
    InstructionOperand* const operand =
        vendor_syntax->mutable_operands(operand_index);

    if (!operand->has_addressing_mode()) {
      InstructionOperand::AddressingMode addressing_mode;
      if (!FindCopy(addressing_mode_map, operand->name(), &addressing_mode)) {
        status = InvalidArgumentError(StrCat(
            "Could not determine addressing mode of operand: ", operand->name(),
            ", instruction ", vendor_syntax->mnemonic()));
        LOG(ERROR) << status;
        continue;
      }
      operand->set_addressing_mode(addressing_mode);
    }

    uint32_t value_size_bits = 0;
    if (FindCopy(value_size_map, operand->name(), &value_size_bits)) {
      operand->set_value_size_bits(value_size_bits);
    }

    if (operand->has_encoding()) {
      UpdateStatus(&status, EraseOperandEncoding(*instruction, *operand,
                                                 available_encodings));
    } else {
      // If there is only one way how an operand can be encoded, we assign this
      // encoding to the operand and remove it from the list of available
      // encodings. Then we'll need to assign encodings to the remaining
      // operands only from those "remaining" encodings.
      InstructionOperand::Encoding operand_encoding;
      if (FindCopy(encoding_map, operand->name(), &operand_encoding)) {
        operand->set_encoding(operand_encoding);
        UpdateStatus(&status, EraseOperandEncoding(*instruction, *operand,
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
  if (ContainsKey(*available_encodings, encoding)) {
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
// TODO(user): The manual actually contains a more detailed definition of
// each encoding scheme, but they are instruction specific and we do not have
// this information available in a machine-readable format. Ideally, our
// assignments should be based on this information. However, for now it looks
// like the heuristics are good enough to let us assign the operands as we need
// them, and so far, we do not need to know the exact matching of operand
// positions and encodings, only what encodings are used.
Status AssignEncodingByEncodingScheme(
    InstructionProto* instruction,
    const std::vector<int>& operands_with_no_encoding,
    InstructionOperandEncodingMultiset* available_encodings) {
  InstructionFormat* const vendor_syntax = instruction->mutable_vendor_syntax();
  const string& encoding_scheme = instruction->encoding_scheme();
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
                       << instruction->DebugString();
      }
    }
  }
  return Status::OK;
}

// Assigns the remaining available encodings to the remaining unassigned
// operands on a first come first served basis. Assumes that there are enough
// available encodings for all remaining operands.
Status AssignEncodingRandomlyFromAvailableEncodings(
    InstructionProto* instruction,
    InstructionOperandEncodingMultiset* available_encodings) {
  InstructionFormat* const vendor_syntax = instruction->mutable_vendor_syntax();
  for (InstructionOperand& operand : *vendor_syntax->mutable_operands()) {
    if (!operand.has_encoding()) {
      if (available_encodings->empty()) {
        return InvalidArgumentError(
            StrCat("No available encodings for instruction:\n",
                   instruction->DebugString()));
      }
      operand.set_encoding(*available_encodings->begin());
      available_encodings->erase(available_encodings->begin());
    }
  }
  return Status::OK;
}

}  // namespace

Status AddOperandInfo(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  const AddressingModeMap addressing_mode_map(std::begin(kAddressingModeMap),
                                              std::end(kAddressingModeMap));
  const EncodingMap encoding_map(std::begin(kEncodingMap),
                                 std::end(kEncodingMap));
  const ValueSizeMap value_size_map(std::begin(kOperandValueSizeBitsMap),
                                    std::end(kOperandValueSizeBitsMap));
  Status status = Status::OK;
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    InstructionFormat* const vendor_syntax =
        instruction.mutable_vendor_syntax();
    const StatusOr<EncodingSpecification> encoding_specification_or_status =
        ParseEncodingSpecification(instruction.raw_encoding_specification());
    RETURN_IF_ERROR(encoding_specification_or_status.status());
    const EncodingSpecification& encoding_specification =
        encoding_specification_or_status.ValueOrDie();

    InstructionOperandEncodingMultiset available_encodings =
        GetAvailableEncodings(encoding_specification);

    // First assign the addressing modes and the encodings that can be
    // determined from the operand itself.
    std::vector<int> operands_with_no_encoding;
    RETURN_IF_ERROR(AssignOperandPropertiesWhereUniquelyDetermined(
        addressing_mode_map, encoding_map, value_size_map, &instruction,
        &available_encodings, &operands_with_no_encoding));

    if (!operands_with_no_encoding.empty()) {
      // There are some operands that were not assigned the encoding just from
      // the name of the operand. We need to use a more sophisticated process.
      if (operands_with_no_encoding.size() == 1 &&
          available_encodings.size() == 1) {
        // There is just one operand where we need to assign the encoding, and
        // only one available encoding, so we simply match them. In theory, the
        // following branch should catch this case, but it doesn't work
        // correctly because some instructions of this type do not use the usual
        // encoding_scheme conventions, but we can correctly handle them using
        // this heuristic.
        InstructionOperand* const operand =
            vendor_syntax->mutable_operands(operands_with_no_encoding.front());
        operand->set_encoding(*available_encodings.begin());
      } else if (operands_with_no_encoding.size() <=
                 available_encodings.size()) {
        // We have enough available encodings to assign to the remaining
        // operands. First try to use the encoding scheme as a guide, and if
        // that fails, we just assign the remaining available encodings to the
        // remaining operands randomly.
        RETURN_IF_ERROR(AssignEncodingByEncodingScheme(
            &instruction, operands_with_no_encoding, &available_encodings));
        RETURN_IF_ERROR(AssignEncodingRandomlyFromAvailableEncodings(
            &instruction, &available_encodings));
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
        // We don't have enough available encodings to encode all the operands.
        status = InvalidArgumentError(StrCat(
            "There are more operands remaining than available encodings: ",
            instruction.DebugString()));
        LOG(ERROR) << status;
        continue;
      }
    }
  }
  return status;
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddOperandInfo, 4000);

Status AddMissingOperandUsage(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    for (int operand_pos = 0;
         operand_pos < instruction.vendor_syntax().operands_size();
         ++operand_pos) {
      InstructionOperand* const operand =
          instruction.mutable_vendor_syntax()->mutable_operands(operand_pos);
      if (operand->usage() != InstructionOperand::USAGE_UNKNOWN) {
        // Nothing to do.
        continue;
      }
      if (operand->encoding() == InstructionOperand::IMMEDIATE_VALUE_ENCODING) {
        // An immediate can only be read from.
        operand->set_usage(InstructionOperand::USAGE_READ);
      } else if (operand->encoding() == InstructionOperand::VEX_V_ENCODING) {
        // A VEX encoded operand is always a source unless explicitly marked as
        // a destination. See table table 2-9 of the SDM volume 2 for details.
        if (operand_pos == 0) {
          return InvalidArgumentError(
              StrCat("Unexpected VEX.vvvv operand without usage specification "
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
      }
      // TODO(user): Add usage information for X87.
    }
  }
  return Status::OK;
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddMissingOperandUsage, 8000);

}  // namespace x86
}  // namespace cpu_instructions
