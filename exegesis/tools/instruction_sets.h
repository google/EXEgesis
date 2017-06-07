// Provides access to instruction sets for all supported architectures.

#ifndef RESEARCH_DEVTOOLS_EXEGESIS_TOOLS_INSTRUCTION_SETS_H_
#define RESEARCH_DEVTOOLS_EXEGESIS_TOOLS_INSTRUCTION_SETS_H_

#include "exegesis/base/microarchitecture.h"
#include "exegesis/proto/instructions.pb.h"

namespace exegesis {

// Returns the instruction set for the architecture specified in the
// command-line flag --exegesis_architecture. Optionally applies the
// transformation given in --exegesis_transforms. Only keeps mnemonics
// in the range [--exegesis_first_mnemonic,
// --exegesis_last_mnemonic].
InstructionSetProto GetTransformedInstructionSetFromCommandLineFlags();

// Same as above, but reads from architecture instead of
// --exegesis_architecture.
InstructionSetProto GetTransformedInstructionSet(const string& architecture);

// Returns the instruction set and itineraries for the microarchitecture
// specified in the command-line flag --exegesis_cpu_model.
MicroArchitectureData GetMicroArchitectureDataFromCommandLineFlags();

}  // namespace exegesis

#endif  // RESEARCH_DEVTOOLS_EXEGESIS_TOOLS_INSTRUCTION_SETS_H_
