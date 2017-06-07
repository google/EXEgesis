#ifndef THIRD_PARTY_CPU_INSTRUCTIONS_CPU_INSTRUCTIONS_UTIL_TEXT_PROCESSING_H_
#define THIRD_PARTY_CPU_INSTRUCTIONS_CPU_INSTRUCTIONS_UTIL_TEXT_PROCESSING_H_

#include "strings/string.h"

namespace exegesis {

// Remove syllabification.
//
// eg.
// "Here is a para-
// graph with an hypen." -> "Here is a paragraph with an hypen."
string Dehyphenate(string input);
void DehyphenateinPlace(string* input);

// Remove leading and trailing whitespace in a multiline string.
string RemoveLineLeadingTrailingWhitespace(string input);
void RemoveLineLeadingTrailingWhitespaceInPlace(string* input);

// Transforms \r\n into \n
string CleanupLineFeed(string input);
void CleanupLineFeedInPlace(string* input);

// Keep a maximum of two consecutive \n.
string CondenseLineFeeds(string input);
void CondenseLineFeedsInPlace(string* input);

// Unfolds a paragraph by joining all lines and removing hyphen when necessary.
// If a line ends with a dot, the next line will not be joined.
// It also removes leading/trailing whitespace and normalizes line feed.
//
// eg.
// "This is a single line.
//
// This is a paragraph that
// wraps and even has hu-
// mon-
// gous words with -." -> "This is a single line.
//
// This is a paragraph that wraps and even has humongous words with -."
string CleanupParagraph(string input);

}  // namespace exegesis

#endif  // THIRD_PARTY_CPU_INSTRUCTIONS_CPU_INSTRUCTIONS_UTIL_TEXT_PROCESSING_H_
