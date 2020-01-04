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

#include "exegesis/itineraries/perf_subsystem.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstdio>
#include <utility>

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/synchronization/mutex.h"
#include "exegesis/base/host_cpu.h"
#include "exegesis/base/microarchitecture.h"
#include "glog/logging.h"
#include "include/perfmon/pfmlib_perf_event.h"
#include "util/gtl/map_util.h"

namespace exegesis {

double PerfResult::Scale(const TimingInfo& timing) const {
  if (timing.time_running == 0 || timing.time_enabled == 0) return 0.0;
  // This scales the counter, taking into account the ratio of time the
  // counter was enabled.
  const double ratio = static_cast<double>(timing.time_running) /
                       static_cast<double>(timing.time_enabled);
  return ratio * static_cast<double>(timing.raw_count) /
         static_cast<double>(num_times_);
}

double PerfResult::GetScaledOrDie(const std::string& name) const {
  return Scale(gtl::FindOrDie(timings_, name));
}

std::string PerfResult::ToString() const {
  std::string result;
  for (const auto& key_value : timings_) {
    absl::StrAppendFormat(&result, "%s: %.2f, ", key_value.first,
                          Scale(key_value.second));
  }
  absl::StrAppendFormat(&result, "(num_times: %u)", num_times_);
  return result;
}

void PerfResult::Accumulate(const PerfResult& delta) {
  for (const auto& key_val : delta.timings_) {
    const std::string& name = key_val.first;
    timings_[name].Accumulate(key_val.second);
  }
}

std::vector<std::string> PerfResult::Keys() const {
  std::vector<std::string> result;
  for (const auto& key_val : timings_) {
    result.push_back(key_val.first);
  }
  return result;
}

PerfSubsystem::PerfSubsystem()
    : microarchitecture_(
          MicroArchitecture::FromIdOrDie(GetMicroArchitectureIdForCpuModelOrDie(
              HostCpuInfoOrDie().cpu_model_id()))) {
  counter_fds_.reserve(kMaxNumCounters);
  event_names_.reserve(kMaxNumCounters);
  timers_.resize(kMaxNumCounters);

  // Check the consistency between CPUs that p4lib and we detect.
  const std::string& cpu_id = microarchitecture_.proto().id();
  CHECK(absl::StrContains(Info(), cpu_id))
      << "'" << Info() << "' vs '" << cpu_id;
}

PerfSubsystem::~PerfSubsystem() { CleanUp(); }

void PerfSubsystem::CleanUp() {
  for (const int fd : counter_fds_) {
    close(fd);
  }
  counter_fds_.resize(0);
  event_names_.resize(0);
}

std::string PerfSubsystem::Info() const {
  std::string result;
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

int PerfSubsystem::AddEvent(const std::string& event_name) {
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
  for (const std::string& event : events) {
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

PerfResult PerfSubsystem::ReadCounters() {
  const int num_fds = counter_fds_.size();
  const int bytes_to_read = sizeof(TimingInfo);
  for (int i = 0; i < num_fds; ++i) {
    const int bytes_read =
        read(counter_fds_[i], timers_.data() + i, bytes_to_read);
    CHECK_EQ(bytes_to_read, bytes_read) << strerror(errno) << " i = " << i;
  }
  // We copy the result to the resulting vector here to avoid polluting the
  // counters with the call to resize().
  std::map<std::string, TimingInfo> timings;
  for (int i = 0; i < num_fds; ++i) {
    gtl::InsertOrDie(&timings, event_names_[i], timers_[i]);
  }
  return PerfResult(std::move(timings));
}

ABSL_CONST_INIT absl::Mutex
    PerfSubsystem::ScopedLibPfmInitialization::  // NOLINT
    refcount_mutex_;
int PerfSubsystem::ScopedLibPfmInitialization::refcount_ = 0;

PerfSubsystem::ScopedLibPfmInitialization::ScopedLibPfmInitialization() {
  absl::MutexLock l(&refcount_mutex_);
  if (refcount_++ == 0) {
    CHECK_EQ(PFM_SUCCESS, pfm_initialize());
  }
}

PerfSubsystem::ScopedLibPfmInitialization::~ScopedLibPfmInitialization() {
  absl::MutexLock l(&refcount_mutex_);
  if (--refcount_ == 0) {
    pfm_terminate();
  }
}

}  // namespace exegesis
