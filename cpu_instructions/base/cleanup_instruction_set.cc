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

#include "cpu_instructions/base/cleanup_instruction_set.h"

#include <algorithm>
#include <map>
#include <vector>
#include "strings/string.h"

#include "gflags/gflags.h"
#include "glog/logging.h"
#include "src/google/protobuf/descriptor.h"
#include "src/google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "src/google/protobuf/repeated_field.h"
#include "src/google/protobuf/util/message_differencer.h"
#include "util/gtl/map_util.h"
#include "util/task/status.h"
#include "util/task/status_macros.h"
#include "util/task/statusor.h"

DEFINE_bool(cpu_instructions_print_transform_names_to_log, true,
            "Print the names of the transforms executed by the transform "
            "pipeline to the log.");
DEFINE_bool(cpu_instructions_print_transform_diffs_to_log, false,
            "Print the names and the diffs of the instruction set before and "
            "after running each transform to the log.");

namespace cpu_instructions {

using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Message;
using ::google::protobuf::util::MessageDifferencer;
using ::cpu_instructions::util::Status;
using ::cpu_instructions::util::StatusOr;

using InstructionSetTransformOrder =
    std::multimap<int, InstructionSetTransform>;

namespace internal {
namespace {

InstructionSetTransformsByName* GetMutableTransformsByName() {
  static InstructionSetTransformsByName* const transforms_by_name =
      new InstructionSetTransformsByName();
  return transforms_by_name;
}

InstructionSetTransformOrder* GetMutableDefaultTransformOrder() {
  static InstructionSetTransformOrder* const transforms_order =
      new InstructionSetTransformOrder();
  return transforms_order;
}

Status RunSingleTransform(
    const string& transform_name,
    InstructionSetTransformRawFunction* transform_function,
    InstructionSetProto* instruction_set) {
  CHECK(transform_function != nullptr);
  CHECK(instruction_set != nullptr);
  if (FLAGS_cpu_instructions_print_transform_names_to_log ||
      FLAGS_cpu_instructions_print_transform_diffs_to_log) {
    LOG(INFO) << "Running: " << transform_name;
  }
  Status transform_status = Status::OK;
  if (FLAGS_cpu_instructions_print_transform_diffs_to_log) {
    const StatusOr<string> diff_or_status =
        RunTransformWithDiff(transform_function, instruction_set);
    if (diff_or_status.ok()) {
      const string& diff = diff_or_status.ValueOrDie();
      // TODO(user): Consider trimming the output, so that we don't flood
      // the output when there are too many diffs.
      if (!diff.empty()) {
        LOG(INFO) << "Difference:\n" << diff;
      }
    }
    transform_status = diff_or_status.status();
  } else {
    transform_status = transform_function(instruction_set);
  }
  if (FLAGS_cpu_instructions_print_transform_names_to_log ||
      FLAGS_cpu_instructions_print_transform_diffs_to_log) {
    const char* const status = transform_status.ok() ? "Success: " : "Failed: ";
    LOG(INFO) << status << transform_name;
  }
  return transform_status;
}

}  // namespace

RegisterInstructionSetTransform::RegisterInstructionSetTransform(
    const string& transform_name, int rank_in_default_pipeline,
    InstructionSetTransformRawFunction transform) {
  InstructionSetTransformsByName& transforms_by_name =
      *GetMutableTransformsByName();
  CHECK(!ContainsKey(transforms_by_name, transform_name))
      << "Transform name '" << transform_name << "' is already used!";
  InstructionSetTransform transform_wrapper =
      [transform_name, transform](InstructionSetProto* instruction_set) {
        return RunSingleTransform(transform_name, transform, instruction_set);
      };
  transforms_by_name[transform_name] = transform_wrapper;
  if (rank_in_default_pipeline != kNotInDefaultPipeline) {
    GetMutableDefaultTransformOrder()->emplace(rank_in_default_pipeline,
                                               transform_wrapper);
  }
}

}  // namespace internal

const InstructionSetTransformsByName& GetTransformsByName() {
  return *internal::GetMutableTransformsByName();
}

std::vector<InstructionSetTransform> GetDefaultTransformPipeline() {
  const InstructionSetTransformOrder& default_pipeline_transforms_order =
      *internal::GetMutableDefaultTransformOrder();
  std::vector<InstructionSetTransform> transforms;
  transforms.reserve(default_pipeline_transforms_order.size());
  for (const auto& element : default_pipeline_transforms_order) {
    transforms.push_back(element.second);
  }
  return transforms;
}

Status RunTransformPipeline(
    const std::vector<InstructionSetTransform>& pipeline,
    InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  for (const InstructionSetTransform& transform : pipeline) {
    CHECK(transform != nullptr);
    RETURN_IF_ERROR(transform(instruction_set));
  }
  return Status::OK;
}

// A message difference reporter that reports the differences to a string, and
// ignores all matched & moved items.
class ConciseDifferenceReporter : public MessageDifferencer::Reporter {
 public:
  explicit ConciseDifferenceReporter(string* output_string)
      : stream_(output_string), base_reporter_(&stream_) {}

  void ReportAdded(const Message& message1, const Message& message2,
                   const std::vector<MessageDifferencer::SpecificField>&
                       field_path) override {
    base_reporter_.ReportAdded(message1, message2, field_path);
  }
  void ReportDeleted(const Message& message1, const Message& message2,
                     const std::vector<MessageDifferencer::SpecificField>&
                         field_path) override {
    base_reporter_.ReportDeleted(message1, message2, field_path);
  }
  void ReportModified(const Message& message1, const Message& message2,
                      const std::vector<MessageDifferencer::SpecificField>&
                          field_path) override {
    base_reporter_.ReportModified(message1, message2, field_path);
  }

 private:
  ::google::protobuf::io::StringOutputStream stream_;
  MessageDifferencer::StreamReporter base_reporter_;
};

StatusOr<string> RunTransformWithDiff(const InstructionSetTransform& transform,
                                      InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  InstructionSetProto original_instruction_set = *instruction_set;

  RETURN_IF_ERROR(transform(instruction_set));

  string differences;
  {
    // NOTE(user): The block here is necessary because the differencer and
    // the reporter must be destroyed before the return value is constructed.
    // Otherwise, the compiler would use move semantics and move the contents of
    // 'differences' of it before the reporter flushes the remaining changes in
    // the destructor. The only way to force this flush is to force the
    // destruction of the objects before the return value is constructed.
    MessageDifferencer differencer;
    ConciseDifferenceReporter reporter(&differences);
    differencer.ReportDifferencesTo(&reporter);

    const FieldDescriptor* const instructions_field =
        instruction_set->GetDescriptor()->FindFieldByName("instructions");
    CHECK(instructions_field != nullptr);
    differencer.TreatAsSet(instructions_field);

    // NOTE(user): We are only interested in the string diff; we can safely
    // ignore the return value saying whether the two are equivalent or not.
    differencer.Compare(original_instruction_set, *instruction_set);
  }

  return differences;
}

namespace {

bool LessOrEqual(const InstructionProto& instruction_a,
                 const InstructionProto& instruction_b) {
  const InstructionFormat& vendor_syntax_a = instruction_a.vendor_syntax();
  const InstructionFormat& vendor_syntax_b = instruction_b.vendor_syntax();

  int comparison = 0;

  // 1. Compare mnemonics.
  comparison = vendor_syntax_a.mnemonic().compare(vendor_syntax_b.mnemonic());

  // 2. Compare the number of operands.
  if (comparison == 0) {
    comparison =
        vendor_syntax_b.operands_size() - vendor_syntax_a.operands_size();
  }

  // 3. Compare the operands.
  if (comparison == 0) {
    const int num_operands = vendor_syntax_a.operands_size();
    for (int operand_index = 0; comparison == 0 && operand_index < num_operands;
         ++operand_index) {
      const string& operand_a = vendor_syntax_a.operands(operand_index).name();
      const string& operand_b = vendor_syntax_b.operands(operand_index).name();
      comparison = operand_a.compare(operand_b);
    }
  }

  // 4. Compare binary encodings.
  if (comparison == 0) {
    const string& specification_a = instruction_a.raw_encoding_specification();
    const string& specification_b = instruction_b.raw_encoding_specification();
    comparison = specification_a.compare(specification_b);
  }

  return comparison < 0;
}

}  // namespace

Status SortByVendorSyntax(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  google::protobuf::RepeatedPtrField<InstructionProto>* const instructions =
      instruction_set->mutable_instructions();
  std::sort(instructions->begin(), instructions->end(), LessOrEqual);
  return Status::OK;
}
REGISTER_INSTRUCTION_SET_TRANSFORM(SortByVendorSyntax, 7000);

}  // namespace cpu_instructions
