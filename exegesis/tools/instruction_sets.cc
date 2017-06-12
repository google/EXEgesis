#include "exegesis/tools/instruction_sets.h"

#include "exegesis/base/architecture_provider.h"
#include "exegesis/base/cleanup_instruction_set.h"
#include "exegesis/base/restrict.h"
#include "exegesis/base/transform_factory.h"
#include "exegesis/proto/instructions.pb.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "strings/str_split.h"
#include "strings/string_view_utils.h"
#include "util/gtl/map_util.h"

DEFINE_string(exegesis_architecture, "",
              "The name of the architecture for which the code is optimized."
              "If 'intel', then the raw parsed output (stright out of SDM) is"
              "returned."
              "If this is not one of the known sources, we'll try to interpret "
              "this as a file.");

DEFINE_string(exegesis_cpu_model, "intel:06_3F",
              "The id of the CPU model for which the code is optimized.");

DEFINE_string(exegesis_first_mnemonic, "", "First mnemonic.");
DEFINE_string(exegesis_last_mnemonic, "ZZZZ", "Last mnemonic (included).");

namespace exegesis {

void CheckArchitectureFlag() {
  CHECK(!FLAGS_exegesis_architecture.empty())
      << "Please provide an architecture (e.g. 'pbtxt:/path/to/file.pb.txt')";
}

InstructionSetProto GetTransformedInstructionSetFromCommandLineFlags() {
  CheckArchitectureFlag();
  return GetTransformedInstructionSet(FLAGS_exegesis_architecture);
}

InstructionSetProto GetTransformedInstructionSet(const string& architecture) {
  InstructionSetProto instruction_set =
      GetArchitectureProtoOrDie(architecture)->raw_instruction_set();

  // Apply transformations.
  CHECK_OK(RunTransformPipeline(GetTransformsFromCommandLineFlags(),
                                &instruction_set));

  // Restrict mnemonic range.
  exegesis::RestrictToMnemonicRange(FLAGS_exegesis_first_mnemonic,
                                    FLAGS_exegesis_last_mnemonic,
                                    &instruction_set);
  return instruction_set;
}

MicroArchitectureData GetMicroArchitectureDataFromCommandLineFlags() {
  CheckArchitectureFlag();
  return MicroArchitectureData::ForCpuModelId(
             GetArchitectureProtoOrDie(FLAGS_exegesis_architecture),
             FLAGS_exegesis_cpu_model)
      .ValueOrDie();
}

}  // namespace exegesis
