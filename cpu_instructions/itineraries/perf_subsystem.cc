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

#include "cpu_instructions/itineraries/perf_subsystem.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstdio>
#include <utility>

#include "base/stringprintf.h"
#include "cpu_instructions/base/host_cpu.h"
#include "glog/logging.h"
#include "include/perfmon/pfmlib_perf_event.h"
#include "strings/str_cat.h"
#include "util/gtl/map_util.h"

namespace cpu_instructions {

string PerfResultString(const PerfResult& perf_result,
                        const int additional_scale_factor) {
  string result;
  for (const auto& key_value : perf_result) {
    StringAppendF(&result, "%s: %.2f, ", key_value.first.c_str(),
                  key_value.second.scaled() /
                      (perf_result.GetScaleFactor() * additional_scale_factor));
  }
  return result;
}

void AccumulateCounters(const PerfResult& data, PerfResult* accumulator) {
  CHECK(accumulator != nullptr);
  for (const auto& key_val : data) {
    const string& event_name = key_val.first;
    const TimingInfo count = key_val.second;
    TimingInfo& current_cumul = (*accumulator)[event_name];
    current_cumul.Accumulate(count);
  }
}

namespace {

bool Contains(const string& big, const string& small) {
  return big.find(small) != string::npos;
}

}  // namespace

PerfSubsystem::PerfSubsystem()
    : microarchitecture_(
          CHECK_NOTNULL(CpuType::FromCpuId(HostCpuInfo::Get().cpu_id()))
              ->microarchitecture()) {
  counter_fds_.reserve(kMaxNumCounters);
  event_names_.reserve(kMaxNumCounters);
  timers_.resize(kMaxNumCounters);
  const int ret = pfm_initialize();
  CHECK_EQ(PFM_SUCCESS, ret);
  // Check the consistency between CPUs that p4lib and we detect.
  CHECK(Contains(Info(), microarchitecture_.proto().id()))
      << "'" << Info() << "' vs '" << microarchitecture_.proto().id() << "'";
}

PerfSubsystem::~PerfSubsystem() {
  CleanUp();
  pfm_terminate();
}

void PerfSubsystem::CleanUp() {
  for (const int fd : counter_fds_) {
    close(fd);
  }
  counter_fds_.resize(0);
  event_names_.resize(0);
}

string PerfSubsystem::Info() const {
  string result;
  int i;
  pfm_for_all_pmus(i) {
    pfm_pmu_info_t pmu_info;
    memset(&pmu_info, 0, sizeof(pmu_info));
    const int pfm_result =
        pfm_get_pmu_info(static_cast<pfm_pmu_t>(i), &pmu_info);
    if (pfm_result == PFM_SUCCESS && pmu_info.is_present) {
      if (!result.empty()) {
        result += ", ";
      }
      result += pmu_info.name;
    }
  }
  return result;
}

void PerfSubsystem::ListEvents() {
  int i;
  pfm_for_all_pmus(i) {
    pfm_pmu_t pmu = static_cast<pfm_pmu_t>(i);
    pfm_event_info_t event_info;
    pfm_pmu_info_t pmu_info;
    memset(&event_info, 0, sizeof(event_info));
    memset(&pmu_info, 0, sizeof(pmu_info));
    event_info.size = sizeof(event_info);
    pmu_info.size = sizeof(pmu_info);
    const int pfm_result = pfm_get_pmu_info(pmu, &pmu_info);
    if (PFM_SUCCESS != pfm_result) continue;
    for (int event = pmu_info.first_event; event != -1;
         event = pfm_get_event_next(event)) {
      const int pfm_result =
          pfm_get_event_info(event, PFM_OS_PERF_EVENT, &event_info);
      CHECK_EQ(PFM_SUCCESS, pfm_result) << pfm_strerror(pfm_result);
      LOG(INFO) << (pmu_info.is_present ? "Active" : "Supported")
                << " Event: " << pmu_info.name << "::" << event_info.name;
    }
  }
}

int PerfSubsystem::AddEvent(const string& event_name) {
  struct perf_event_attr attr = {0};
  memset(&attr, 0, sizeof(attr));
  attr.size = sizeof(attr);
  const int pfm_result = pfm_get_perf_event_encoding(
      event_name.c_str(), PFM_PLM3, &attr, nullptr, nullptr);
  CHECK_EQ(PFM_SUCCESS, pfm_result)
      << pfm_strerror(pfm_result) << " " << event_name << " " << Info();
  attr.disabled = 1;
  // attr.pinned;
  attr.exclude_kernel = 1;

  // Always collect stats for how often the collection was occurring.
  attr.read_format =
      PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
  const int fd = perf_event_open(&attr, getpid(), -1, -1, 0);
  CHECK_LE(0, fd) << pfm_strerror(fd) << ": " << event_name;
  counter_fds_.push_back(fd);
  event_names_.push_back(event_name);
  return counter_fds_.size() - 1;
}

void PerfSubsystem::StartCollecting() {
  for (const int fd : counter_fds_) {
    const int ret = ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    CHECK_EQ(0, ret) << strerror(errno) << ", fd = " << fd;
  }
}

void PerfSubsystem::StartCollectingEvents(EventCategory category) {
  CleanUp();
  const auto& events = (microarchitecture_.proto().perf_events().*(category))();
  CHECK_LE(events.size(), 4)
      << "There should be less that 4 events to avoid multiplexing";
  for (const string& event : events) {
    AddEvent(event);
  }
  StartCollecting();
}

void PerfSubsystem::StopCollecting() {
  for (const int fd : counter_fds_) {
    const int ret = ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    CHECK_EQ(0, ret) << strerror(errno) << " fd = " << fd;
  }
}

void PerfSubsystem::ReadCounters(PerfResult* result) {
  const int num_fds = counter_fds_.size();
  const int bytes_to_read = sizeof(TimingInfo);
  for (int i = 0; i < num_fds; ++i) {
    const int bytes_read =
        read(counter_fds_[i], timers_.data() + i, bytes_to_read);
    CHECK_EQ(bytes_to_read, bytes_read) << strerror(errno) << " i = " << i;
  }
  // We copy the result to the resulting vector here to avoid polluting the
  // counters with the call to resize().
  for (int i = 0; i < num_fds; ++i) {
    (*result)[event_names_[i]] = timers_[i];
  }
}

}  // namespace cpu_instructions
