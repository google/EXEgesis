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

#include "exegesis/x86/instruction_set_utils.h"

namespace exegesis {
namespace x86 {

const absl::flat_hash_set<std::string>& GetX87FpuInstructionMnemonics() {
  // NOTE(ondrasej): The list is taken from the Intel SDM, May 2018 version,
  // sections 5.2 X87 FPU Instructions and 5.3 X87 FPU and SIMD State Management
  // Instructions.
  static const absl::flat_hash_set<std::string>* const kMnemonics =
      new absl::flat_hash_set<std::string>(
          {// 5.2.1 x87 FPU Data Transfer Instructions.
           "FBLD", "FBSTP", "FCMOVB", "FCMOVBE", "FCMOVE", "FCMOVNB",
           "FCMOVNBE", "FCMOVNE", "FCMOVNU", "FCMOVU", "FILD", "FIST", "FISTP",
           "FLD", "FST", "FSTP", "FXCH",

           // Note that FISTTP is provided by SSE3, but it is still a floating
           // point stack instruction.
           "FISTTP",

           // 5.2.2 x87 FPU Basic Arithmetic Instructions.
           "FABS", "FADD", "FADDP", "FCHS", "FDIV", "FDIVP", "FDIVR", "FDIVRP",
           "FIADD", "FIDIV", "FIDIVR", "FIMUL", "FISUB", "FISUBR", "FMUL",
           "FMULP", "FPREM", "FPREM1", "FRNDINT", "FSCALE", "FSQRT", "FSUB",
           "FSUBP", "FSUBR", "FSUBRP", "FXTRACT",

           // 5.2.3 x87 FPU Comparison Instructions.
           "FCOM", "FCOMI", "FCOMIP", "FCOMP", "FCOMPP", "FICOM", "FICOMP",
           "FTST", "FUCOM", "FUCOMI", "FUCOMIP", "FUCOMP", "FUCOMPP", "FXAM",

           // 5.2.4 x87 FPU Transcendental Instructions.
           "F2XM1", "FCOS", "FPATAN", "FPTAN", "FSIN", "FSINCOS", "FYL2X",
           "FYL2XP1",

           // 5.2.5 x87 FPU Load Constant Instructions.
           "FLD1", "FLDL2E", "FLDL2T", "FLDLG2", "FLDLN2", "FLDPI", "FLDZ",

           // 5.2.6 x87 FPU Control Instructions.
           "FCLEX", "FDECSTP", "FFREE", "FINCSTP", "FINIT", "FLDCW", "FLDENV",
           "FNCLEX", "FNINIT", "FNOP", "FNSAVE", "FNSTCW", "FNSTENV", "FNSTSW",
           "FRSTOR", "FSAVE", "FSTCW", "FSTENV", "FSTSW", "WAIT",

           // 5.3 x87 FPU and SIMD State Management Functions.
           "FXRSTOR", "FXSAVE"});
  return *kMnemonics;
}

}  // namespace x86
}  // namespace exegesis
