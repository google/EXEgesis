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

// Contains helper functions for testing the instruction set transforms.

#ifndef EXEGESIS_BASE_CLEANUP_INSTRUCTION_SET_TEST_UTILS_H_
#define EXEGESIS_BASE_CLEANUP_INSTRUCTION_SET_TEST_UTILS_H_

#include <string>

#include "exegesis/base/cleanup_instruction_set.h"

namespace exegesis {

// Tests 'transform' by running it on 'input_proto', and comparing the modified
// proto with 'expected_output_proto'.
void TestTransform(const InstructionSetTransform& transform,
                   const std::string& input_proto,
                   const std::string& expected_output_proto);

}  // namespace exegesis

#endif  // EXEGESIS_BASE_CLEANUP_INSTRUCTION_SET_TEST_UTILS_H_
