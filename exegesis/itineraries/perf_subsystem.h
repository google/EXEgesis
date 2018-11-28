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

#ifndef EXEGESIS_ITINERARIES_PERF_SUBSYSTEM_H_
#define EXEGESIS_ITINERARIES_PERF_SUBSYSTEM_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "absl/synchronization/mutex.h"
#include "exegesis/base/microarchitecture.h"
#include "exegesis/proto/microarchitecture.pb.h"
#include "glog/logging.h"
#include "src/google/protobuf/repeated_field.h"
#include "util/gtl/map_util.h"

namespace exegesis {
// A minimalistic interface to the Linux kernel perf subsystem, based on
// libpfm4.

// The perf subsystem counters consist of three 64-bit integers.
struct TimingInfo {
  TimingInfo() : raw_count(0), time_enabled(0), time_running(0) {}
  TimingInfo(uint64_t c, uint64_t e, uint64_t r)
      : raw_count(c), time_enabled(e), time_running(r) {}

  uint64_t raw_count;     // How many times the counter was incremented.
  uint64_t time_enabled;  // How much time the counter was enabled.
  uint64_t time_running;  // How much time the profiled code has been running.

  TimingInfo& Accumulate(const TimingInfo& other) {
    raw_count += other.raw_count;
    time_enabled += other.time_enabled;
    time_running += other.time_running;
    return *this;
  }
};

// Used to store the result of a profiled run.
// The names of each event are stored in the map so that the object can
// actually be used independently from a PerfSubsystem object.
// We use an ordered map, because they are out of the critical performance path,
// they are small (less than 10 pairs), and they enable to display sorted
// results easily.
class PerfResult {
 public:
  PerfResult() = default;
  PerfResult(const PerfResult&) = default;
  PerfResult& operator=(const PerfResult&) = default;

  // For tests.
  explicit PerfResult(std::map<std::string, TimingInfo> timings)
      : timings_(std::move(timings)) {}

  bool HasTiming(const std::string& name) const {
    return gtl::ContainsKey(timings_, name);
  }

  // Returns the scaled value for the given counter name.
  double GetScaledOrDie(const std::string& name) const;

  void SetScaleFactor(uint64_t num_times) { num_times_ = num_times; }

  // Returns a human-readable cycle count for `result`.
  std::string ToString() const;

  // Accumulates the counters in delta.
  void Accumulate(const PerfResult& delta);

  // Returns all keys.
  std::vector<std::string> Keys() const;

 private:
  double Scale(const TimingInfo& info) const;

  std::map<std::string, TimingInfo> timings_;
  uint64_t num_times_ = 1;
};  // namespace exegesis

// Not thread safe.
class PerfSubsystem {
 public:
  // Represents an event category from PerfEventsProto.
  using EventCategory =
      const ::google::protobuf::RepeatedPtrField<std::string>& (
          PerfEventsProto::*)() const;

  // Creates a perf subsystem for the host microarchitecture.
  PerfSubsystem();

  ~PerfSubsystem();

  // Cleans up the used counters. This is useful for preparing the object
  // to collect other events.
  void CleanUp();

  // Returns a string indicating which performance monitoring unit is
  // supported by the running system.
  std::string Info() const;

  // Lists all the events supported by the running platform.
  void ListEvents();

  // Adds an event to be measured by the current object. Returns the index of
  // the newly added event.
  // Note: To enable instruction counting on machines running Debian, execute
  // the following commands to modify the permissions.
  //   sudo echo "1" > /proc/sys/kernel/perf_event_paranoid
  //   sudo echo "0" > /proc/sys/kernel/kptr_restrict
  int AddEvent(const std::string& event_name);

  // Starts collecting data, i.e. hardware counters will be updated from here.
  void StartCollecting();

  // A short-cut that adds the events in 'category' and starts collecting.
  void StartCollectingEvents(EventCategory category);

  // A short-cut that stops collecting and reads the counters.
  PerfResult StopAndReadCounters() {
    StopCollecting();
    return ReadCounters();
  }

  // Reads the hardware counters and returns a PerfResult that contains all the
  // useful information, independently of the PerfSubsystem.
  PerfResult ReadCounters();

 private:
  // A class that ensures that we always manipulate libpfm initialization in a
  // thread-safe way, and that we do not initialize/terminate concurrently.
  class ScopedLibPfmInitialization {
   public:
    ScopedLibPfmInitialization();
    ~ScopedLibPfmInitialization();

   private:
    static absl::Mutex refcount_mutex_;
    static int refcount_;
  };

  // This interface can handle at most kMaxNumCounters counters at the same
  // time.
  static constexpr const int kMaxNumCounters = 128;

  // Stops collecting data, i.e. hardware counters will be stop being updated
  // from here.
  void StopCollecting();

  const MicroArchitecture& microarchitecture_;
  // File descriptor for each counter.
  std::vector<int> counter_fds_;
  // Name as given by libpfm4, of the event for each counter.
  std::vector<std::string> event_names_;
  // Used to store the result of the profiling.
  std::vector<TimingInfo> timers_;
  ScopedLibPfmInitialization scoped_libpfm_;
};

}  // namespace exegesis

#define EXEGESIS_MEASURE_LOOP(result, num_iter, s, events)          \
  perf.StartCollectingEvents(&::exegesis::PerfEventsProto::events); \
  for (int i = 0; i < num_iter; ++i) {                              \
    s;                                                              \
  }                                                                 \
  (result)->Accumulate(perf.StopAndReadCounters());

// A basic macro that measures a code snippet s.
#define EXEGESIS_RUN_UNDER_PERF(result, num_iter, s)                \
  {                                                                 \
    ::exegesis::PerfSubsystem perf;                                 \
    EXEGESIS_MEASURE_LOOP(result, num_iter, s, computation_events); \
    EXEGESIS_MEASURE_LOOP(result, num_iter, s, memory_events);      \
    EXEGESIS_MEASURE_LOOP(result, num_iter, s, cycle_events);       \
    EXEGESIS_MEASURE_LOOP(result, num_iter, s, uops_events);        \
    (result)->SetScaleFactor(num_iter);                             \
  }

// A basic macro that counts 'event' on a code snippet s. Resets result.
#define EXEGESIS_COUNT_EVENT_UNDER_PERF(result, num_iter, s, event) \
  {                                                                 \
    ::exegesis::PerfSubsystem perf;                                 \
    perf.AddEvent(event);                                           \
    perf.StartCollecting();                                         \
    for (int i = 0; i < num_iter; ++i) {                            \
      s;                                                            \
    }                                                               \
    *(result) = perf.StopAndReadCounters();                         \
    (result)->SetScaleFactor(num_iter);                             \
  }

#endif  // EXEGESIS_ITINERARIES_PERF_SUBSYSTEM_H_
