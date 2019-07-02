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

#include "exegesis/util/pdf/pdf_document_parser.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "exegesis/util/pdf/geometry.h"
#include "util/graph/connected_components.h"
#include "util/gtl/map_util.h"

ABSL_FLAG(double, exegesis_pdf_max_character_distance, 0.9,
          "The maximal distance of two characters to be considered part of "
          "the same cell. The value is a multiplier; the real distance is "
          "obtained by multiplying the font size with this coefficient.");

namespace exegesis {
namespace pdf {

namespace {

void Union(const BoundingBox& a, BoundingBox* b) { *b = Union(a, *b); }

// Returns the direction vector corresponding to value's orientation.
// +---+
// |   |
// | +------->  returns  +-->
// |   |
// +---+
template <typename T>
Vec2F GetForwardDirection(const T& value) {
  return GetDirectionVector(value.orientation());
}

// Returns the direction vector corresponding to value's orientation rotated by
// 90 degrees.
// +---+
// |   |
// | +------->  returns  +
// |   |                 |
// +---+                 v
template <typename T>
Vec2F GetSidewaysDirection(const T& value) {
  return GetDirectionVector(RotateClockwise90(value.orientation()));
}

// Computes the Vec2F going from a's center to b's center.
template <typename T>
Vec2F GetVector(const T& a, const T& b) {
  return GetCenter(b.bounding_box()) - GetCenter(a.bounding_box());
}

// Helper class providing indexed access to characters.
// Indexed access is needed to use ConnectedComponent.
class Characters {
 public:
  Characters(const PdfCharacters* characters, const BoundingBox& page)
      : characters_(characters), tree_(page) {
    centers_.reserve(characters_->size());
    size_t i = 0;
    for (const auto& character : *characters_) {
      const auto& center = GetCenter(character.bounding_box());
      centers_.push_back(center);
      tree_.Insert(i++, center);
    }
  }

  size_t size() const { return characters_->size(); }

  const PdfCharacter& Get(size_t index) const {
    return characters_->Get(index);
  }

  // Gathers characters close to the one pointed to by 'index' to prune the
  // O(N^2) search.
  const Indices GetCandidates(size_t index) const {
    const auto character = Get(index);
    const auto center = GetCenter(character.bounding_box());
    const float size = character.font_size() * 2.0f;
    Indices indices;
    tree_.QueryRange(CreateBox(center, size, size), &indices);
    return indices;
  }

 private:
  const PdfCharacters* const characters_;
  std::vector<Point> centers_;
  QuadTree tree_;
};

std::vector<Indices> GetClusters(DenseConnectedComponentsFinder* finder) {
  std::map<int, Indices> all_indices;
  const std::vector<int> component_ids = finder->GetComponentIds();
  for (size_t i = 0; i < component_ids.size(); ++i) {
    all_indices[component_ids[i]].push_back(i);
  }
  std::vector<Indices> output;
  output.reserve(all_indices.size());
  for (auto& id_indices_pair : all_indices) {
    output.push_back(std::move(id_indices_pair.second));
  }
  return output;
}

// Actually clusters the characters by retaining the closest character in the
// forward direction and linking them together in PdfTextSegments.
void ClusterCharacters(const Characters& all, PdfTextSegments* segments) {
  // Returns FLT_MAX if characters[b] is not on the same line, backward or too
  // far away from characters[a].
  const auto GetCharacterDistance = [&all](size_t index_a,
                                           size_t index_b) -> float {
    const auto& a = all.Get(index_a);
    const auto& b = all.Get(index_b);
    const bool same_orientation = a.orientation() == b.orientation();
    const Vec2F a2b = GetVector(a, b);
    const float forward_distance = a2b.dot_product(GetForwardDirection(a));
    const float sideways_distance = a2b.dot_product(GetSidewaysDirection(a));
    const bool same_line = fabs(forward_distance) > fabs(sideways_distance);
    const bool within_distance =
        forward_distance > 0 &&
        forward_distance <
            absl::GetFlag(FLAGS_exegesis_pdf_max_character_distance) *
                a.font_size();
    if (same_line && same_orientation && within_distance) {
      return forward_distance;
    }
    return FLT_MAX;
  };

  DenseConnectedComponentsFinder components;
  components.SetNumberOfNodes(all.size());

  // For each character, adds an edge between it and the closest one.
  for (size_t i = 0; i < all.size(); ++i) {
    float min_distance = FLT_MAX;
    size_t candidate_index = 0;
    for (size_t j : all.GetCandidates(i)) {
      const float distance = GetCharacterDistance(i, j);
      if (distance < min_distance) {
        candidate_index = j;
        min_distance = distance;
      }
    }
    if (min_distance < FLT_MAX) {
      components.AddEdge(i, candidate_index);
    }
  }

  // Pushes a set of character indices as a new segment.
  for (auto& indices : GetClusters(&components)) {
    // Returns whether characters[a] is before characters[b].
    const auto reading_order_cmp = [&all](size_t index_a, size_t index_b) {
      const auto& a = all.Get(index_a);
      const auto& b = all.Get(index_b);
      const Vec2F forward = GetForwardDirection(a);
      return GetVector(a, b).dot_product(forward) > 0;
    };
    std::sort(indices.begin(), indices.end(), reading_order_cmp);
    PdfTextSegment segment;
    BoundingBox* bounding_box = segment.mutable_bounding_box();
    bool first = true;
    for (const size_t index : indices) {
      const auto& character = all.Get(index);
      if (first) {
        segment.set_font_size(character.font_size());
        segment.set_orientation(RotateClockwise90(character.orientation()));
        segment.set_fill_color_hash(character.fill_color_hash());
        *bounding_box = character.bounding_box();
        first = false;
      }
      segment.add_character_indices(index);
      segment.mutable_text()->append(character.utf8());
      Union(character.bounding_box(), bounding_box);
    }
    if (!segment.text().empty()) {
      segment.Swap(segments->Add());
    }
  }
}

class Segments {
 public:
  Segments(const PdfPagePreventSegmentBindings& prevent_bindings,
           const PdfTextSegments* segments)
      : segments_(segments) {
    for (size_t i = 0; i < segments->size(); ++i) {
      gtl::InsertOrDie(&first_char_index_to_segment_index_,
                       GetFirstCharIndex(i), i);
      gtl::InsertIfNotPresent(&text_to_index_, segments->Get(i).text(), i);
    }
    for (const auto& prevent_binding : prevent_bindings) {
      const std::string key =
          CreateKey(prevent_binding.first(), prevent_binding.second());
      if (!gtl::InsertIfNotPresent(&prevent_bindings_, key)) {
        LOG(FATAL) << "Duplicated prevent_segment_bindings '" << key
                   << "' in config file";
      }
    }
  }

  ~Segments() {
    if (!prevent_bindings_.empty()) {
      LOG(ERROR) << "The following prevent_segment_bindings were not consumed\n"
                 << absl::StrJoin(prevent_bindings_, "\n");
    }
  }

  size_t size() const { return segments_->size(); }

  const PdfTextSegment& Get(size_t index) const {
    return segments_->Get(index);
  }

  size_t GetFollowingSegment(size_t index) const {
    const size_t last_char_index = GetLastCharIndex(index);
    const size_t following_char_index = last_char_index + 1;
    return ::exegesis::gtl::FindWithDefault(first_char_index_to_segment_index_,
                                            following_char_index, index);
  }

  size_t GetIndexFor(absl::string_view text) const {
    return ::exegesis::gtl::FindWithDefault(text_to_index_, text,
                                            absl::string_view::npos);
  }

  bool ConsumePreventSegmentBinding(const PdfTextSegment& a,
                                    const PdfTextSegment& b) {
    const std::string key = CreateKey(a.text(), b.text());
    const bool prevent = prevent_bindings_.contains(key);
    if (prevent) {
      LOG(INFO) << "Preventing segment binding between '" << key << "'";
      prevent_bindings_.erase(key);
    }
    return prevent;
  }

 private:
  static std::string CreateKey(const std::string& a, const std::string& b) {
    return absl::StrCat(a, " <-> ", b);
  }

  size_t GetFirstCharIndex(size_t index) const {
    return Get(index).character_indices(0);
  }

  size_t GetLastCharIndex(size_t index) const {
    const auto& segment = Get(index);
    CHECK_GT(segment.character_indices_size(), 0);
    return segment.character_indices(segment.character_indices_size() - 1);
  }

  const PdfTextSegments* segments_;
  absl::flat_hash_map<size_t, size_t> first_char_index_to_segment_index_;
  // ok to store absl::string_view we own the data.
  absl::flat_hash_map<absl::string_view, size_t> text_to_index_;
  absl::flat_hash_set<std::string> prevent_bindings_;
};

// Clusters the consecutive segments and link them together into PdfTextBlocks.
// Segments of a paragraph appear next to each other in the document.
//
// 1.-------  4.------ 5.------
// 2.-------
// 3.--
//
// In the example above, the first character of 2. immediately follows the last
// character of 1.
// Same thing for the first character of 3. which immediately follows the last
// character of 2.
// This code clusters segments that form paragraphs - aka 'segments that are
// below each other' taking in consideration the orientation of the text.
void ClusterSegments(Segments* segments, PdfTextBlocks* blocks) {
  const auto is_connected = [segments](const PdfTextSegment& a,
                                       const PdfTextSegment& b) {
    const Orientation sideways = RotateClockwise90(a.orientation());
    const Span h_span_a = GetSpan(a.bounding_box(), sideways);
    const Span h_span_b = GetSpan(b.bounding_box(), sideways);
    const bool same_column = Intersects(h_span_a, h_span_b);
    const bool same_font = a.font_size() == b.font_size();
    const bool same_orientation = a.orientation() == b.orientation();
    const bool same_color = a.fill_color_hash() == b.fill_color_hash();
    const Vec2F forward = GetForwardDirection(a);
    const float distance = GetVector(a, b).dot_product(forward);
    const bool within_distance = distance > 0 && distance < 1.7 * a.font_size();
    const bool prevent_binding = segments->ConsumePreventSegmentBinding(a, b);
    return same_column && same_font && same_orientation && within_distance &&
           same_color && !prevent_binding;
  };

  const size_t segments_size = segments->size();
  DenseConnectedComponentsFinder components;
  components.SetNumberOfNodes(segments_size);

  // Linear algorithm. We only try to connect segments with a contiguous
  // character flow.
  for (size_t i = 0; i < segments_size; ++i) {
    const auto next_segment_index = segments->GetFollowingSegment(i);
    if (next_segment_index == i) continue;
    if (is_connected(segments->Get(i), segments->Get(next_segment_index))) {
      components.AddEdge(i, next_segment_index);
    }
  }

  for (auto& indices : GetClusters(&components)) {
    // Returns whether segments[a] is before segments[b].
    const auto reading_order_cmp = [segments](size_t a_index, size_t b_index) {
      const auto& a = segments->Get(a_index);
      const auto& b = segments->Get(b_index);
      const Vec2F forward = GetForwardDirection(a);
      return GetVector(a, b).dot_product(forward) > 0;
    };
    std::sort(indices.begin(), indices.end(), reading_order_cmp);
    PdfTextBlock block;
    BoundingBox* bounding_box = block.mutable_bounding_box();
    std::string* text = block.mutable_text();
    bool first = true;
    for (const size_t index : indices) {
      const auto& segment = segments->Get(index);
      if (first) {
        block.set_font_size(segment.font_size());
        block.set_orientation(segment.orientation());
        *bounding_box = segment.bounding_box();
        first = false;
      }
      if (!text->empty()) text->push_back('\n');
      text->append(segment.text());
      Union(segment.bounding_box(), bounding_box);
    }
    block.Swap(blocks->Add());
  }
}

class Blocks {
 public:
  explicit Blocks(const PdfTextBlocks* blocks) {
    blocks_.reserve(blocks->size());
    for (const auto& block : *blocks) blocks_.push_back(&block);
  }

  explicit Blocks(std::vector<const PdfTextBlock*> blocks_ptr)
      : blocks_(std::move(blocks_ptr)) {}

  size_t size() const { return blocks_.size(); }

  const PdfTextBlock& Get(size_t index) const { return *blocks_.at(index); }

  Blocks Keep(Indices indices) const {
    std::vector<const PdfTextBlock*> subset;
    subset.reserve(indices.size());
    for (const size_t index : indices) subset.push_back(blocks_.at(index));
    return Blocks(std::move(subset));
  }

 private:
  std::vector<const PdfTextBlock*> blocks_;
};

// Clusters blocks on the same column and merge them in reading order.
// In the following example A and D would be merged into a single block.
// +--------+       +--------+    +-+
// |   A    |       |   B    |    |C|
// +--------+       |        |    | |
// +-----+          |        |    | |
// |  D  |          |        |    +-+
// +-----+          +--------+
void ClusterColumns(const Blocks& row_blocks, PdfTextBlocks* output) {
  const auto is_same_column = [](const PdfTextBlock& a, const PdfTextBlock& b) {
    const Orientation horizontal = Orientation::EAST;
    const Span h_span_a = GetSpan(a.bounding_box(), horizontal);
    const Span h_span_b = GetSpan(b.bounding_box(), horizontal);
    return Intersects(h_span_a, h_span_b);
  };

  const size_t blocks_size = row_blocks.size();
  DenseConnectedComponentsFinder connected_columns;
  connected_columns.SetNumberOfNodes(blocks_size);

  for (size_t i = 0; i < blocks_size; ++i) {
    for (size_t j = i + 1; j < blocks_size; ++j) {
      if (is_same_column(row_blocks.Get(i), row_blocks.Get(j))) {
        connected_columns.AddEdge(i, j);
      }
    }
  }

  for (auto& col_indices : GetClusters(&connected_columns)) {
    const auto top_down_cmp = [&row_blocks](size_t a_index, size_t b_index) {
      const auto& a = row_blocks.Get(a_index).bounding_box();
      const auto& b = row_blocks.Get(b_index).bounding_box();
      return a.top() < b.top();
    };
    std::sort(col_indices.begin(), col_indices.end(), top_down_cmp);
    PdfTextBlock output_block;

    BoundingBox* bounding_box = output_block.mutable_bounding_box();
    std::string* text = output_block.mutable_text();
    bool first = true;
    for (const size_t index : col_indices) {
      const PdfTextBlock& block = row_blocks.Get(index);
      if (first) {
        *bounding_box = block.bounding_box();
        output_block.set_font_size(block.font_size());
        first = false;
      }
      Union(block.bounding_box(), bounding_box);
      if (!text->empty()) text->push_back('\n');
      text->append(block.text());
    }
    // Removing trailing whitespace.
    while (!text->empty() && std::isspace(text->back())) text->pop_back();
    output_block.Swap(output->Add());
  }
}

// Clusters blocks on the same row. A row is a set of blocks which spans
// connects. In the following example A, B, C and D are all on the same row.
// A column clustering pass will merge A and D into a single block. See
// ClusterColumns function above.
// +--------+       +--------+    +-+
// |   A    |       |   B    |    |C|
// +--------+       |        |    | |
// +-----+          |        |    | |
// |  D  |          |        |    +-+
// +-----+          +--------+
void ClusterRows(const Blocks& page_blocks, PdfTextTableRows* rows) {
  const auto SameRow = [](const PdfTextBlock& a, const PdfTextBlock& b) {
    const Orientation vertical = Orientation::SOUTH;
    const Span v_span_a = GetSpan(a.bounding_box(), vertical);
    const Span v_span_b = GetSpan(b.bounding_box(), vertical);
    return v_span_a.ContainsCenterOf(v_span_b) ||
           v_span_b.ContainsCenterOf(v_span_a);
  };

  const size_t blocks_size = page_blocks.size();
  DenseConnectedComponentsFinder connected_rows;
  connected_rows.SetNumberOfNodes(blocks_size);

  // O(N^2) algorithm with low N to find row aligned blocks.
  for (size_t i = 0; i < blocks_size; ++i) {
    for (size_t j = i + 1; j < blocks_size; ++j) {
      if (SameRow(page_blocks.Get(i), page_blocks.Get(j))) {
        connected_rows.AddEdge(i, j);
      }
    }
  }

  for (auto& row_indices : GetClusters(&connected_rows)) {
    const Blocks row_blocks = page_blocks.Keep(row_indices);

    PdfTextTableRow row;
    auto* text_blocks = row.mutable_blocks();
    ClusterColumns(row_blocks, text_blocks);

    const auto left_cmp = [](const PdfTextBlock& a, const PdfTextBlock& b) {
      return a.bounding_box().left() < b.bounding_box().left();
    };
    std::sort(text_blocks->begin(), text_blocks->end(), left_cmp);

    BoundingBox* bounding_box = row.mutable_bounding_box();
    bool first = true;
    for (const PdfTextBlock& block : *text_blocks) {
      if (first) {
        *bounding_box = block.bounding_box();
        first = false;
      }
      Union(block.bounding_box(), bounding_box);
    }
    row.Swap(rows->Add());
  }
}
}  // namespace

void Cluster(PdfPage* page,
             const PdfPagePreventSegmentBindings& prevent_segment_bindings) {
  // First cluster characters into segments.
  const PdfCharacters& page_characters = page->characters();
  const BoundingBox page_bbox = CreateBox(0, 0, page->width(), page->height());
  Characters characters(&page_characters, page_bbox);
  PdfTextSegments* page_segments = page->mutable_segments();
  page_segments->Clear();
  ClusterCharacters(characters, page_segments);

  // Then cluster segments in blocks.
  Segments segments(prevent_segment_bindings, page_segments);
  PdfTextBlocks* page_blocks = page->mutable_blocks();
  page_blocks->Clear();
  ClusterSegments(&segments, page_blocks);

  // Last cluster blocks in rows.
  const Blocks blocks(page_blocks);
  PdfTextTableRows* page_rows = page->mutable_rows();
  page_rows->Clear();
  ClusterRows(blocks, page_rows);

  // Sort rows from top to bottom.
  std::sort(page_rows->begin(), page_rows->end(),
            [](const PdfTextTableRow& a, const PdfTextTableRow& b) {
              return a.bounding_box().top() < b.bounding_box().top();
            });

  // Set row/col numbers.
  for (size_t row_index = 0; row_index < page_rows->size(); ++row_index) {
    auto* row = page_rows->Mutable(row_index);
    for (size_t col_index = 0; col_index < row->blocks_size(); ++col_index) {
      auto* block = row->mutable_blocks(col_index);
      block->set_row(row_index);
      block->set_col(col_index);
    }
  }
}

}  // namespace pdf
}  // namespace exegesis
