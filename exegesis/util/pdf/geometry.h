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

// Simple geometry header with PODs.
#ifndef EXEGESIS_UTIL_PDF_GEOMETRY_H_
#define EXEGESIS_UTIL_PDF_GEOMETRY_H_

#include <memory>
#include <vector>

#include "exegesis/proto/pdf/pdf_document.pb.h"

namespace exegesis {
namespace pdf {

typedef std::vector<size_t> Indices;

////////////////////////////////////////////////////////////////////////////////
// A simple 2D vector with float coordinates.
struct Vec2F {
  Vec2F(float x, float y) : x(x), y(y) {}
  Vec2F operator-(const Vec2F& o) const { return {x - o.x, y - o.y}; }
  Vec2F operator*(float ratio) const { return {x * ratio, y * ratio}; }
  float dot_product(const Vec2F& o) const { return x * o.x + y * o.y; }
  float norm_square() const { return x * x + y * y; }

  float x = 0.0f;
  float y = 0.0f;
};

////////////////////////////////////////////////////////////////////////////////
// A simple 2D Point with float coordinates.
struct Point {
  Point(float x, float y) : x(x), y(y) {}
  Vec2F operator-(const Point& o) const { return {x - o.x, y - o.y}; }

  float x = 0.0f;
  float y = 0.0f;
};

////////////////////////////////////////////////////////////////////////////////

// Creates a BoundingBox from left, right, top, down and check that left <=
// right and top <= bottom.
BoundingBox CreateBox(float left, float top, float right, float bottom);
BoundingBox CreateBox(const Point& center, float width, float height);

// Returns the width of a BoundingBox.
float GetWidth(const BoundingBox&);

// Returns the height of a BoundingBox.
float GetHeight(const BoundingBox&);

// Get the center point of a BoundingBox.
Point GetCenter(const BoundingBox& bbox);

// Returns whether a BoundingBox contains a Point.
// BoundingBox edges are inclusive.
bool Contains(const BoundingBox& bounding_box, const Point& point);

// Returns whether a BoundingBox contains another BoundingBox (i.e. contains all
// four corners).
// BoundingBox edges are inclusive.
bool Contains(const BoundingBox& container, const BoundingBox& inside);

// Returns whether two BoundingBoxes intersects.
// If a and b share an edge, they intersect.
bool Intersects(const BoundingBox& a, const BoundingBox& b);

// Return the Union of two BoundingBoxes.
BoundingBox Union(const BoundingBox& a, const BoundingBox& b);

////////////////////////////////////////////////////////////////////////////////
// A QuadTree to accelerate nearest neighbors search.
class QuadTree {
 public:
  explicit QuadTree(const BoundingBox& bounding_box)
      : bounding_box_(bounding_box) {}

  // Adds the point with a particular index and position.
  bool Insert(size_t point_index, const Point& point_position);

  // Gathers points in the range bounding box into output.
  void QueryRange(const BoundingBox& range, Indices* output) const;

  // Returns whether this node is subdivided.
  bool IsSubdivided() const;

  // The maximum number of points per BoundingBox region. If more
  // points are added the region is subdivided.
  static constexpr const size_t kCapacity = 16;

 private:
  void Subdivide();

  struct PointData {
    PointData(Point position, size_t index)
        : position(position), index(index) {}
    Point position;
    size_t index;
  };

  const BoundingBox bounding_box_;
  std::unique_ptr<QuadTree> quadrant_ne_;  // Subtree for North East quadrant
  std::unique_ptr<QuadTree> quadrant_nw_;  // Subtree for North West quadrant
  std::unique_ptr<QuadTree> quadrant_se_;  // Subtree for South East quadrant
  std::unique_ptr<QuadTree> quadrant_sw_;  // Subtree for South West quadrant
  std::vector<PointData> points_;          // points stored in this node.
};

////////////////////////////////////////////////////////////////////////////////
// An interval between min and max (inclusive) and associated set logic.
//
//  +----------+
// min        max
struct Span {
  // min should be less or equal to max.
  Span(float min, float max);

  // Returns (max - min).
  float size() const;

  // Returns (max + min) / 2.
  float center() const;

  // In the following example, Span A contains Span B.
  // +----------+ span a
  //    +--+      span b
  bool Contains(const Span& other) const;

  // In the following example, Span A contains the center of Span B.
  // +----------+   span a
  //    +----|----+ span b
  bool ContainsCenterOf(const Span& other) const;

  float min = 0.0f;
  float max = 0.0f;
};

// In the following example
// +----------+      span a
//              +--+ span b
// +---------------+ Union
Span Union(const Span& a, const Span& b);

// In the following example
// +----------+   span a
//          +---+ span b
//          +-+   Intersection
Span Intersection(const Span& a, const Span& b);

// In the following example, Span A does not intersect with Span B.
// +----------+       span a
//              +--+  span b
bool Intersects(const Span& a, const Span& b);

// Returns the ratio of the intersection span over the union span.
// +----------+ span a
//    +--+      span b
// Here OverlapRatio = 2 / 10
//
// +------+      span a
//          +--+ span b
// Here OverlapRatio = 0
//
// +------+ span a
// +------+ span b
// Here OverlapRatio = 1
float OverlapRatio(const Span& a, const Span& b);

// Returns the Span of a BoundingBox along a specific orientation.
// +  +-----+  +
// |  |     |  |
// v  |     |  |
//    +-----+  +
Span GetSpan(const BoundingBox& box, const Orientation orientation);

// Returns the direction vector for a particular orientation.
Vec2F GetDirectionVector(Orientation orientation);

// Returns orientation rotated by 90 degrees clockwise.
Orientation RotateClockwise90(Orientation orientation);

}  // namespace pdf
}  // namespace exegesis

#endif  // EXEGESIS_UTIL_PDF_GEOMETRY_H_
