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

#ifndef BASE_MACROS_H_
#define BASE_MACROS_H_

// This is a fallback implementation of FALLTHROUGH_INTENDED - if it is not
// defined by other projects, we define it as a harmless no-op statement. Note
// that defining it as a macro with no contents is not desirable, because that
// would leave an extra semicolon in the code.
#ifndef FALLTHROUGH_INTENDED
#define FALLTHROUGH_INTENDED \
  do {                       \
  } while (0)
#endif  // FALLTHROUGH_INTENDED

#endif  // BASE_MACROS_H_
