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

#include "exegesis/util/pdf/geometry.h"

#include <cfloat>

#include "glog/logging.h"

namespace exegesis {
namespace pdf {

BoundingBox CreateBox(float left, float top, float right, float bottom) {
  BoundingBox bbox;
  bbox.set_left(left);
  bbox.set_top(top);
  bbox.set_right(right);
  bbox.set_bottom(bottom);
  CHECK_GE(bbox.right(), bbox.left());
  CHECK_GE(bbox.bottom(), bbox.top());
  return bbox;
}

BoundingBox CreateBox(const Point& center, float width, float height) {
  const float half_width = width * .5f;
  const float half_height = height * .5f;
  return CreateBox(center.x - half_width, center.y - half_height,
                   center.x + half_width, center.y + half_height);
}

float GetWidth(const BoundingBox& bbox) { return bbox.right() - bbox.left(); }

float GetHeight(const BoundingBox& bbox) { return bbox.bottom() - bbox.top(); }

Point GetCenter(const BoundingBox& bbox) {
  return {(bbox.left() + bbox.right()) / 2.0f,
          (bbox.top() + bbox.bottom()) / 2.0f};
}

bool Contains(const BoundingBox& bounding_box, const Point& point) {
  return point.x >= bounding_box.left() && point.x <= bounding_box.right() &&
         point.y >= bounding_box.top() && point.y <= bounding_box.bottom();
}

bool Contains(const BoundingBox& container, const BoundingBox& inside) {
  return container.left() <= inside.left() && container.top() <= inside.top() &&
         container.right() >= inside.right() &&
         container.bottom() >= inside.bottom();
}

bool Intersects(const BoundingBox& a, const BoundingBox& b) {
  if (a.right() < b.left()) return false;  // a is left of b
  if (a.left() > b.right()) return false;  // a is right of b
  if (a.bottom() < b.top()) return false;  // a is above b
  if (a.top() > b.bottom()) return false;  // a is below b
  return true;
}

BoundingBox Union(const BoundingBox& a, const BoundingBox& b) {
  return CreateBox(std::min(a.left(), b.left()), std::min(a.top(), b.top()),
                   std::max(a.right(), b.right()),
                   std::max(a.bottom(), b.bottom()));
}

////////////////////////////////////////////////////////////////////////////////

bool QuadTree::Insert(size_t index, const Point& position) {
  if (!Contains(bounding_box_, position)) return false;
  if (points_.size() < kCapacity) {
    points_.emplace_back(position, index);
    return true;
  }
  if (!IsSubdivided()) Subdivide();
  if (quadrant_ne_->Insert(index, position)) return true;
  if (quadrant_nw_->Insert(index, position)) return true;
  if (quadrant_se_->Insert(index, position)) return true;
  if (quadrant_sw_->Insert(index, position)) return true;
  LOG(FATAL);
  return false;
}

void QuadTree::QueryRange(const BoundingBox& bounding_box,
                          Indices* output) const {
  if (!Intersects(bounding_box_, bounding_box)) return;
  for (const auto& point_data : points_) {
    if (Contains(bounding_box, point_data.position)) {
      output->push_back(point_data.index);
    }
  }
  if (IsSubdivided()) {
    quadrant_ne_->QueryRange(bounding_box, output);
    quadrant_nw_->QueryRange(bounding_box, output);
    quadrant_se_->QueryRange(bounding_box, output);
    quadrant_sw_->QueryRange(bounding_box, output);
  }
}

bool QuadTree::IsSubdivided() const {
  return quadrant_ne_ && quadrant_nw_ && quadrant_se_ && quadrant_sw_;
}

void QuadTree::Subdivide() {
  CHECK(!IsSubdivided());
  const Point center = GetCenter(bounding_box_);
  quadrant_ne_.reset(new QuadTree(CreateBox(
      bounding_box_.left(), bounding_box_.top(), center.x, center.y)));
  quadrant_nw_.reset(new QuadTree(CreateBox(center.x, bounding_box_.top(),
                                            bounding_box_.right(), center.y)));
  quadrant_se_.reset(new QuadTree(CreateBox(bounding_box_.left(), center.y,
                                            center.x, bounding_box_.bottom())));
  quadrant_sw_.reset(new QuadTree(CreateBox(
      center.x, center.y, bounding_box_.right(), bounding_box_.bottom())));
}

////////////////////////////////////////////////////////////////////////////////

Span::Span(float min, float max) : min(min), max(max) { CHECK_LE(min, max); }

float Span::size() const { return max - min; }

float Span::center() const { return (max + min) / 2.0f; }

bool Span::Contains(const Span& other) const {
  return min <= other.min && other.max <= max;
}

bool Span::ContainsCenterOf(const Span& other) const {
  const float center = other.center();
  return min <= center && center <= max;
}

Span Union(const Span& a, const Span& b) {
  return Span(std::min(a.min, b.min), std::max(a.max, b.max));
}

Span Intersection(const Span& a, const Span& b) {
  const auto lower = std::max(a.min, b.min);
  const auto upper = std::min(a.max, b.max);
  if (upper < lower) return Span(0.0f, 0.0f);
  return Span(lower, upper);
}

bool Intersects(const Span& a, const Span& b) {
  return a.max >= b.min && a.min <= b.max;
}

float OverlapRatio(const Span& a, const Span& b) {
  const auto intersection_size = Intersection(a, b).size();
  const auto union_size = Union(a, b).size();
  return union_size == 0.0f ? 0.0f : intersection_size / union_size;
}

Span GetSpan(const BoundingBox& box, const Orientation orientation) {
  const Vec2F direction = GetDirectionVector(orientation);
  CHECK_EQ(direction.norm_square(), 1);
  float max = -FLT_MAX;
  float min = FLT_MAX;
  for (const Vec2F& corner :
       {Vec2F(box.left(), box.top()), Vec2F(box.right(), box.top()),
        Vec2F(box.left(), box.bottom()), Vec2F(box.right(), box.bottom())}) {
    const float distance = corner.dot_product(direction);
    if (distance < min) min = distance;
    if (distance > max) max = distance;
  }
  return Span(min, max);
}

Vec2F GetDirectionVector(Orientation orientation) {
  switch (orientation) {
    case NORTH:
      return Vec2F(0, -1);
    case EAST:
      return Vec2F(1, 0);
    case SOUTH:
      return Vec2F(0, 1);
    case WEST:
      return Vec2F(-1, 0);
    case Orientation_INT_MIN_SENTINEL_DO_NOT_USE_:
    case Orientation_INT_MAX_SENTINEL_DO_NOT_USE_:
      break;
  }
  LOG(FATAL);
  return Vec2F(0, 0);
}

Orientation RotateClockwise90(Orientation orientation) {
  switch (orientation) {
    case NORTH:
      return EAST;
    case EAST:
      return SOUTH;
    case SOUTH:
      return WEST;
    case WEST:
      return NORTH;
    case Orientation_INT_MIN_SENTINEL_DO_NOT_USE_:
    case Orientation_INT_MAX_SENTINEL_DO_NOT_USE_:
      break;
  }
  LOG(FATAL);
  return NORTH;
}
}  // namespace pdf
}  // namespace exegesis
