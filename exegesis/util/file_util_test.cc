#include "exegesis/util/file_util.h"

#include "strings/string.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "strings/str_cat.h"

namespace exegesis {
namespace {

TEST(FileUtilTest, WriteToFileAndReadItAgain) {
  constexpr char kContents[] = "Hello world!";
  const string test_file = StrCat(getenv("TEST_TMPDIR"), "testfile");
  WriteTextToFileOrStdOutOrDie(test_file, kContents);
  const string contents_from_file = ReadTextFromFileOrStdInOrDie(test_file);
  EXPECT_EQ(contents_from_file, kContents);
}

}  // namespace
}  // namespace exegesis
