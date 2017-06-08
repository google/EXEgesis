#include "exegesis/util/file_util.h"

#include <cstdio>
#include <memory>

#include "glog/logging.h"

namespace exegesis {
namespace {

// Use a 1 MB buffer for the operations. This should be enough for most files
// that we deal with.
constexpr size_t kBufferSize = 1 << 20;

// Returns true if 'source_or_target' should be treated as a file name; returns
// false if STDIN/STDOUT should be used instead.
bool IsFileName(const string& source_or_target) {
  return source_or_target != "-";
}

}  // namespace

string ReadTextFromFileOrStdInOrDie(const string& source) {
  CHECK(!source.empty());
  const bool is_file = IsFileName(source);
  FILE* const input = is_file ? fopen(source.c_str(), "rb") : stdin;
  CHECK(input != nullptr) << "Could not open '" << source << "'";
  std::unique_ptr<char[]> buffer(new char[kBufferSize]);
  string out;
  size_t read_items = 0;
  do {
    read_items = fread(buffer.get(), sizeof(char), kBufferSize, input);
    out.append(buffer.get(), read_items);
  } while (read_items == kBufferSize);
  CHECK(feof(input));
  if (is_file) fclose(input);
  return out;
}

void WriteTextToFileOrStdOutOrDie(const string& target, const string& data) {
  CHECK(!target.empty());
  const bool is_file = IsFileName(target);
  FILE* const output = is_file ? fopen(target.c_str(), "wb") : stdout;
  CHECK(output != nullptr) << "Could not open '" << target << "'";
  const size_t written_items =
      fwrite(data.data(), sizeof(char), data.size(), output);
  CHECK_EQ(written_items, data.size());
  fflush(output);
  if (is_file) fclose(output);
}

}  // namespace exegesis
