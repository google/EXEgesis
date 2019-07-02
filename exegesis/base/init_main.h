// Copyright 2019 Google Inc.
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

#ifndef THIRD_PARTY_EXEGESIS_EXEGESIS_BASE_INIT_MAIN_H_
#define THIRD_PARTY_EXEGESIS_EXEGESIS_BASE_INIT_MAIN_H_

namespace exegesis {

// This function should be called at the beginning of each main() function to
// parse command-line flags and perform any other necessary initialization.
void InitMain(int argc, char* argv[]);

}  // namespace exegesis

#endif  // THIRD_PARTY_EXEGESIS_EXEGESIS_BASE_INIT_MAIN_H_
