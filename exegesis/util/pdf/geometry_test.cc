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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace pdf {
namespace {

////////////////////////////////////////////////////////////////////////////////
// BoundingBox

TEST(GeometryTest, CreateBox) {
  const BoundingBox box = CreateBox(1.0f, 1.0f, 2.0f, 2.0f);
  ASSERT_EQ(box.left(), 1.0f);
  ASSERT_EQ(box.top(), 1.0f);
  ASSERT_EQ(box.right(), 2.0f);
  ASSERT_EQ(box.bottom(), 2.0f);
}

TEST(GeometryTest, BoundingBox) {
  const BoundingBox box = CreateBox(1.0f, 1.0f, 2.0f, 2.0f);
  ASSERT_EQ(GetWidth(box), 1.0f);
  ASSERT_EQ(GetHeight(box), 1.0f);
}

TEST(GeometryTest, BoundingBoxUnionNoOp) {
  const BoundingBox box = CreateBox(1.0f, 1.0f, 2.0f, 2.0f);
  const BoundingBox no_op = Union(box, box);
  ASSERT_EQ(box.left(), no_op.left());
  ASSERT_EQ(box.top(), no_op.top());
  ASSERT_EQ(box.right(), no_op.right());
  ASSERT_EQ(box.bottom(), no_op.bottom());
}

TEST(GeometryTest, BoundingBoxUnion) {
  const BoundingBox a = CreateBox(1.0f, 1.0f, 2.0f, 2.0f);
  const BoundingBox b = CreateBox(3.0f, 4.0f, 5.0f, 6.0f);
  const BoundingBox u = Union(a, b);
  ASSERT_EQ(u.left(), 1.0f);
  ASSERT_EQ(u.top(), 1.0f);
  ASSERT_EQ(u.right(), 5.0f);
  ASSERT_EQ(u.bottom(), 6.0f);
}

TEST(GeometryTest, BoundingBoxContains) {
  const BoundingBox a = CreateBox(1.0f, 1.0f, 2.0f, 2.0f);
  EXPECT_TRUE(Contains(a, Point(1.5f, 1.5f)));
  // Edges are inclusive.
  EXPECT_TRUE(Contains(a, Point(1.0f, 1.0f)));
  EXPECT_TRUE(Contains(a, Point(1.0f, 2.0f)));
  EXPECT_TRUE(Contains(a, Point(2.0f, 2.0f)));
  EXPECT_TRUE(Contains(a, Point(2.0f, 1.0f)));
  // Outside
  EXPECT_FALSE(Contains(a, Point(0.0f, 0.0f)));
  EXPECT_FALSE(Contains(a, Point(3.0f, 3.0f)));
  EXPECT_FALSE(Contains(a, Point(3.0f, 0.0f)));
}

TEST(GeometryTest, BoundingBoxIntersects) {
  // Intersect with self (bounds are inclusive).
  EXPECT_TRUE(Intersects(CreateBox(1.0f, 1.0f, 2.0f, 2.0f),
                         CreateBox(1.0f, 1.0f, 2.0f, 2.0f)));
  // Boxes share a point.
  EXPECT_TRUE(Intersects(CreateBox(1.0f, 1.0f, 2.0f, 2.0f),
                         CreateBox(2.0f, 2.0f, 3.0f, 3.0f)));
  // One Box contain the other.
  EXPECT_TRUE(Intersects(CreateBox(1.0f, 1.0f, 4.0f, 4.0f),
                         CreateBox(2.0f, 2.0f, 3.0f, 3.0f)));
  EXPECT_TRUE(Intersects(CreateBox(2.0f, 2.0f, 3.0f, 3.0f),
                         CreateBox(1.0f, 1.0f, 4.0f, 4.0f)));
  // Boxes are disjoint.
  EXPECT_FALSE(Intersects(CreateBox(1.0f, 1.0f, 2.0f, 2.0f),
                          CreateBox(3.0f, 3.0f, 4.0f, 4.0f)));
}

TEST(GeometryTest, BoundingContains) {
  // Contains with self (bounds are inclusive).
  EXPECT_TRUE(Contains(CreateBox(1.0f, 1.0f, 2.0f, 2.0f),
                       CreateBox(1.0f, 1.0f, 2.0f, 2.0f)));
  // Boxes share a point.
  EXPECT_FALSE(Contains(CreateBox(1.0f, 1.0f, 2.0f, 2.0f),
                        CreateBox(2.0f, 2.0f, 3.0f, 3.0f)));
  // One Box contain the other.
  EXPECT_TRUE(Contains(CreateBox(1.0f, 1.0f, 4.0f, 4.0f),
                       CreateBox(2.0f, 2.0f, 3.0f, 3.0f)));
  EXPECT_FALSE(Contains(CreateBox(2.0f, 2.0f, 3.0f, 3.0f),
                        CreateBox(1.0f, 1.0f, 4.0f, 4.0f)));
  // Boxes are disjoint.
  EXPECT_FALSE(Contains(CreateBox(1.0f, 1.0f, 2.0f, 2.0f),
                        CreateBox(3.0f, 3.0f, 4.0f, 4.0f)));
}

TEST(GeometryTest, BoundingBoxCenter) {
  const Point center = GetCenter(CreateBox(1.0f, 1.0f, 2.0f, 3.0f));
  EXPECT_EQ(center.x, 1.5f);
  EXPECT_EQ(center.y, 2.0f);
}

////////////////////////////////////////////////////////////////////////////////
// Vec2F

TEST(GeometryTest, Vec2F) {
  const Vec2F vec = Vec2F(3.0f, 4.0f);
  ASSERT_EQ(vec.x, 3.0f);
  ASSERT_EQ(vec.y, 4.0f);
  ASSERT_EQ(vec.norm_square(), 25.0f);
  ASSERT_EQ(vec.dot_product(vec), 25.0f);
}

TEST(GeometryTest, Vec2FSubstract) {
  const Vec2F a = Vec2F(3.0f, 4.0f);
  const Vec2F zero = a - a;
  ASSERT_EQ(zero.x, 0.0f);
  ASSERT_EQ(zero.y, 0.0f);
  const Vec2F b = a - Vec2F(1.0f, 1.0f);
  ASSERT_EQ(b.x, 2.0f);
  ASSERT_EQ(b.y, 3.0f);
}

////////////////////////////////////////////////////////////////////////////////
// Point

TEST(GeometryTest, Point) {
  const Point point = Point(3.0f, 4.0f);
  ASSERT_EQ(point.x, 3.0f);
  ASSERT_EQ(point.y, 4.0f);
}

TEST(GeometryTest, PointSubtract) {
  const Point a = Point(3.0f, 4.0f);
  const Point b = Point(1.0f, 1.0f);
  const Vec2F result = a - b;
  ASSERT_EQ(result.x, 2.0f);
  ASSERT_EQ(result.y, 3.0f);
}

////////////////////////////////////////////////////////////////////////////////
// QuadTree

TEST(GeometryTest, QuadTree) {
  const BoundingBox area = CreateBox(1.0f, 1.0f, 10.0f, 10.0f);
  QuadTree tree(area);
  // tree is currently empty.
  Indices indices;
  tree.QueryRange(area, &indices);
  EXPECT_TRUE(indices.empty());
  // Adding point outside of area yields false and still no points stored.
  EXPECT_FALSE(tree.Insert(0, Point(11.0f, 11.0f)));
  tree.QueryRange(area, &indices);
  EXPECT_TRUE(indices.empty());
  // Adding one point in the surface.
  EXPECT_TRUE(tree.Insert(0, Point(5.0f, 5.0f)));
  tree.QueryRange(area, &indices);
  EXPECT_EQ(indices.size(), 1);
  EXPECT_EQ(indices.front(), 0);
  // Querying an area with no points.
  indices.clear();
  tree.QueryRange(CreateBox(1.0f, 1.0f, 2.0f, 2.0f), &indices);
  EXPECT_TRUE(indices.empty());
  // Querying an area with the point.
  indices.clear();
  tree.QueryRange(CreateBox(5.0f, 5.0f, 5.0f, 5.0f), &indices);
  EXPECT_EQ(indices.size(), 1);
  EXPECT_EQ(indices.front(), 0);
}

TEST(GeometryTest, QuadTreeQuadrant) {
  const BoundingBox area = CreateBox(0.0f, 0.0f, 4.0f, 4.0f);
  QuadTree tree(area);
  const size_t count = (QuadTree::kCapacity / 4) + 1;
  for (size_t i = 0; i < count; ++i) {
    EXPECT_TRUE(tree.Insert(i, Point(1, 1)));  // North East
    EXPECT_TRUE(tree.Insert(i, Point(3, 1)));  // North West
    EXPECT_TRUE(tree.Insert(i, Point(1, 3)));  // South East
    EXPECT_TRUE(tree.Insert(i, Point(3, 3)));  // South West
  }
  ASSERT_TRUE(tree.IsSubdivided());
  {
    // Whole area.
    Indices indices;
    tree.QueryRange(area, &indices);
    EXPECT_EQ(indices.size(), 4 * count);
  }
  {
    // North East
    Indices indices;
    tree.QueryRange(CreateBox(0.0f, 0.0f, 2.0f, 2.0f), &indices);
    EXPECT_EQ(indices.size(), count);
  }
  {
    // North West
    Indices indices;
    tree.QueryRange(CreateBox(2.0f, 0.0f, 4.0f, 2.0f), &indices);
    EXPECT_EQ(indices.size(), count);
  }
  {
    // South East
    Indices indices;
    tree.QueryRange(CreateBox(0.0f, 2.0f, 2.0f, 4.0f), &indices);
    EXPECT_EQ(indices.size(), count);
  }
  {
    // South West
    Indices indices;
    tree.QueryRange(CreateBox(2.0f, 2.0f, 4.0f, 4.0f), &indices);
    EXPECT_EQ(indices.size(), count);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Span

using testing::AllOf;
using testing::Field;
using testing::FloatEq;

TEST(GeometryTest, Span) {
  const Span span(1.0f, 5.0f);
  EXPECT_EQ(span.min, 1.0f);
  EXPECT_EQ(span.max, 5.0f);
}

TEST(GeometryTest, SpanSize) {
  EXPECT_THAT(Span(1.0f, 5.0f).size(), FloatEq(4.0f));
}

TEST(GeometryTest, SpanCenter) {
  EXPECT_THAT(Span(1.0f, 5.0f).center(), FloatEq(3.0f));
}

const auto SpanEq = [](const Span& expected) {
  return AllOf(Field(&Span::min, FloatEq(expected.min)),
               Field(&Span::max, FloatEq(expected.max)));
};

TEST(GeometryTest, SpanUnion) {
  EXPECT_THAT(Union(Span(1.0f, 5.0f), Span(1.0f, 2.0f)),
              SpanEq(Span(1.0f, 5.0f)));
  EXPECT_THAT(Union(Span(1.0f, 5.0f), Span(1.0f, 2.0f)),
              SpanEq(Span(1.0f, 5.0f)));
  EXPECT_THAT(Union(Span(1.0f, 5.0f), Span(2.0f, 2.0f)),
              SpanEq(Span(1.0f, 5.0f)));
  EXPECT_THAT(Union(Span(1.0f, 5.0f), Span(2.0f, 5.0f)),
              SpanEq(Span(1.0f, 5.0f)));
  EXPECT_THAT(Union(Span(1.0f, 5.0f), Span(2.0f, 6.0f)),
              SpanEq(Span(1.0f, 6.0f)));
  EXPECT_THAT(Union(Span(1.0f, 5.0f), Span(6.0f, 7.0f)),
              SpanEq(Span(1.0f, 7.0f)));
}

TEST(GeometryTest, SpanIntersection) {
  EXPECT_THAT(Intersection(Span(1.0f, 5.0f), Span(1.0f, 2.0f)),
              SpanEq(Span(1.0f, 2.0f)));
  EXPECT_THAT(Intersection(Span(1.0f, 5.0f), Span(2.0f, 2.0f)),
              SpanEq(Span(2.0f, 2.0f)));
  EXPECT_THAT(Intersection(Span(1.0f, 5.0f), Span(2.0f, 5.0f)),
              SpanEq(Span(2.0f, 5.0f)));
  EXPECT_THAT(Intersection(Span(1.0f, 5.0f), Span(2.0f, 6.0f)),
              SpanEq(Span(2.0f, 5.0f)));
  EXPECT_THAT(Intersection(Span(1.0f, 5.0f), Span(6.0f, 7.0f)),
              SpanEq(Span(0.0f, 0.0f)));
}

TEST(GeometryTest, SpanOverlapRatio) {
  EXPECT_THAT(OverlapRatio(Span(1.0f, 5.0f), Span(1.0f, 2.0f)),
              FloatEq(1.0f / 4.0f));
  EXPECT_THAT(OverlapRatio(Span(1.0f, 5.0f), Span(2.0f, 2.0f)), FloatEq(0.0f));
  EXPECT_THAT(OverlapRatio(Span(1.0f, 5.0f), Span(2.0f, 5.0f)),
              FloatEq(3.0f / 4.0f));
  EXPECT_THAT(OverlapRatio(Span(1.0f, 5.0f), Span(2.0f, 6.0f)),
              FloatEq(3.0f / 5.0f));
  EXPECT_THAT(OverlapRatio(Span(1.0f, 5.0f), Span(6.0f, 7.0f)), FloatEq(0.0f));
}

TEST(GeometryTest, SpanContains) {
  EXPECT_TRUE(Span(1.0f, 5.0f).Contains(Span(1.0f, 2.0f)));
  EXPECT_TRUE(Span(1.0f, 5.0f).Contains(Span(2.0f, 2.0f)));
  EXPECT_TRUE(Span(1.0f, 5.0f).Contains(Span(2.0f, 5.0f)));
  EXPECT_FALSE(Span(1.0f, 5.0f).Contains(Span(2.0f, 6.0f)));
  EXPECT_FALSE(Span(1.0f, 5.0f).Contains(Span(6.0f, 7.0f)));
}

TEST(GeometryTest, SpanContainsCenterOf) {
  EXPECT_TRUE(Span(1.0f, 5.0f).ContainsCenterOf(Span(1.0f, 2.0f)));
  EXPECT_TRUE(Span(1.0f, 5.0f).ContainsCenterOf(Span(2.0f, 2.0f)));
  EXPECT_TRUE(Span(1.0f, 5.0f).ContainsCenterOf(Span(2.0f, 5.0f)));
  EXPECT_TRUE(Span(1.0f, 5.0f).ContainsCenterOf(Span(2.0f, 6.0f)));
  EXPECT_FALSE(Span(1.0f, 5.0f).ContainsCenterOf(Span(6.0f, 7.0f)));
}

TEST(GeometryTest, SpanIntersects) {
  EXPECT_TRUE(Intersects(Span(1.0f, 5.0f), Span(1.0f, 2.0f)));
  EXPECT_TRUE(Intersects(Span(1.0f, 5.0f), Span(2.0f, 2.0f)));
  EXPECT_TRUE(Intersects(Span(1.0f, 5.0f), Span(2.0f, 5.0f)));
  EXPECT_TRUE(Intersects(Span(1.0f, 5.0f), Span(2.0f, 6.0f)));
  EXPECT_FALSE(Intersects(Span(1.0f, 5.0f), Span(6.0f, 7.0f)));
}

TEST(GeometryTest, GetSpan) {
  const BoundingBox box = CreateBox(1.0f, 2.0f, 3.0f, 4.0f);
  const Span span_h = GetSpan(box, Orientation::EAST);
  EXPECT_EQ(span_h.min, 1.0f);
  EXPECT_EQ(span_h.max, 3.0f);
  const Span span_v = GetSpan(box, Orientation::SOUTH);
  EXPECT_EQ(span_v.min, 2.0f);
  EXPECT_EQ(span_v.max, 4.0f);
}

}  // namespace

}  // namespace pdf
}  // namespace exegesis
