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

#include "exegesis/base/registers.h"

#include <functional>

#include "absl/strings/str_cat.h"
#include "exegesis/proto/registers.pb.h"
#include "glog/logging.h"

namespace exegesis {
namespace {

RegisterSetProto MakeRegisters(
    const std::vector<RegisterTemplate>& templates,
    const std::function<std::string(int)>& base_name_callback,
    int num_registers, int base_binary_encoding) {
  CHECK_GT(templates.size(), 0);
  CHECK_GT(num_registers, 0);
  RegisterSetProto register_set;
  for (int i = 0; i < num_registers; ++i) {
    RegisterGroupProto* const group = register_set.add_register_groups();
    const std::string base_name = base_name_callback(i);
    for (const auto& tpl : templates) {
      RegisterProto* const reg = group->add_registers();
      reg->set_name(absl::StrCat(tpl.prefix, base_name, tpl.suffix));
      const int binary_encoding =
          base_binary_encoding + i + tpl.encoding_offset;
      reg->set_binary_encoding(binary_encoding);
      BitRange* const position = reg->mutable_position_in_group();
      position->set_lsb(tpl.lsb);
      position->set_msb(tpl.msb);
      reg->set_feature_name(tpl.feature_name);
      reg->set_register_class(tpl.register_class);
    }
    // Use the name of the first register in the group for naming the group. By
    // convention, this should be the most "representative" register of the
    // group.
    const std::string& register_name = group->registers(0).name();
    group->set_name(absl::StrCat(register_name, " group"));
    group->set_description(
        absl::StrCat("The group of registers aliased with ", register_name));
  }
  return register_set;
}

}  // namespace

RegisterSetProto MakeRegistersFromBaseNames(
    const std::vector<RegisterTemplate>& templates,
    const std::vector<std::string>& base_names, int base_binary_encoding) {
  const auto base_name_callback = [&base_names](int i) {
    return base_names[i];
  };
  return MakeRegisters(templates, base_name_callback, base_names.size(),
                       base_binary_encoding);
}

RegisterSetProto MakeRegistersFromBaseNameAndIndices(
    const std::vector<RegisterTemplate>& templates,
    const std::string& base_name, int begin_index, int end_index,
    int base_binary_encoding) {
  const auto base_name_callback = [&base_name, begin_index](int i) {
    return absl::StrCat(base_name, begin_index + i);
  };
  return MakeRegisters(templates, base_name_callback, end_index - begin_index,
                       base_binary_encoding);
}

}  // namespace exegesis
