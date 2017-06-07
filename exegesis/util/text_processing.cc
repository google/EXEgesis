#include "exegesis/util/text_processing.h"

#include "re2/re2.h"

namespace exegesis {

void DehyphenateInPlace(string* input) {
  // We need to match characters before hypen so we don't join items in lists.
  static const LazyRE2 re = {R"((\pL)-\r?\n(\pL))"};
  RE2::GlobalReplace(input, *re, R"(\1\2)");
}

string Dehyphenate(string input) {
  DehyphenateInPlace(&input);
  return input;
}

void RemoveLineLeadingTrailingWhitespaceInPlace(string* input) {
  static const LazyRE2 leading_whitespace = {R"((?m:^([ \t\p{Zs}]+)))"};
  RE2::GlobalReplace(input, *leading_whitespace, "");

  static const LazyRE2 trailing_whitespace = {R"((?m:([ \t\p{Zs}]+)$))"};
  RE2::GlobalReplace(input, *trailing_whitespace, "");
}

string RemoveLineLeadingTrailingWhitespace(string input) {
  RemoveLineLeadingTrailingWhitespaceInPlace(&input);
  return input;
}

void CleanupLineFeedInPlace(string* input) {
  static const LazyRE2 cleanup_line_feed = {R"((\r?\n))"};
  RE2::GlobalReplace(input, *cleanup_line_feed, "\n");
}

string CleanupLineFeed(string input) {
  CleanupLineFeedInPlace(&input);
  return input;
}

void CondenseLineFeedsInPlace(string* input) {
  static const LazyRE2 condense_line_feed = {R"((\n){3,})"};
  RE2::GlobalReplace(input, *condense_line_feed, "\n\n");
}

string CondenseLineFeeds(string input) {
  CondenseLineFeedsInPlace(&input);
  return input;
}

string CleanupParagraph(string input) {
  RemoveLineLeadingTrailingWhitespaceInPlace(&input);
  CleanupLineFeedInPlace(&input);
  CondenseLineFeedsInPlace(&input);
  DehyphenateInPlace(&input);
  static const LazyRE2 re = {R"(([^.\n])\n([^-\n]))"};
  RE2::GlobalReplace(&input, *re, R"(\1 \2)");
  return input;
}

}  // namespace exegesis
