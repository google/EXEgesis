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

#ifndef EXEGESIS_UTIL_INSTRUCTION_SYNTAX_H_
#define EXEGESIS_UTIL_INSTRUCTION_SYNTAX_H_

#include "strings/string.h"

#include "exegesis/proto/instructions.pb.h"

namespace cpu_instructions {

// Parses a code string in assembly format and returns a corresponding
// InstructionFormat.
// NOTE(bdb): This only handles x86 prefixes.
// TODO(bdb): Make this x86-independent.
InstructionFormat ParseAssemblyStringOrDie(const string& code);

// Returns an assembler-ready string corresponding to the InstructionFormat
// passed as argument.
string ConvertToCodeString(const InstructionFormat& proto);

}  // namespace cpu_instructions

#endif  // EXEGESIS_UTIL_INSTRUCTION_SYNTAX_H_
