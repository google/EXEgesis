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

// This program converts a pdf document in binary proto format:
// Usage:
// bazel run -c opt \
// exegesis/tools:pdf2proto -- \
// --exegesis_pdf_input_file=/path/to/file.pdf \
// --exegesis_pdf_output_file=/path/to/file.pdf.pb

#include <string>

#include "absl/flags/flag.h"
#include "exegesis/base/init_main.h"
#include "exegesis/proto/pdf/pdf_document.pb.h"
#include "exegesis/util/pdf/xpdf_util.h"
#include "exegesis/util/proto_util.h"
#include "glog/logging.h"
#include "util/task/status.h"

ABSL_FLAG(std::string, exegesis_pdf_input_file, "",
          "filename or filename:start-end e.g. "
          "'file1.pdf' or 'file2.pdf:83-86'. "
          "Ranges are 1-based and inclusive. The upper bound can be 0 to "
          "process all the pages to the end. If no range is provided, "
          "the entire PDF is processed.");
ABSL_FLAG(std::string, exegesis_pdf_output_file, "",
          "Where to dump instructions.");
ABSL_FLAG(std::string, exegesis_pdf_patch_sets_file, "",
          "A set of patches to original documents.");
ABSL_FLAG(std::string, exegesis_pdf_input_request, "",
          "The input pdf specification as a PdfParseRequest text proto.");

namespace exegesis {
namespace pdf {
namespace {

void Main() {
  const std::string exegesis_pdf_input_file =
      absl::GetFlag(FLAGS_exegesis_pdf_input_file);
  const std::string exegesis_pdf_input_request =
      absl::GetFlag(FLAGS_exegesis_pdf_input_request);
  const std::string exegesis_pdf_output_file =
      absl::GetFlag(FLAGS_exegesis_pdf_output_file);
  const bool is_file = !exegesis_pdf_input_file.empty();
  const bool is_request = !exegesis_pdf_input_request.empty();
  CHECK(is_file != is_request)
      << ", specify one of --exegesis_pdf_input_file or "
         "--exegesis_pdf_input_request";
  CHECK(!absl::GetFlag(FLAGS_exegesis_pdf_output_file).empty())
      << "missing --exegesis_pdf_output_file";

  PdfDocumentsChanges patch_sets;
  if (!absl::GetFlag(FLAGS_exegesis_pdf_patch_sets_file).empty()) {
    CHECK_OK(ReadTextProto(absl::GetFlag(FLAGS_exegesis_pdf_patch_sets_file),
                           &patch_sets));
  }

  const auto pdf_request = is_request
                               ? ParseProtoFromStringOrDie<PdfParseRequest>(
                                     exegesis_pdf_input_request)
                               : ParseRequestOrDie(exegesis_pdf_input_file);

  WriteBinaryProtoOrDie(exegesis_pdf_output_file,
                        ParseOrDie(pdf_request, patch_sets));
}

}  // namespace
}  // namespace pdf
}  // namespace exegesis

int main(int argc, char** argv) {
  exegesis::InitMain(argc, argv);
  ::exegesis::pdf::Main();
  return 0;
}
