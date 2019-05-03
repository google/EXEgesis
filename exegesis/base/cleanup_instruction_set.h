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

// Contains the library of InstructionSetProto transformations used for cleaning
// up the instruction database.

#ifndef EXEGESIS_BASE_CLEANUP_INSTRUCTION_SET_H_
#define EXEGESIS_BASE_CLEANUP_INSTRUCTION_SET_H_

#include <functional>
#include <limits>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "exegesis/proto/instructions.pb.h"
#include "util/task/status.h"
#include "util/task/statusor.h"

namespace exegesis {

using ::exegesis::util::Status;
using ::exegesis::util::StatusOr;

// The type of all instruction database transforms.
// InstructionSetTransformRawFunction is the type of the functions that can be
// registered as a stransform using REGISTER_INSTRUCTION_SET_TRANSFORM.
// InstructionSetTransform is a std::function wrapper around this type.
using InstructionSetTransformRawFunction = Status(InstructionSetProto*);
using InstructionSetTransform = std::function<Status(InstructionSetProto*)>;

// The list of instruction database transforms indexed by their names.
using InstructionSetTransformsByName =
    absl::flat_hash_map<std::string, InstructionSetTransform>;

// Returns the list of all available transforms, indexed by their names.
const InstructionSetTransformsByName& GetTransformsByName();

// Returns the default sequence of transforms that need to be applied to the
// data from the Intel manual to clean them up and transform them into a format
// suitable for machine processing. The values in the vector are pointers to
// functions, and the caller must not delete them.
//
// Note that some of the transforms expect that another transform was already
// executed, and they might not function correctly if this assumption is
// violated. The vector contains the transforms in the correct order.
std::vector<InstructionSetTransform> GetDefaultTransformPipeline();

// Runs the given transform on the given instruction set proto, and computes a
// diff of the changes made by the transform. The changes are returned as a
// human-readable string; the returned string is empty if and only if the
// transform did not make any changes to the proto.
StatusOr<std::string> RunTransformWithDiff(
    const InstructionSetTransform& transform,
    InstructionSetProto* instruction_set);

// Runs all transforms from 'pipeline' on the given instruction set proto.
// Returns Status::OK if all transform succeeds; otherwise, stops on the first
// transform that fails. The state of the instruction set proto after a failure
// is undefined.
Status RunTransformPipeline(
    const std::vector<InstructionSetTransform>& pipeline,
    InstructionSetProto* instruction_set);

// Sorts the instructions by their vendor syntax. The sorting criteria are:
// 1. The mnemonic (lexicographical order),
// 2. The operands names (two-level lexicographical order).
// 3. The operand tags (three-level lexicographical order).
// 4. The binary encoding of the instruction.
// This transform should be the last transform in the set, so that it cleans up
// after the changes done by the other instructions.
Status SortByVendorSyntax(InstructionSetProto* instruction_set);

// A registration mechanism for the instruction set pipeline. Registering the
// transform will add it to the list returned by GetTransformsByName, and
// optionally also to the default transform pipeline.
// The value of 'rank_in_default_pipeline' is either kNotInDefaultPipeline, or
// an integer value. GetDefaultTransformPipeline returns the list of registered
// transforms whose rank was not kNotInDefaultPipeline sorted by their rank; the
// order of transforms that have the same rank is undefined, and it may change
// with each build of the code.
#define REGISTER_INSTRUCTION_SET_TRANSFORM(transform,                      \
                                           rank_in_default_pipeline)       \
  ::exegesis::internal::RegisterInstructionSetTransform                    \
      register_transform_##transform(#transform, rank_in_default_pipeline, \
                                     transform)

// A special value passed to REGISTER_INSTRUCTION_SET_TRANSFORM for transforms
// that are not included in the default pipeline.
constexpr int kNotInDefaultPipeline = std::numeric_limits<int>::max();

namespace internal {

// A helper class used for the implementation of the registerer; the constructor
// registers the transform to all the relevant lists.
class RegisterInstructionSetTransform {
 public:
  RegisterInstructionSetTransform(const std::string& transform_name,
                                  int rank_in_default_pipeline,
                                  InstructionSetTransformRawFunction transform);
};

}  // namespace internal
}  // namespace exegesis

#endif  // EXEGESIS_BASE_CLEANUP_INSTRUCTION_SET_H_
