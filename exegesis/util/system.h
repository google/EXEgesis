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

// Utilities for interacting with the host and system.

#ifndef EXEGESIS_UTIL_SYSTEM_H_
#define EXEGESIS_UTIL_SYSTEM_H_

namespace exegesis {

// Assigns the current thread to core 'core_id'. Dies if the core cannot be
// bound to.
void SetCoreAffinity(int core_id);

// Same as above, but picks the first available core.
void PinCoreAffinity();

// Gets the last available CPU core id on the machine.
int GetLastAvailableCore();

}  // namespace exegesis

#endif  // EXEGESIS_UTIL_SYSTEM_H_
