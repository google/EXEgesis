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

#ifndef EXEGESIS_ITINERARIES_DECOMPOSITION_H_
#define EXEGESIS_ITINERARIES_DECOMPOSITION_H_

#include <algorithm>
#include <string>
#include <vector>

#include "exegesis/base/microarchitecture.h"
#include "exegesis/base/port_mask.h"
#include "exegesis/proto/instructions.pb.h"
#include "exegesis/proto/microarchitecture.pb.h"
#include "ortools/linear_solver/linear_solver.h"
#include "src/google/protobuf/repeated_field.h"
#include "util/task/status.h"

// Mixed Integer Programming model for decomposing an observation into execution
// port masks.
//
// The goal of the model is to find a collection of port masks that best
// explains the measurements from the performance counters while taking into
// account possible noise (denoted as error) in the measurements.

// A port mask is a list of possible execution ports for a given
// micro-operation. A micro-operation "uses" a port mask if it can execute using
// any (and only one) of the execution ports in the port mask.
//
// Each execution port of a port mask is denoted by its port number.
//
// Some instructions decompose into several micro-operations that use the same
// execution port masks. At this level of abstraction, such micro-operations are
// interchangeable. Micro-operations using the same port mask are given an index
// 'n' which does not correspond to any ranking between them.
//
// Let is_used_[mask][n] be a binary variable denoting whether
// the nth micro-operation using 'mask' is executed.
//
// Let load_[port][mask][n] be a continuous variable in [0, 1.0] that
// represents the average consumptiom of micro-operations using 'mask' that
// are executed on each 'port' during the measurements. Since there can be more
// than one micro-operation using 'mask' in a given instruction, each
// micro-operation is given an index 'n'.
//

// To avoid symmetric solutions, we add the constraints:
// (C1) is_used_[mask][n+1] <= is_used_[mask][n],
//
// A micro-operation consumes resources if and only if it is executed. When it
// is executed (is_used_[mask][n] = 1.0) the constraint also means that 100%
// of the resource usage for the micro-operation is provisioned:
// (C2) \forall n \forall mask
//      sum_{port \in mask} load_[port][mask][n] == is_used_[mask][n].
//
// The micro-operations executed using 'mask' are only executed using those
// ports in 'mask':
// (C3) \forall mask \forall n \forall port \notin mask
//      load_[port][mask][n] = 0.0.
//
// We assume that the number of micro-operations executed is greater than
// what the performance counters ("uops_retired:all" on Haswell, which we call
// uops_executed for clarity below) report.
// TODO(bdb): check whether this is truly necessary.
// (C4) \sum_{mask,n} is_used_[mask][n] >= floor(uops_retired)
//
// The following relates load_, error, and measurement.
// (C5) \forall port \sum_{mask,n}
//      load_[port][mask][n] + error[port] = measurement[port].
//
// The following counts the number of micro-operations that are inferred from
// the measurements:
// (C6) num_uops_ = \sum_{mask,n} is_used_[mask][n]
//      num_uops_ >= floor(uops_retired):
//
// There are multiple, conflicting goals:
// (O1) Favor instructions using masks with high cardinality.
// (O2) Minimize the difference between the execution port with the highest
//      load_ value and and the execution port with the lowest load_ value
//      for a given port mask.
// (O3) Deal with the error in the measurement. Ideally we would minimize the
//      L2-norm of the error, but it would require an LCQP MIP solver. We
//      therefore chose the sum of the errors on each port.
// (O4) We wlso minimize the L1-norm of the error on all ports.
// (O5) Minimize num_uops_ so that it be as close as possible to its
//      lower-bound.
//
// The terms of the objective function are therefore:
// (O1) \sum_{mask} \sum_n K^(#num_port-mask_size) is_used_[mask][n] +
// (O2) \sum_{mask} \sum_n kBalancingWeight * max_port load_[mask][n] +
// (O2) \sum_{mask} \sum_n -kBalancingWeight * min_port load_[mask][n] +
// (O3) \sum_{port} kErrorWeight * \abs(error_[port]) +
// (O4) kMaxErrorWeight * max_{port} \abs(error_[port]) +
// (O5) kNumUopsWeight * num_uops_,
// where K, kBalancingWeight, kErrorWeight, kMaxErrorWeight, and kNumUopsWeight
// are appropriately chosen constants.

namespace exegesis {
namespace itineraries {

using ::exegesis::util::Status;

// Computes the number of execution ports from 'port_masks'. It is simply 1 +
// the maximum port number in 'port_masks'.
int ComputeNumExecutionPorts(const std::vector<PortMask>& port_masks);

template <typename T>
int GetPositionInVector(const std::vector<T>& container, const T& value) {
  auto it = std::find(container.begin(), container.end(), value);
  CHECK(it != container.end());
  return it - container.begin();
}

class DecompositionSolver {
 public:
  using MicroOps = google::protobuf::RepeatedPtrField<MicroOperationProto>;

  // The argument should outlive the solver.
  explicit DecompositionSolver(const MicroArchitecture& microarchitecture);

  // Runs the decomposition solver on 'observation'.
  Status Run(const ObservationVector& observation);

  // Runs the decomposition solver on 'measurements'. 'num_uops' is the number
  // of micro-operations measured by the performance counters.
  Status Run(const std::vector<double>& measurements, double num_uops);

  // Returns a string detailing the port masks used by each micro-operation,
  // and the allocation of each execution port to each micro-operation.
  std::string DebugString() const;

  // Returns the result as list of micro-operations.
  MicroOps GetMicroOps() const;

  // Returns the time spent to solve the underlying MIP.
  double wall_time() const { return solver_->wall_time(); }

  // Returns the value of the objective function after minimization.
  double objective_value() const { return solver_->Objective().Value(); }

  // Returns the list of port masks corresponding to each micro-operation
  // of the instruction.
  const std::vector<PortMask>& port_masks_list() const {
    return port_masks_list_;
  }

  // Returns a list of vectors representing the load on each port for each of
  // the micro-operation in the same order as with port_masks_list().
  const std::vector<std::vector<double>>& port_loads() const {
    return port_loads_;
  }

  // Returns the signature of the instruction, i.e. the list of all the port
  // masks it is using according to the result of the decomposition.
  const std::vector<int>& signature() const { return signature_; }

  // Returns the histogram of the instruction, i.e. how many times each port
  // mask it is using according to the result of the decomposition.
  const std::vector<int>& histogram() const { return histogram_; }

  // Returns the measurements assigned to error for each of the ports.
  const std::vector<double>& error_values() const { return error_values_; }

  double max_error_value() const { return max_error_value_; }

  bool is_order_unique() const { return is_order_unique_; }

 private:
  // Fills in the results (port_masks_list_, port_loads_, error_values_) at the
  // end of Run().
  void FillInResults();

  // The CPU microarchitecture for which to solve. Not owned.
  const MicroArchitecture* microarchitecture_;

  // The number of execution ports, as computed from execution_port_masks_.
  const int num_execution_ports_;

  // The number of port masks.
  const int num_port_masks_;

  // port_masks_list_[n] is the port mask used by micro-operation n.
  std::vector<PortMask> port_masks_list_;

  // The signature of the instruction, i.e. the list of all the port masks it
  // is using according to the result of the decomposition.
  std::vector<int> signature_;

  // The histogram of the instruction, i.e. how many times each port
  // mask it is using according to the result of the decomposition.
  std::vector<int> histogram_;

  // port_loads_[n][port] contains the load of 'port' for micro-operation n.
  std::vector<std::vector<double>> port_loads_;

  // error_values_[port] is the measurement error on port 'port'.
  std::vector<double> error_values_;

  // The maximum measurement error on all ports.
  double max_error_value_;

  // True if the order between micro-operations computed by
  // OrderMicroOperations is unique.
  bool is_order_unique_;

  // The underlying MIP solver.
  std::unique_ptr<operations_research::MPSolver> solver_;

  // is_used_[mask][n] is a binary variable representing that the nth
  // micro-operation using 'mask' is executed.
  std::vector<std::vector<operations_research::MPVariable*>> is_used_;

  // load_[port][mask][n] represents the share of micro-operations
  // in 'mask' that are executed on 'port'. Since there can be more than one
  // micro-operation using 'mask', each micro-operation is given an index
  // 'n'.
  std::vector<std::vector<std::vector<operations_research::MPVariable*>>> load_;

  // min_load_[mask][n] represents the minimum load for a given 'mask'
  // and a given 'n'.
  std::vector<std::vector<operations_research::MPVariable*>> min_load_;

  // max_load_[mask][n] represents the maximum load for a given 'mask'
  // and a given 'n'.
  std::vector<std::vector<operations_research::MPVariable*>> max_load_;

  // error_[port] is a non-negative continuous variable representing the work
  // that cannot be assigned to any micro-operation.
  // TODO(bdb): Check that the error is indeed nonnegative.
  std::vector<operations_research::MPVariable*> error_;

  // The maximum error: \forall port, max_error_ >= error_[port].
  operations_research::MPVariable* max_error_;

  // The number of micro-operations executed, as found by the decomposition
  // model: num_uops_ = sum_{mask, n} is_used[mask][n].
  // num_uops_ is also lower-bounded by the number of measured micro-operations
  // num_uops_ >= floor(measured micro-operations);
  operations_research::MPVariable* num_uops_;
};

// Returns a signature with the port mask indices in the order in which the
// micro-operations were executed. For this the memory reads (read address
// generation) micro-operations are put first, and the memory write (write
// address generation, then write to memory buffer) are put last.
// is_order_unique is set to true if the order between the
// micro-operations is unique, i.e. when:
// - there is at most one type of port mask excepting the memory
//   micro-operations,
// - there are two types of port mask and they were ordered using
//   instruction collision.
// TODO(bdb): implement instruction collision.
std::vector<int> OrderMicroOperations(
    std::vector<int> histogram, int load_store_address_generation_mask_index,
    int store_address_generation_mask_index, int memory_buffer_write_mask_index,
    bool* is_order_unique);

}  // namespace itineraries
}  // namespace exegesis

#endif  // EXEGESIS_ITINERARIES_DECOMPOSITION_H_
