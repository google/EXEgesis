// Copyright 2016 Google Inc.
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

#include "exegesis/util/pdf/pdf_document_utils.h"

#include <dirent.h>

#include <algorithm>
#include <map>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "exegesis/util/proto_util.h"
#include "glog/logging.h"
#include "util/gtl/map_util.h"
#include "util/task/status.h"

namespace exegesis {
namespace pdf {

namespace {

// Positive, in bound is simply itselt
//   GetIndex(5, 1) => 1
// Negative starts from the end of the array
//   GetIndex(5, -1) => 4
// Invalid index yields -1
//   GetIndex(5, 10) => -1
int GetIndex(int size, int index) {
  CHECK_GE(size, 0);
  if (index >= 0 && index >= size) return -1;
  if (index < 0 && -index > size) return -1;
  return index >= 0 ? index : size + index;
}

typedef std::vector<size_t> Hashes;
typedef Hashes::const_iterator HashesItr;

// Stores a mapping between a_index and b_index of length match_length.
struct EqualRangeReference {
  EqualRangeReference() = default;
  EqualRangeReference(size_t a_index, size_t b_index, size_t match_length)
      : a_index(a_index), b_index(b_index), match_length(match_length) {}

  size_t last_a_index() const { return a_index + match_length - 1; }

  size_t a_index;
  size_t b_index;
  size_t match_length;
};

// The value used to separate the two concatenated buffers used by the suffix
// array. It must be a small value not present in the original buffers.
// 0 fits nicely here, a special check is made when building the hashes to
// ensure they are never 0.
// More information here:  https://cs.stackexchange.com/a/9619
constexpr const size_t kSentinel = 0;

// A convenient structure holding the concatenation of a, kSentinel and b.
// 'a' and 'b' are the hashes of the blocks in the 'from' and 'to' documents
// respectively.
struct ConcatHashes {
  static Hashes ConcatenateHashes(const Hashes& a, const Hashes& b) {
    Hashes buffer;
    CHECK_GT(a.size(), 1);
    CHECK_GT(b.size(), 1);
    buffer.reserve(a.size() + b.size() + 1);
    buffer.insert(std::end(buffer), std::begin(a), std::end(a));
    buffer.push_back(kSentinel);
    buffer.insert(std::end(buffer), std::begin(b), std::end(b));
    return buffer;
  }

  ConcatHashes(const Hashes& a, const Hashes& b)
      : array(ConcatenateHashes(a, b)),
        begin(std::begin(array)),
        end_a(begin + a.size()),
        begin_b(end_a + 1),
        end(std::end(array)) {
    CHECK_EQ(std::count(begin, end, kSentinel), 1);
  }

  // Returns the size of the concatenated buffer.
  size_t size() const { return array.size(); }

  // Returns whether the iterator points in the a buffer or not.
  bool IsInA(const HashesItr& i) const { return i < end_a; }

  // Takes two iterators and compares then lexicographically.
  bool LexicographicCompare(const HashesItr& a, const HashesItr& b) const {
    return std::lexicographical_compare(a, end, b, end);
  }

  // Returns the index of itr within the original a or b buffer.
  size_t Index(const HashesItr& itr) const {
    CHECK(itr != end_a);
    return IsInA(itr) ? std::distance(begin, itr) : std::distance(begin_b, itr);
  }

  const Hashes array;
  const HashesItr begin;    // points to begin of array (it's also begin of a).
  const HashesItr end_a;    // points to end of a within array.
  const HashesItr begin_b;  // points to begin of b within array.
  const HashesItr end;      // points to end of array (it's also end of b).
};

// Finds all the matching subsequences between a and b. The algorithm is
// described here: https://cs.stackexchange.com/a/9619 and uses lcp array and
// suffix array.
std::vector<EqualRangeReference> GetMatchingRanges(const Hashes& a,
                                                   const Hashes& b) {
  // The concatenation of a, kSentinel and b.
  const ConcatHashes hashes(a, b);

  // Initialize suffix array pointers.
  std::vector<HashesItr> pointers;
  pointers.reserve(hashes.size());
  for (auto itr = hashes.begin; itr != hashes.end; ++itr) {
    pointers.push_back(itr);
  }
  // Get common suffixes.
  std::sort(std::begin(pointers), std::end(pointers),
            [&hashes](const HashesItr a, const HashesItr b) {
              return hashes.LexicographicCompare(a, b);
            });

  std::vector<EqualRangeReference> ranges;

  // We now traverse the suffix array and extract common prefixes for adjacent
  // pointers. See https://en.wikipedia.org/wiki/LCP_array.
  // LCP array would also find matching subsequences within a and within b, we
  // want only matching subsequences between a and b so we have to check that
  // iterators don't belong to the same set of hashes.
  bool is_first_iteration = true;
  HashesItr previous_itr;
  bool previous_is_in_a;
  for (const auto& itr : pointers) {
    const bool is_in_a = hashes.IsInA(itr);
    if (!is_first_iteration) {
      const auto pair = std::mismatch(previous_itr, hashes.end, itr);
      const auto match_length = std::distance(previous_itr, pair.first);
      if (match_length > 0 && is_in_a != previous_is_in_a) {
        const auto current_index = hashes.Index(itr);
        const auto previous_index = hashes.Index(previous_itr);
        if (is_in_a) {
          ranges.emplace_back(current_index, previous_index, match_length);
        } else {
          ranges.emplace_back(previous_index, current_index, match_length);
        }
      }
    }
    previous_itr = itr;
    previous_is_in_a = is_in_a;
    is_first_iteration = false;
  }
  return ranges;
}

// Computes the matching ranges and convert them into a block to block mapping
// starting by longest matches. We have a higher confidence that the blocks are
// referring to the same content if the matching length is high.
absl::flat_hash_map<size_t, size_t> GetBlockMapping(const Hashes& a,
                                                    const Hashes& b) {
  auto matching_ranges = GetMatchingRanges(a, b);

  // We order the results to get the longest matches first.
  std::sort(std::begin(matching_ranges), std::end(matching_ranges),
            [](const EqualRangeReference& a, const EqualRangeReference& b) {
              return a.match_length > b.match_length;
            });

  absl::flat_hash_map<size_t, size_t> block_mapping;
  for (const auto& match : matching_ranges) {
    // If this match is already included in a previous one we skip it.
    if (block_mapping.count(match.a_index) ||
        block_mapping.count(match.last_a_index()))
      continue;
    for (size_t i = 0; i < match.match_length; ++i) {
      block_mapping[match.a_index + i] = match.b_index + i;
    }
  }
  return block_mapping;
}

// A simple tuple to serve as a key in a map.
struct BlockPosition {
  BlockPosition() = default;
  BlockPosition(const BlockPosition&) = default;
  BlockPosition(size_t page, size_t row, size_t col)
      : page(page), row(row), col(col) {}

  bool operator<(const BlockPosition& other) const {
    return std::tie(page, row, col) <
           std::tie(other.page, other.row, other.col);
  }

  size_t page = 0;
  size_t row = 0;
  size_t col = 0;
};

std::ostream& operator<<(std::ostream& s, const BlockPosition& p) {
  return s << "[" << p.page << "," << p.row << "," << p.col << "]";
}

// Computes and stores various indexes:
// - PdfDocument's blocks by index
// - block's index by BlockPosition (page, row, col).
// - BlockPosition by block's index.
struct BlockIndex {
  BlockIndex(const BlockIndex&) = delete;
  BlockIndex(const PdfDocument& document) {
    const std::hash<std::string> fingerprint_;
    for (const auto& page : document.pages()) {
      for (const auto& row : page.rows()) {
        for (const auto& block : row.blocks()) {
          const auto index = blocks.size();
          const auto hash = fingerprint_(block.text());
          CHECK_NE(hash, kSentinel);  // Hash should never be kSentinel
          hashes.push_back(hash);
          blocks.push_back(&block);
          const BlockPosition position(page.number(), block.row(), block.col());
          gtl::InsertOrDieNoPrint(&position_to_index, position, index);
          gtl::InsertOrDieNoPrint(&index_to_position, index, position);
        }
      }
    }
  }

  size_t GetIndex(const BlockPosition position) const {
    return gtl::FindOrDieNoPrint(position_to_index, position);
  }

  BlockPosition GetPosition(size_t index) const {
    return gtl::FindOrDie(index_to_position, index);
  }

  Hashes hashes;
  std::vector<const PdfTextBlock*> blocks;
  std::map<BlockPosition, size_t> position_to_index;
  std::map<size_t, BlockPosition> index_to_position;
};

// Takes a mapping from blocks to blocks and tries to rewrite the input patch
// for the output document. If the patch is not part of the mapping we don't try
// to be smart and we simply give up.
bool RewritePatch(const absl::flat_hash_map<size_t, size_t>& block_mapping,
                  const BlockIndex& index_in, const BlockIndex& index_out,
                  const size_t patch_in_page, const PdfPagePatch& patch_in,
                  size_t* patch_out_page, PdfPagePatch* patch_out) {
  CHECK(patch_out_page);
  CHECK(patch_out);
  const BlockPosition in_pos(patch_in_page, patch_in.row(), patch_in.col());
  const size_t in_index = index_in.GetIndex(in_pos);
  const size_t* out_index = gtl::FindOrNull(block_mapping, in_index);
  if (out_index == nullptr) return false;
  const BlockPosition out_pos = index_out.GetPosition(*out_index);
  *patch_out_page = out_pos.page;
  *patch_out = patch_in;
  patch_out->set_row(out_pos.row);
  patch_out->set_col(out_pos.col);
  return true;
}

// Takes a mapping from pages to PdfPagePatch and write it as a
// PdfDocumentChanges.
void SetPatches(const std::map<size_t, std::vector<PdfPagePatch>>& page_patches,
                const PdfDocumentId& document_id, PdfDocumentChanges* patches) {
  *patches->mutable_document_id() = document_id;
  for (const auto& page_patch_pair : page_patches) {
    auto* page_patch = patches->add_pages();
    page_patch->set_page_number(page_patch_pair.first);
    for (const auto& patch : page_patch_pair.second) {
      *page_patch->add_patches() = patch;
    }
  }
}

}  // namespace

const PdfTextBlock* GetCellOrNull(const PdfPage& page, int row, int col) {
  const int row_index = GetIndex(page.rows_size(), row);
  if (row_index < 0) return nullptr;
  const PdfTextTableRow& row_data = page.rows(row_index);
  const int col_index = GetIndex(row_data.blocks_size(), col);
  if (col_index < 0) return nullptr;
  return &row_data.blocks(col_index);
}

const std::string& GetCellTextOrEmpty(const PdfPage& page, int row, int col) {
  const PdfTextBlock* const block = GetCellOrNull(page, row, col);
  static const std::string kEmpty;
  if (block == nullptr) return kEmpty;
  return block->text();
}

std::string* GetMutableCellTextOrNull(PdfPage* page, int row, int col) {
  const int row_index = GetIndex(page->rows_size(), row);
  if (row_index < 0) return nullptr;
  return GetMutableCellTextOrNull(page->mutable_rows(row_index), col);
}

std::string* GetMutableCellTextOrNull(PdfTextTableRow* row, int col) {
  const int col_index = GetIndex(row->blocks_size(), col);
  if (col_index < 0) return nullptr;
  return row->mutable_blocks(col_index)->mutable_text();
}

void ApplyPatchOrDie(const PdfPagePatch& patch, PdfPage* page) {
  std::string* text = GetMutableCellTextOrNull(page, patch.row(), patch.col());
  CHECK(text != nullptr) << "No valid cell for patch "
                         << patch.ShortDebugString();
  CHECK_EQ(*text, patch.expected())
      << "Can't apply patch " << patch.ShortDebugString();
  switch (patch.action_case()) {
    case PdfPagePatch::ACTION_NOT_SET:
      LOG(FATAL) << "action must be one of replacement or remove_cell for "
                 << patch.ShortDebugString();
      break;
    case PdfPagePatch::kReplacement:
      *text = patch.replacement();
      break;
    case PdfPagePatch::kRemoveCell: {
      CHECK(patch.remove_cell()) << "remove_cell must be true if set";
      // Remove the cell.
      auto* blocks = page->mutable_rows(patch.row())->mutable_blocks();
      blocks->erase(blocks->begin() + patch.col());
      // And renumber the blocks.
      for (size_t col = 0; col < blocks->size(); ++col) {
        blocks->Mutable(col)->set_col(col);
      }
      break;
    }
  }
}

std::vector<const PdfTextTableRow*> GetPageBodyRows(const PdfPage& page,
                                                    const float margin,
                                                    const int max_row) {
  const float top_margin = margin;
  const float bottom_margin = page.height() - margin;
  std::vector<const PdfTextTableRow*> result;
  int row_count = 0;
  for (const auto& row : page.rows()) {
    if (row_count == max_row) {
      break;
    }
    if (row.bounding_box().top() > top_margin &&
        row.bounding_box().bottom() < bottom_margin) {
      result.push_back(&row);
      ++row_count;
    }
  }
  return result;
}

PdfDocumentsChanges LoadConfigurations(const std::string& directory) {
  PdfDocumentsChanges patch_sets;
  DIR* dir = nullptr;
  struct dirent* ent = nullptr;
  if ((dir = opendir(directory.c_str())) != nullptr) {
    while ((ent = readdir(dir)) != nullptr) {
      const std::string full_path = absl::StrCat(directory, "/", ent->d_name);
      LOG(INFO) << "Reading configuration file " << full_path;
      CHECK_OK(ReadTextProto(full_path, patch_sets.add_documents()));
    }
    closedir(dir);
  }
  return patch_sets;
}

const PdfDocumentChanges* GetConfigOrNull(const PdfDocumentsChanges& patch_sets,
                                          const PdfDocumentId& document_id) {
  for (const auto& document : patch_sets.documents()) {
    const auto& current_id = document.document_id();
    if (current_id.title() == document_id.title() &&
        current_id.creation_date() == document_id.creation_date() &&
        current_id.modification_date() == document_id.modification_date()) {
      return &document;
    }
  }
  return nullptr;
}

void TransferPatches(const PdfDocumentChanges& changes, const PdfDocument& from,
                     const PdfDocument& to,
                     PdfDocumentChanges* successful_patches,
                     PdfDocumentChanges* failed_patches) {
  LOG(INFO) << "Building index for original document";
  const BlockIndex index_in(from);
  LOG(INFO) << "Building index for destination document";
  const BlockIndex index_out(to);
  LOG(INFO) << "Finding text block matches";
  const auto block_mapping = GetBlockMapping(index_in.hashes, index_out.hashes);
  LOG(INFO) << "Processing patches";
  std::map<size_t, std::vector<PdfPagePatch>> successful_page_patches;
  std::map<size_t, std::vector<PdfPagePatch>> failed_page_patches;
  for (const auto& page_changes : changes.pages()) {
    const size_t patch_in_page = page_changes.page_number();
    for (const auto& patch : page_changes.patches()) {
      size_t patch_out_page;
      PdfPagePatch patch_out;
      if (RewritePatch(block_mapping, index_in, index_out, patch_in_page, patch,
                       &patch_out_page, &patch_out)) {
        successful_page_patches[patch_out_page].push_back(patch_out);
      } else {
        failed_page_patches[patch_in_page].push_back(patch);
      }
    }
  }
  SetPatches(successful_page_patches, to.document_id(), successful_patches);
  SetPatches(failed_page_patches, from.document_id(), failed_patches);
}

}  // namespace pdf
}  // namespace exegesis
