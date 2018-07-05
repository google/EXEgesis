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

#include "exegesis/itineraries/decomposition.h"

#include <random>

#include "absl/strings/str_cat.h"
#include "exegesis/testing/test_util.h"
#include "exegesis/x86/microarchitectures.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/google/protobuf/text_format.h"

namespace exegesis {
namespace itineraries {
namespace {

using ::exegesis::testing::EqualsProto;
using ::exegesis::util::Status;
using ::exegesis::x86::HaswellMicroArchitecture;
using ::testing::AnyOf;
using ::testing::Eq;

// Synthetic benchmark based in the x86 instruction NEG m8, ADD RSI,16.
TEST(DecompositionTest, Negate) {
  DecompositionSolver solver(HaswellMicroArchitecture());
  // TODO(bdb): Only consider user-time measurements with the :u modifier.
  const std::string kObservationProto =
      R"proto(observations { event_name: 'cycles' measurement: 2.3336 }
              observations { event_name: 'num_times' measurement: 1 }
              observations {
                event_name: 'uops_executed_port:port_0'
                measurement: 0.4328
              }
              observations {
                event_name: 'uops_executed_port:port_1'
                measurement: 0.4720
              }
              observations {
                event_name: 'uops_executed_port:port_2'
                measurement: 0.8410
              }
              observations {
                event_name: 'uops_executed_port:port_3'
                measurement: 0.9518
              }
              observations {
                event_name: 'uops_executed_port:port_4'
                measurement: 1.0042
              }
              observations {
                event_name: 'uops_executed_port:port_5'
                measurement: 0.6130
              }
              observations {
                event_name: 'uops_executed_port:port_6'
                measurement: 0.6512
              }
              observations {
                event_name: 'uops_executed_port:port_7'
                measurement: 0.2257
              }
              observations { event_name: 'uops_issued:any' measurement: 5.1906 }
              observations {
                event_name: 'uops_retired:all'
                measurement: 5.1162
              })proto";
  ObservationVector observation;
  google::protobuf::TextFormat::ParseFromString(kObservationProto,
                                                &observation);
  ASSERT_OK(solver.Run(observation));
  const std::string kExpectedResult =
      "P23 P0156 P0156 P237 P4 "
      "\nP0156: {0: 0.22060, 1: 0.25980, 5: 0.25980, 6: 0.25980, }"
      "\nP0156: {0: 0.21220, 1: 0.21220, 5: 0.28780, 6: 0.28780, }"
      "\nP23: {2: 0.50000, 3: 0.50000, }"
      "\nP237: {2: 0.34100, 3: 0.43330, 7: 0.22570, }"
      "\nP4: {4: 1.00000, }\nmax_error = 0.10360"
      "\nerror {0: 0.00000, 1: 0.00000, 2: 0.00000, 3: 0.01850, "
      "4: 0.00420, 5: 0.06540, 6: 0.10360, 7: 0.00000, }"
      "\nis_order_unique = 1\n";
  EXPECT_LE(fabs(solver.objective_value() - 3588.3), 1e-6);
  EXPECT_EQ(kExpectedResult, solver.DebugString());

  ItineraryProto proto;
  *proto.mutable_micro_ops() = solver.GetMicroOps();
  constexpr char kExpected[] = R"proto(
    micro_ops {
      port_mask { port_numbers: [ 2, 3 ] }
      latency: 1
    }
    micro_ops {
      port_mask { port_numbers: [ 0, 1, 5, 6 ] }
      latency: 1
    }
    micro_ops {
      port_mask { port_numbers: [ 0, 1, 5, 6 ] }
      latency: 1
    }
    micro_ops {
      port_mask { port_numbers: [ 2, 3, 7 ] }
      latency: 1
    }
    micro_ops {
      port_mask { port_numbers: 4 }
      latency: 1
    }
  )proto";
  EXPECT_THAT(proto, EqualsProto(kExpected));
}

// Class for generating random measurements to be decomposed by the MIP
// decomposition algorithm. The algorithm generating the measurements works as
// follows:
// 1/ Generate a signature, i.e. a list of port masks, whose size 'num_uops'
//    is passed as an arguement.
// 2/ For each port mask in the signature, generate a load vector, i.e. a
//    vector of random doubles indexed by port number whose sum is equal to 1.0.
//    (A micro-operation using a port mask must be executed 100% of the time).
//    Only those ports that belong to the port mask have a value different from
//    zero. (A micro-operation using a port mask can only be executed on the
//    ports of that port mask).
//    Based on our observations, we decided that the minimum load on a given
//    port is at least 0.1.
// 3/ Generate a error vector. It's a vector of random values indexed by port
//    number whose sum is equal to a parameter.
// 4/ Add all the load vectors and the error vector together to form a
//    measurement, together with a random number of micro-operations depending
//    on 'num_uops'.
class MeasurementGenerator {
 public:
  MeasurementGenerator(const MicroArchitecture& microarchitecture,
                       const std::mt19937::result_type random_seed);

  // Returns a vector of random loads for port mask number 'mask_index' in
  // 'port_masks_'. The vector of loads is such that the sum of its elements is
  // equal to 1.0. To actually be on the safe side, the sum can be up to
  // 1.0 + 1e-6. The excess is going to be accounted in the error term of
  // the MIP decomposition model.
  std::vector<double> GenerateLoads(int mask_index);

  // Returns a std::vector<double> of random number whose sum is equal to
  // 'target_sum'.
  std::vector<double> GenerateFullVector(double target_sum);

  // Generates a signature of length 'num_uops'.
  std::vector<int> GenerateSignature(int num_uops);

  // Generates an histogram with 'num_uops' uops.
  std::vector<int> GenerateHistogram(int num_uops);

  // Generate a random number of micro-operations executed, which is a
  // double number between num_uops and num_uops + 0.9.
  double GenerateNumUopsExecuted(int num_uops) {
    std::uniform_real_distribution<double> uniform_double(num_uops,
                                                          num_uops + .9);
    return uniform_double(generator_);
  }

  // Adds 'loads' element-wise to 'measurements'.
  void Add(const std::vector<double>& loads, std::vector<double>* measurements);
  // Converts a signature, which is a list of port mask indices, to an histogram
  // which is a vector indexed by position in port_mask counting the use of each
  // port mask.
  std::vector<int> ConvertSignatureToHistogram(
      const std::vector<int>& signature);

  int num_ports() const { return num_ports_; }

 private:
  const std::vector<PortMask> port_masks_;

  std::mt19937 generator_;
  const int num_ports_;
};

MeasurementGenerator::MeasurementGenerator(
    const MicroArchitecture& microarchitecture,
    const std::mt19937::result_type random_seed)
    : port_masks_(microarchitecture.port_masks()),
      generator_(random_seed),
      num_ports_(ComputeNumExecutionPorts(port_masks_)) {}

std::vector<double> MeasurementGenerator::GenerateLoads(int mask_index) {
  const auto& port_mask = port_masks_[mask_index];
  std::vector<double> result(num_ports_, 0.0);
  const int mask_size = port_mask.num_possible_ports();
  std::vector<double> points(mask_size, 0.0);
  double sum = 0.0;
  std::uniform_real_distribution<double> uniform_double(0.0, 1.0);
  for (int i = 0; i < mask_size; ++i) {
    points[i] = uniform_double(generator_);
    sum += points[i];
  }
  const double kMinLoad = 0.1;
  const double kExcess = 1e-6;
  // Since we are essentially going to multiply by the inverse of sum, we add
  // kExcess to make sure that the sum of result will always be slightly greater
  // than one, thus making the decomposition problem feasible.
  const double scaling_factor = (1 + kExcess - kMinLoad * mask_size) / sum;
  int i = 0;
  for (int port : port_mask) {
    result[port] = kMinLoad + points[i] * scaling_factor;
    ++i;
  }
  return result;
}

std::vector<double> MeasurementGenerator::GenerateFullVector(
    double target_sum) {
  std::vector<double> values(num_ports_, 0.0);
  if (target_sum == 0.0) return values;
  double sum = 0;
  std::uniform_real_distribution<double> uniform_double(0.0, 1.0);
  for (double& value : values) {
    value = uniform_double(generator_);
    sum += value;
  }
  const double scale = target_sum / sum;
  for (double& value : values) {
    value *= scale;
  }
  return values;
}

std::vector<int> MeasurementGenerator::GenerateSignature(int num_uops) {
  std::vector<int> signature(num_uops);
  const int num_port_masks = port_masks_.size();
  std::uniform_int_distribution<int> uniform_int(0, num_port_masks - 1);
  for (int uop = 0; uop < num_uops; ++uop) {
    signature[uop] = uniform_int(generator_);
  }
  std::sort(signature.begin(), signature.end());
  return signature;
}

std::vector<int> MeasurementGenerator::GenerateHistogram(int num_uops) {
  const int num_port_masks = port_masks_.size();
  std::vector<int> histogram(num_port_masks, 0);
  std::uniform_int_distribution<int> uniform_int(0, num_port_masks - 1);
  for (int uop = 0; uop < num_uops; ++uop) {
    ++histogram[uniform_int(generator_)];
  }
  return histogram;
}

void MeasurementGenerator::Add(const std::vector<double>& loads,
                               std::vector<double>* measurements) {
  CHECK(measurements);
  CHECK_EQ(loads.size(), measurements->size());
  for (int i = 0; i < loads.size(); ++i) {
    (*measurements)[i] += loads[i];
  }
}

bool AreEqual(std::vector<int> v1, std::vector<int> v2) {
  if (v1.size() != v2.size()) return false;
  return std::equal(v1.begin(), v1.end(), v2.begin());
}

std::vector<int> MeasurementGenerator::ConvertSignatureToHistogram(
    const std::vector<int>& signature) {
  const int num_port_masks = port_masks_.size();
  std::vector<int> histogram(num_port_masks, 0);
  for (const int mask : signature) {
    DCHECK_LE(0, mask);
    DCHECK_LT(mask, num_port_masks);
    ++histogram[mask];
  }
  return histogram;
}

void DecomposeRandomInstructions(const std::mt19937::result_type random_seed) {
  const auto& microarchitecture = HaswellMicroArchitecture();
  const int kMaxNumOps = 5;
  std::vector<int> num_successes(kMaxNumOps + 1, 0);
  MeasurementGenerator m(microarchitecture, random_seed);
  const int kNumIters = 100;
  int num_errors = 0;
  for (int num_uops = 1; num_uops <= kMaxNumOps; ++num_uops) {
    for (int iter = 0; iter < kNumIters; ++iter) {
      std::vector<double> measurements = m.GenerateFullVector(0.0);
      const std::vector<int> signature = m.GenerateSignature(num_uops);
      for (int mask_index : signature) {
        const std::vector<double> loads = m.GenerateLoads(mask_index);
        m.Add(loads, &measurements);
      }
      const std::vector<double> errors = m.GenerateFullVector(0.05 * num_uops);
      m.Add(errors, &measurements);
      const double uops_executed = m.GenerateNumUopsExecuted(num_uops);
      DecompositionSolver solver(microarchitecture);
      const Status status = solver.Run(measurements, uops_executed);
      if (!status.ok()) {
        ++num_errors;
        LOG(INFO) << status;
      } else if (AreEqual(m.ConvertSignatureToHistogram(signature),
                          solver.histogram())) {
        ++num_successes[num_uops];
      }
    }
  }
  for (int num_uops = 1; num_uops < num_successes.size(); ++num_uops) {
    LOG(INFO) << num_uops
              << " uops: " << (100 * num_successes[num_uops]) / kNumIters
              << "% success rate.";
  }
  LOG(INFO) << num_errors << " errors.";
  // GPLK does return "ABNORMAL" in one of the random cases.
  EXPECT_LE(num_errors, 1);
}

constexpr const std::mt19937::result_type kDeterministicSeed = 123456;

TEST(DecompositionTest, Random) {
  DecomposeRandomInstructions(kDeterministicSeed);
}

TEST(DecompositionTest, OrderMicroOperations) {
  const auto& microarchitecture = HaswellMicroArchitecture();
  MeasurementGenerator m(microarchitecture, kDeterministicSeed);
  const int load_store_address_generation_mask_index =
      GetPositionInVector(microarchitecture.port_masks(),
                          *microarchitecture.store_address_generation());
  const int store_address_generation_mask_index =
      GetPositionInVector(microarchitecture.port_masks(),
                          *microarchitecture.load_store_address_generation());
  const int memory_buffer_write_mask_index = GetPositionInVector(
      microarchitecture.port_masks(), *microarchitecture.store_data());
  const int kNumIter = 1000;
  for (int iter = 0; iter < kNumIter; ++iter) {
    const int kMaxNumUops = 10;
    std::vector<int> histogram = m.GenerateHistogram(kMaxNumUops);
    bool solid = false;
    std::vector<int> signature = OrderMicroOperations(
        histogram, load_store_address_generation_mask_index,
        store_address_generation_mask_index, memory_buffer_write_mask_index,
        &solid);
    EXPECT_EQ(kMaxNumUops, signature.size());
    const int num_memory_writes = histogram[memory_buffer_write_mask_index];
    if (num_memory_writes > 0) {
      EXPECT_EQ(memory_buffer_write_mask_index, signature.back());
      // And if there are address generation uops, one occurs right before.
      if (histogram[store_address_generation_mask_index] +
              histogram[load_store_address_generation_mask_index] >=
          num_memory_writes) {
        const int penultimate_mask_index = signature[kMaxNumUops - 2];
        EXPECT_THAT(penultimate_mask_index,
                    AnyOf(Eq(store_address_generation_mask_index),
                          Eq(load_store_address_generation_mask_index)));
      }
    } else {
      if (histogram[store_address_generation_mask_index] > 0 ||
          histogram[load_store_address_generation_mask_index] > 0) {
        EXPECT_THAT(signature.front(),
                    AnyOf(Eq(store_address_generation_mask_index),
                          Eq(load_store_address_generation_mask_index)));
      }
    }
  }
}

}  // namespace
}  // namespace itineraries
}  // namespace exegesis
