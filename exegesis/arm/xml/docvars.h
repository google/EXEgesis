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

#ifndef EXEGESIS_ARM_XML_DOCVARS_H_
#define EXEGESIS_ARM_XML_DOCVARS_H_

#include "exegesis/arm/xml/docvars.pb.h"
#include "tinyxml2.h"
#include "util/task/status.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace arm {
namespace xml {

using ::exegesis::util::Status;
using ::exegesis::util::StatusOr;

// Parses the given <docvars> XML node into a proper DocVar proto.
StatusOr<dv::DocVars> ParseDocVars(::tinyxml2::XMLNode* node);

// Returns whether the given 'subset' is really a subset of 'docvars', i.e. all
// fields set in 'subset' are exactly equal to their equivalent in 'docvars'.
Status DocVarsContains(const dv::DocVars& docvars, const dv::DocVars& subset);

}  // namespace xml
}  // namespace arm
}  // namespace exegesis

#endif  // EXEGESIS_ARM_XML_DOCVARS_H_
