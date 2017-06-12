// A tool to compute itineraries for an instruction set.

#include <functional>
#include <utility>

#include "gflags/gflags.h"

#include "exegesis/base/cpu_model.h"
#include "exegesis/itineraries/compute_itineraries.h"
#include "exegesis/tools/instruction_sets.h"
#include "exegesis/util/proto_util.h"
#include "exegesis/util/system.h"
#include "glog/logging.h"

DEFINE_string(exegesis_output_itineraries, "",
              "File where to store the computed itineraries in Proto format.");
DEFINE_int32(exegesis_pin_to_core, 0,
             "Pin the process to the given core. This helps for getting more "
             "reliable results.");

namespace exegesis {

void Main() {
  SetCoreAffinity(FLAGS_exegesis_pin_to_core);

  const auto microarchitecture_data =
      GetMicroArchitectureDataFromCommandLineFlags();

  InstructionSetItinerariesProto itineraries =
      microarchitecture_data.itineraries();
  LOG(ERROR) << itineraries::ComputeItineraries(
      microarchitecture_data.instruction_set(), &itineraries);

  WriteTextProtoOrDie(FLAGS_exegesis_output_itineraries, itineraries);
}

}  // namespace exegesis

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  CHECK(!FLAGS_exegesis_output_itineraries.empty())
      << "Please specify the output.";
  exegesis::Main();
  return 0;
}
