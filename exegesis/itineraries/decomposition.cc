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

// TODO(courbet): Be consistent with micro_op/uop naming.
#include "exegesis/itineraries/decomposition.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numeric>
#include <string>
#include <utility>

#include "absl/algorithm/container.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "exegesis/base/microarchitecture.h"
#include "exegesis/util/instruction_syntax.h"
#include "glog/logging.h"
#include "ortools/linear_solver/linear_solver.h"
#include "src/google/protobuf/repeated_field.h"
#include "src/google/protobuf/text_format.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"
#include "util/task/statusor.h"

namespace exegesis {
namespace itineraries {

using ::absl::c_linear_search;
using ::exegesis::util::Status;
using ::operations_research::MPConstraint;
using ::operations_research::MPModelProto;
using ::operations_research::MPObjective;
using ::operations_research::MPSolver;
using ::operations_research::MPVariable;

int ComputeNumExecutionPorts(const std::vector<PortMask>& port_masks) {
  int max_execution_port_num = -1;
  for (auto mask : port_masks) {
    for (int port_num : mask) {
      max_execution_port_num = std::max(port_num, max_execution_port_num);
    }
  }
  return max_execution_port_num + 1;
}

DecompositionSolver::DecompositionSolver(
    const MicroArchitecture& microarchitecture)
    : microarchitecture_(&microarchitecture),
      num_execution_ports_(
          ComputeNumExecutionPorts(microarchitecture_->port_masks())),
      num_port_masks_(microarchitecture_->port_masks().size()),
      solver_(new MPSolver("DecompositionLPForInstruction",
                           MPSolver::GLPK_MIXED_INTEGER_PROGRAMMING)) {}

Status DecompositionSolver::Run(const ObservationVector& observations) {
  absl::flat_hash_map<std::string, double> key_val;
  for (const auto& observation : observations.observations()) {
    key_val[observation.event_name()] = observation.measurement();
  }
  // TODO(bdb): Only consider user-time measurements with the :u modifier.
  const double uops_retired =
      ::exegesis::gtl::FindWithDefault(key_val, "uops_retired:all", 0.0);
  std::vector<double> measurements(num_execution_ports_, 0.0);
  for (int port = 0; port < num_execution_ports_; ++port) {
    // We use 0.0 if the data does not exist. This may happen if the CPU
    // has fewer execution ports than Haswell. This means that it is the duty
    // of the PMU subsystem to check that the data it measures is properly
    // stored.
    // TODO(bdb): Add execution port information for architectures other than
    // Haswell.
    // TODO(bdb): Only consider user-time measurements with the :u modifier.
    measurements[port] = ::exegesis::gtl::FindWithDefault(
        key_val, absl::StrCat("uops_executed_port:port_", port), 0.0);
  }
  return Run(measurements, uops_retired);
}

Status DecompositionSolver::Run(const std::vector<double>& measurements,
                                double uops_retired) {
  const double kMaxError = 1.0;

  if (uops_retired > 50.0) {
    return util::InternalError(
        absl::StrCat("Too many uops to solve the problem",
                     absl::StrFormat("%.17g", uops_retired)));
  }
  // Compute the maximum number of uops per port mask. This enables us to
  // create less variables and to make the model easier to solve.
  std::vector<int> max_uops_per_mask(num_port_masks_, 0);
  for (int mask = 0; mask < num_port_masks_; ++mask) {
    double total_load = 0.0;
    for (const int port : microarchitecture_->port_masks()[mask]) {
      total_load += measurements[port];
    }
    max_uops_per_mask[mask] = static_cast<int>(total_load);
  }

  // Create load_[port][mask][n].
  load_.resize(num_execution_ports_);
  for (int port = 0; port < num_execution_ports_; ++port) {
    load_[port].resize(num_port_masks_);
    for (int mask = 0; mask < num_port_masks_; ++mask) {
      load_[port][mask].resize(max_uops_per_mask[mask]);
      for (int n = 0; n < max_uops_per_mask[mask]; ++n) {
        load_[port][mask][n] = solver_->MakeNumVar(
            0.0, 1.0, absl::StrCat("load_", port, "_", mask, "_", n));
      }
    }
  }

  // Create min_load_[mask][n].
  min_load_.resize(num_port_masks_);
  for (int mask = 0; mask < num_port_masks_; ++mask) {
    min_load_[mask].resize(max_uops_per_mask[mask]);
    for (int n = 0; n < max_uops_per_mask[mask]; ++n) {
      min_load_[mask][n] = solver_->MakeNumVar(
          0, MPSolver::infinity(), absl::StrCat("min_load_", mask, "_", n));
    }
  }

  // Create max_load_[mask][n].
  max_load_.resize(num_port_masks_);
  for (int mask = 0; mask < num_port_masks_; ++mask) {
    max_load_[mask].resize(max_uops_per_mask[mask]);
    for (int n = 0; n < max_uops_per_mask[mask]; ++n) {
      max_load_[mask][n] = solver_->MakeNumVar(
          0, MPSolver::infinity(), absl::StrCat("max_load_", mask, "_", n));
    }
  }

  // Create is_used_[mask][n].
  is_used_.resize(num_port_masks_);
  for (int mask = 0; mask < num_port_masks_; ++mask) {
    is_used_[mask].resize(max_uops_per_mask[mask]);
    for (int n = 0; n < max_uops_per_mask[mask]; ++n) {
      is_used_[mask][n] =
          solver_->MakeIntVar(0, 1, absl::StrCat("is_used_", mask, "_", n));
    }
  }

  error_.resize(num_execution_ports_);
  for (int port = 0; port < num_execution_ports_; ++port) {
    error_[port] =
        solver_->MakeNumVar(0.0, kMaxError, absl::StrCat("error_", port));
  }

  // max_error_ = max_port error_[port].
  max_error_ = solver_->MakeNumVar(0, MPSolver::infinity(), "max_error_");
  // \forall port max_error_ >= error_[port]
  for (int port = 0; port < num_execution_ports_; ++port) {
    MPConstraint* const max_constraint = solver_->MakeRowConstraint(
        0.0, MPSolver::infinity(), absl::StrCat("max_error_constraint_", port));
    max_constraint->SetCoefficient(error_[port], -1.0);
    max_constraint->SetCoefficient(max_error_, 1.0);
  }

  // Symmetry breaking between the is_used_'s:
  // \forall mask \forall n is_used_[mask][n+1] <= is_used_[mask][n].
  for (int mask = 0; mask < num_port_masks_; ++mask) {
    for (int n = 0; n < max_uops_per_mask[mask] - 1; ++n) {
      MPConstraint* const ordering_constraint = solver_->MakeRowConstraint(
          0.0, MPSolver::infinity(),
          absl::StrCat("is_used_", mask, "_", n + 1, "_le_uop_execution_", mask,
                       "_", n));
      ordering_constraint->SetCoefficient(is_used_[mask][n], 1.0);
      ordering_constraint->SetCoefficient(is_used_[mask][n + 1], -1.0);
    }
  }

  // Ports should only be used by port masks that reference them:
  // \forall port \notin mask load_[port][mask][n] = 0.
  for (int mask = 0; mask < num_port_masks_; ++mask) {
    for (int n = 0; n < max_uops_per_mask[mask]; ++n) {
      for (int port = 0; port < num_execution_ports_; ++port) {
        if (c_linear_search(microarchitecture_->port_masks()[mask], port))
          continue;
        load_[port][mask][n]->SetUB(0.0);
      }
    }
  }

  // \forall n \forall mask
  // \sum_{port \in mask} load_[port][mask][n] = is_used_[mask][n].
  for (int mask = 0; mask < num_port_masks_; ++mask) {
    for (int n = 0; n < max_uops_per_mask[mask]; ++n) {
      MPConstraint* const execution_constraint = solver_->MakeRowConstraint(
          0.0, 0.0,
          absl::StrCat("sum_over_port_in_mask_load_", mask, "_", n,
                       "_eq_uop_execution_", mask, "_", n));
      execution_constraint->SetCoefficient(is_used_[mask][n], -1.0);
      for (const int port : microarchitecture_->port_masks()[mask]) {
        execution_constraint->SetCoefficient(load_[port][mask][n], 1.0);
      }
    }
  }

  // \forall port \sum_{mask,n}
  // load_[port][mask][n] + error_[port] = measurement[port].
  for (int port = 0; port < num_execution_ports_; ++port) {
    MPConstraint* const measurement_constraint = solver_->MakeRowConstraint(
        measurements[port], measurements[port],
        absl::StrCat("sum_over_mask_in_port_", port, "_sum_over_n_load_", port,
                     "mask_n_plus_error_", port, "_eq_measurement_", port));
    measurement_constraint->SetCoefficient(error_[port], 1.0);
    for (int mask = 0; mask < num_port_masks_; ++mask) {
      for (int n = 0; n < max_uops_per_mask[mask]; ++n) {
        measurement_constraint->SetCoefficient(load_[port][mask][n], 1.0);
      }
    }
  }

  // \forall mask \sum_{port \in mask} error_[port] <= 1.0.
  for (int mask = 0; mask < num_port_masks_; ++mask) {
    MPConstraint* const max_error_constraint = solver_->MakeRowConstraint(
        0.0, 1.0,
        absl::StrCat("sum_over_port_in_mask_", mask, "_error_port_le_1"));
    for (const int port : microarchitecture_->port_masks()[mask]) {
      max_error_constraint->SetCoefficient(error_[port], 1.0);
    }
  }

  // \forall mask \forall n \forall port in mask
  // min_load_[mask][n] <= load_[port][mask][n].
  for (int mask = 0; mask < num_port_masks_; ++mask) {
    for (int n = 0; n < max_uops_per_mask[mask]; ++n) {
      for (const int port : microarchitecture_->port_masks()[mask]) {
        MPConstraint* const min_load_constraint = solver_->MakeRowConstraint(
            0, MPSolver::infinity(),
            absl::StrCat("min_load_constraint_", mask, "_", n, "_", port));
        min_load_constraint->SetCoefficient(min_load_[mask][n], -1.0);
        min_load_constraint->SetCoefficient(load_[port][mask][n], 1.0);
      }
    }
  }

  // \forall mask \forall n \forall port in mask
  // load_[port][mask][n] <= max_load_[mask][n].
  for (int mask = 0; mask < num_port_masks_; ++mask) {
    for (int n = 0; n < max_uops_per_mask[mask]; ++n) {
      for (const int port : microarchitecture_->port_masks()[mask]) {
        MPConstraint* const max_load_constraint = solver_->MakeRowConstraint(
            0, MPSolver::infinity(),
            absl::StrCat("max_load_constraint_", mask, "_", n, "_", port));
        max_load_constraint->SetCoefficient(max_load_[mask][n], 1.0);
        max_load_constraint->SetCoefficient(load_[port][mask][n], -1.0);
      }
    }
  }

  // num_uops_ = \sum_{mask,n} is_used_[mask][n]
  // num_uops_ >= floor(uops_retired)
  num_uops_ = solver_->MakeNumVar(floor(uops_retired), MPSolver::infinity(),
                                  "num_uops_");
  MPConstraint* const num_ops_equality =
      solver_->MakeRowConstraint(0, 0, "num_uops_eq_sum_is_used_sub_mask_n");
  num_ops_equality->SetCoefficient(num_uops_, -1.0);
  for (int mask = 0; mask < num_port_masks_; ++mask) {
    for (int n = 0; n < max_uops_per_mask[mask]; ++n) {
      num_ops_equality->SetCoefficient(is_used_[mask][n], 1.0);
    }
  }

  MPObjective* const objective = solver_->MutableObjective();
  objective->SetMinimization();
  const double kPortMaskSizeWeights[] = {1, 32, 16, 8, 4, 2, 1};
  const double kBalancingWeight = 10000.0;
  for (int mask = 0; mask < num_port_masks_; ++mask) {
    const int num_possible_execution_ports =
        microarchitecture_->port_masks()[mask].num_possible_ports();
    const double mask_size_weight =
        kPortMaskSizeWeights[num_possible_execution_ports];
    for (int n = 0; n < max_uops_per_mask[mask]; ++n) {
      // Objective function (O1): Favor masks with more alternatives.
      objective->SetCoefficient(is_used_[mask][n], mask_size_weight);
      // Objective function (O2): For a given mask, balance the consumptions
      // between the different micro-operations executing using that mask.
      objective->SetCoefficient(min_load_[mask][n], -kBalancingWeight);
      objective->SetCoefficient(max_load_[mask][n], kBalancingWeight);
    }
    // Objective function (O3): kErrorWeight * \sum_{port} \abs(error_[port]).
    const double kErrorWeight = 1000.0;
    for (int port = 0; port < num_execution_ports_; ++port) {
      objective->SetCoefficient(error_[port], kErrorWeight);
    }
    // Objective function (O4): kMaxErrorWeight * max_{port} \abs(error_[port]).
    const double kMaxErrorWeight = 1000.0;
    objective->SetCoefficient(max_error_, kMaxErrorWeight);
    // Objective function (O5): kNumUopsWeight * num_uops_.
    const double kNumUopsWeight = 1.0;
    objective->SetCoefficient(num_uops_, kNumUopsWeight);
  }
#ifdef NDEBUG
  const int kLimitInMs = 2000;
#else
  const int kLimitInMs = 20000;
#endif
  solver_->set_time_limit(kLimitInMs);
  switch (solver_->Solve()) {
    case MPSolver::OPTIMAL:
      FillInResults();
      return util::OkStatus();
    case MPSolver::FEASIBLE:
      return util::InternalError("Model is not optimal.");
    case MPSolver::INFEASIBLE:
      return util::InternalError("Model is infeasible.");
    case MPSolver::UNBOUNDED:
      return util::InternalError("Model is unbounded.");
    case MPSolver::ABNORMAL:
      return util::InternalError("Abnormal computation.");
    case MPSolver::MODEL_INVALID:
      return util::InternalError("Invalid model.");
    case MPSolver::NOT_SOLVED:
      return util::InternalError("Not solved.");
      // No default case as we want the compiler to check for the complete
      // treatment of all the cases.
  }
  return util::InternalError("Never reached.");
}

void DecompositionSolver::FillInResults() {
  const double kThreshold = 1e-6;
  port_masks_list_.clear();
  histogram_.clear();
  histogram_.assign(microarchitecture_->port_masks().size(), 0);
  port_loads_.clear();
  for (int mask = 0; mask < num_port_masks_; ++mask) {
    for (int n = 0; n < is_used_[mask].size(); ++n) {
      if (is_used_[mask][n]->solution_value() >= 1.0 - kThreshold) {
        ++histogram_[mask];
        const PortMask port_mask(microarchitecture_->port_masks()[mask]);
        port_masks_list_.push_back(port_mask);
        std::vector<double> loads;
        loads.reserve(num_execution_ports_);
        for (int port = 0; port < num_execution_ports_; ++port) {
          loads.push_back(load_[port][mask][n]->solution_value());
        }
        port_loads_.push_back(std::move(loads));
      } else {
        CHECK_GE(kThreshold, is_used_[mask][n]->solution_value());
      }
    }
  }
  CHECK(microarchitecture_->load_store_address_generation());
  CHECK(microarchitecture_->store_address_generation());
  CHECK(microarchitecture_->store_data());
  const int load_store_address_generation_mask_index =
      GetPositionInVector(microarchitecture_->port_masks(),
                          *microarchitecture_->load_store_address_generation());
  const int store_address_generation_mask_index =
      GetPositionInVector(microarchitecture_->port_masks(),
                          *microarchitecture_->store_address_generation());
  const int memory_buffer_write_mask_index = GetPositionInVector(
      microarchitecture_->port_masks(), *microarchitecture_->store_data());
  signature_ =
      OrderMicroOperations(histogram_, load_store_address_generation_mask_index,
                           store_address_generation_mask_index,
                           memory_buffer_write_mask_index, &is_order_unique_);
  error_values_.clear();
  error_values_.reserve(num_execution_ports_);
  max_error_value_ = max_error_->solution_value();
  for (const MPVariable* const error_var : error_) {
    error_values_.push_back(error_var->solution_value());
  }
}

std::string DecompositionSolver::DebugString() const {
  const double kThreshold = 1e-6;
  std::string output;
  DCHECK_EQ(port_masks_list_.size(), port_loads_.size());
  for (const int port_mask_index : signature_) {
    const PortMask& port_mask =
        microarchitecture_->port_masks()[port_mask_index];
    absl::StrAppend(&output, port_mask.ToString(), " ");
  }
  absl::StrAppend(&output, "\n");
  for (int i = 0; i < port_masks_list_.size(); ++i) {
    absl::StrAppend(&output, port_masks_list_[i].ToString(), ": {");
    for (int port = 0; port < num_execution_ports_; ++port) {
      const double load = port_loads_[i][port];
      if (load < kThreshold) continue;
      absl::StrAppendFormat(&output, "%d: %.5f, ", port, load);
    }
    absl::StrAppend(&output, "}\n");
  }
  absl::StrAppendFormat(&output, "max_error = %.5f\nerror {", max_error_value_);
  for (int port = 0; port < num_execution_ports_; ++port) {
    absl::StrAppendFormat(&output, "%d: %.5f, ", port, error_values_[port]);
  }
  absl::StrAppend(&output,
                  "}\nis_order_unique = ", static_cast<int>(is_order_unique()),
                  "\n");
  return output;
}

DecompositionSolver::MicroOps DecompositionSolver::GetMicroOps() const {
  MicroOps result;
  CHECK_EQ(signature_.size(), port_loads_.size());
  for (int i = 0; i < signature_.size(); ++i) {
    MicroOperationProto* const micro_op = result.Add();
    *micro_op->mutable_port_mask() =
        microarchitecture_->port_masks()[signature_[i]].ToProto();
    micro_op->set_latency(std::lround(
        std::accumulate(port_loads_[i].begin(), port_loads_[i].end(), 0.0)));
    // For now we cannot tell whether ports can be used in parallel, so we
    // assume the best case where all micro-ops are independent.
    // TODO(courbet): Make the dependencies a DAG when the information is
    // available.
  }
  return result;
}

std::vector<int> OrderMicroOperations(
    std::vector<int> histogram, int load_store_address_generation_mask_index,
    int store_address_generation_mask_index, int memory_buffer_write_mask_index,
    bool* is_order_unique) {
  std::vector<int> signature;
  const int expected_signature_size =
      std::accumulate(histogram.begin(), histogram.end(), 0);
  signature.reserve(expected_signature_size);

  // For each memory-buffer write micro-operation (p4 on Haswell) there must be
  // one generic address generation micro-operation
  // (p237, load_store_address_generation_mask_index) or write-address
  // generation micro-operation (p4, store_address_generation_mask_index).
  // We first pair the p4s with the p237s if any, and compute the number of
  // such pairs.
  const int num_p237_p4_pairs =
      std::min(histogram[memory_buffer_write_mask_index],
               histogram[store_address_generation_mask_index]);
  // Remove the number of pairs (store_address_generation_mask_index,
  // memory_buffer_write_mask_index) from the number of
  // memory_buffer_write_mask_index's and store_address_generation_mask_index's.
  histogram[memory_buffer_write_mask_index] -= num_p237_p4_pairs;
  histogram[store_address_generation_mask_index] -= num_p237_p4_pairs;
  // We then pair the p4s with the p23s if any.
  const int num_p23_p4_pairs =
      std::min(histogram[memory_buffer_write_mask_index],
               histogram[load_store_address_generation_mask_index]);
  histogram[memory_buffer_write_mask_index] -= num_p23_p4_pairs;
  histogram[load_store_address_generation_mask_index] -= num_p23_p4_pairs;
  // If there are p23s, (for example if the instruction reads from memory, and
  // does not write to it), put them first.
  for (int i = 0; i < histogram[load_store_address_generation_mask_index];
       ++i) {
    signature.push_back(load_store_address_generation_mask_index);
  }
  // Now all the generic address generation micro-operations have either been
  // paired with memory writes or have been pushed back to signature.
  histogram[load_store_address_generation_mask_index] = 0;
  // Do the same with p237s. TODO(bdb): check that this actually happens.
  for (int i = 0; i < histogram[store_address_generation_mask_index]; ++i) {
    signature.push_back(store_address_generation_mask_index);
  }
  histogram[store_address_generation_mask_index] = 0;
  // Now take care of the micro-operations that are not related to memory.
  int num_different_computation_port_masks = 0;
  for (int mask_index = 0; mask_index < histogram.size(); ++mask_index) {
    if (mask_index == store_address_generation_mask_index ||
        mask_index == load_store_address_generation_mask_index ||
        mask_index == memory_buffer_write_mask_index) {
      continue;
    }
    for (int k = 0; k < histogram[mask_index]; ++k) {
      signature.push_back(mask_index);
    }
    if (histogram[mask_index] != 0) {
      ++num_different_computation_port_masks;
      histogram[mask_index] = 0;
    }
  }
  // Now push all the pairs (load_store_address_generation_mask_index,
  // memory_buffer_write_mask_index).
  DCHECK_EQ(0, histogram[load_store_address_generation_mask_index]);
  // histogram[memory_buffer_write_mask_index] was
  // already decremented by the appropriate amount.
  for (int i = 0; i < num_p23_p4_pairs; ++i) {
    signature.push_back(load_store_address_generation_mask_index);
    signature.push_back(memory_buffer_write_mask_index);
  }
  // Now push all the pairs (store_address_generation_mask_index,
  // memory_buffer_write_mask_index).
  // NOTE(bdb): No need to clear histogram[store_address_generation_mask_index]
  // as it was already. memory_buffer_write_mask_index was already decremented
  // by the appropriate amount.
  for (int i = 0; i < num_p237_p4_pairs; ++i) {
    signature.push_back(store_address_generation_mask_index);
    signature.push_back(memory_buffer_write_mask_index);
  }
  // There may be remaining p4s. Push them.
  for (int i = 0; i < histogram[memory_buffer_write_mask_index]; ++i) {
    signature.push_back(memory_buffer_write_mask_index);
  }
  // Now that we have issued all the p4s, we can clear
  // histogram[memory_buffer_write_mask_index].
  histogram[memory_buffer_write_mask_index] = 0;
  // Check that all the port masks have correctly been consumed.
  for (int i = 0; i < histogram.size(); ++i) {
    CHECK_EQ(0, histogram[i]) << i;
  }
  // Check that the signature has the expected size.
  CHECK_EQ(expected_signature_size, signature.size());
  // If there is one or zero micro-operations using computation port masks,
  // then the ordering of the micro-operations is unique.
  *is_order_unique = (num_different_computation_port_masks <= 1);
  return signature;
}

}  // namespace itineraries
}  // namespace exegesis
