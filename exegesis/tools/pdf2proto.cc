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

#include "strings/string.h"

#include "gflags/gflags.h"

#include "exegesis/proto/pdf/pdf_document.pb.h"
#include "exegesis/util/pdf/xpdf_util.h"
#include "exegesis/util/proto_util.h"
#include "glog/logging.h"

DEFINE_string(exegesis_pdf_input_file, "",
              "filename or filename:start-end e.g. "
              "'file1.pdf' or 'file2.pdf:83-86'. "
              "Ranges are 1-based and inclusive. The upper bound can be 0 to "
              "process all the pages to the end. If no range is provided, "
              "the entire PDF is processed.");
DEFINE_string(exegesis_pdf_output_file, "", "Where to dump instructions.");
DEFINE_string(exegesis_pdf_patch_sets_file, "",
              "A set of patches to original documents.");
DEFINE_string(exegesis_pdf_input_request, "",
              "The input pdf specification as a PdfParseRequest text proto.");

namespace exegesis {
namespace pdf {
namespace {

void Main() {
  const bool is_file = !FLAGS_exegesis_pdf_input_file.empty();
  const bool is_request = !FLAGS_exegesis_pdf_input_request.empty();
  CHECK(is_file != is_request)
      << ", specify one of --exegesis_pdf_input_file or "
         "--exegesis_pdf_input_request";
  CHECK(!FLAGS_exegesis_pdf_output_file.empty())
      << "missing --exegesis_pdf_output_file";

  PdfDocumentsChanges patch_sets;
  if (!FLAGS_exegesis_pdf_patch_sets_file.empty()) {
    ReadTextProtoOrDie(FLAGS_exegesis_pdf_patch_sets_file, &patch_sets);
  }

  const auto pdf_request =
      is_request ? ParseProtoFromStringOrDie<PdfParseRequest>(
                       FLAGS_exegesis_pdf_input_request)
                 : ParseRequestOrDie(FLAGS_exegesis_pdf_input_file);

  WriteBinaryProtoOrDie(FLAGS_exegesis_pdf_output_file,
                        ParseOrDie(pdf_request, patch_sets));
}

}  // namespace
}  // namespace pdf
}  // namespace exegesis

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  ::exegesis::pdf::Main();
  return 0;
}
