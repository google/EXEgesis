// Copyright 2018 Google Inc.
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

#include "exegesis/util/index_type.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace exegesis {
namespace {

template <typename ValueType>
class IndexTypeTest : public ::testing::Test {
 protected:
  DEFINE_INDEX_TYPE(MyIndex, ValueType);
};

using ValueTypes = ::testing::Types<int, int64_t, uint64_t>;
TYPED_TEST_SUITE(IndexTypeTest, ValueTypes);

TYPED_TEST(IndexTypeTest, DefaultConstructor) {
  typename TestFixture::MyIndex index;
  EXPECT_EQ(index.value(), TypeParam(0));
}

TYPED_TEST(IndexTypeTest, FromValue) {
  for (const TypeParam value : {0, 1, 10, 100, 1 << 20}) {
    typename TestFixture::MyIndex index(value);
    EXPECT_EQ(index.value(), value);
  }
}

TYPED_TEST(IndexTypeTest, CopyAndAssignment) {
  using MyIndex = typename TestFixture::MyIndex;
  const TypeParam kValue = 12345;
  const MyIndex a(kValue);
  const MyIndex b(a);
  EXPECT_EQ(b.value(), kValue);
  MyIndex c;
  EXPECT_EQ(c.value(), 0);
  c = a;
  EXPECT_EQ(c.value(), kValue);
}

TYPED_TEST(IndexTypeTest, Comparisons) {
  constexpr int kNumValues = 10;
  for (int value_1 = 0; value_1 < kNumValues; ++value_1) {
    for (int value_2 = 0; value_2 < kNumValues; ++value_2) {
      typename TestFixture::MyIndex index_1(value_1);
      typename TestFixture::MyIndex index_2(value_2);
      if (value_1 == value_2) {
        EXPECT_EQ(index_1, index_2);
        EXPECT_LE(index_1, index_2);
        EXPECT_GE(index_1, index_2);
        EXPECT_FALSE(index_1 < index_2);
        EXPECT_FALSE(index_1 > index_2);
        EXPECT_FALSE(index_1 != index_2);
        EXPECT_EQ(index_1, value_2);
        EXPECT_LE(index_1, value_2);
        EXPECT_GE(index_1, value_2);
        EXPECT_FALSE(index_1 < value_2);
        EXPECT_FALSE(index_1 > value_2);
        EXPECT_FALSE(index_1 != value_2);
      } else if (value_1 < value_2) {
        EXPECT_NE(index_1, index_2);
        EXPECT_LT(index_1, index_2);
        EXPECT_LE(index_1, index_2);
        EXPECT_FALSE(index_1 == index_2);
        EXPECT_FALSE(index_1 >= index_2);
        EXPECT_FALSE(index_1 > index_2);
        EXPECT_NE(index_1, value_2);
        EXPECT_LT(index_1, value_2);
        EXPECT_LE(index_1, value_2);
        EXPECT_FALSE(index_1 == value_2);
        EXPECT_FALSE(index_1 >= value_2);
        EXPECT_FALSE(index_1 > value_2);
      } else if (value_1 > value_2) {
        EXPECT_NE(index_1, index_2);
        EXPECT_GT(index_1, index_2);
        EXPECT_GE(index_1, index_2);
        EXPECT_FALSE(index_1 == index_2);
        EXPECT_FALSE(index_1 <= index_2);
        EXPECT_FALSE(index_1 < index_2);
        EXPECT_NE(index_1, value_2);
        EXPECT_GT(index_1, value_2);
        EXPECT_GE(index_1, value_2);
        EXPECT_FALSE(index_1 == value_2);
        EXPECT_FALSE(index_1 <= value_2);
        EXPECT_FALSE(index_1 < value_2);
      }
    }
  }
}

TYPED_TEST(IndexTypeTest, Increment) {
  using MyIndex = typename TestFixture::MyIndex;
  constexpr TypeParam kNumIterations = 100;
  MyIndex index;
  for (TypeParam i = 0; i < kNumIterations; i += 2) {
    const TypeParam expected_value = i + 1;
    const MyIndex pre_increment = ++index;
    const MyIndex post_increment = index++;
    EXPECT_EQ(pre_increment.value(), expected_value);
    EXPECT_EQ(post_increment.value(), expected_value);
  }
}

TYPED_TEST(IndexTypeTest, Decrement) {
  using MyIndex = typename TestFixture::MyIndex;
  constexpr TypeParam kNumIterations = 100;
  constexpr TypeParam kInitialValue = 2 * kNumIterations + 1;
  MyIndex index(kInitialValue);
  for (TypeParam i = 0; i < kNumIterations; i += 2) {
    const TypeParam expected_value = kInitialValue - i - 1;
    const MyIndex pre_decrement = --index;
    const MyIndex post_decrement = index--;
    EXPECT_EQ(pre_decrement.value(), expected_value);
    EXPECT_EQ(post_decrement.value(), expected_value);
  }
}

#ifdef NO_COMPILE_TEST
// NOTE(ondrasej): The code below is not correctly typed - a value of type
// IndexA may not be assigned to a variable of type IndexB. Uncomment the code
// to test that it breaks the build.
TEST(IndexTypeTest, TwoDifferentIndices) {
  DEFINE_INDEX_TYPE(IndexA, int);
  DEFINE_INDEX_TYPE(IndexB, int);
  const IndexA a(1);
  IndexB b(1);
  b = a;
}
#endif  // NO_COMPILE_TEST

}  // namespace
}  // namespace exegesis
