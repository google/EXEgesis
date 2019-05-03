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

#include "exegesis/util/system.h"

#include <sched.h>

#include "glog/logging.h"

namespace exegesis {

void SetCoreAffinity(int core_id) {
  cpu_set_t affinity;
  CPU_ZERO(&affinity);
  CPU_SET(core_id, &affinity);
  CHECK_EQ(0, sched_setaffinity(0, sizeof(affinity), &affinity));
}

void PinCoreAffinity() {
  cpu_set_t affinity;
  CPU_ZERO(&affinity);
  CHECK_EQ(0, sched_getaffinity(0, sizeof(affinity), &affinity));
  constexpr int kMaxCores = 4096;  // "Ought to be enough for anybody".
  for (int core_id = 0; core_id < kMaxCores; ++core_id) {
    if (CPU_ISSET(core_id, &affinity)) {
      LOG(INFO) << "Selected core " << core_id;
      SetCoreAffinity(core_id);
      return;
    }
  }
}

int GetLastAvailableCore() {
  cpu_set_t affinity;
  CPU_ZERO(&affinity);
  CHECK_EQ(0, sched_getaffinity(0, sizeof(affinity), &affinity));
  constexpr int kMaxCores = 4095;  // "Ought to be enough for anybody".
  for (int core_id = kMaxCores; core_id >= 0; --core_id) {
    if (CPU_ISSET(core_id, &affinity)) {
      LOG(INFO) << "Last available core: " << core_id;
      return core_id;
    }
  }
  return 0;
}

}  // namespace exegesis
