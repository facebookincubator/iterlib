//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
#include "ExpectIterator.h"

#include "iterlib/FutureIterator.h"
#include "iterlib/LimitIterator.h"
#include "iterlib/RandomIterator.h"
#include "iterlib/ReverseIterator.h"

using namespace folly;
using namespace iterlib::variant;
using namespace iterlib;

// (->> (json_literal ..)
//      (limit 2))
TEST(IteratorTest, LimitIterator) {
  const auto res = std::vector<ItemOptimized>{
      {1, 0, unordered_map_t{{"int1", 2L},
                             {"str1", std::string("banana")},
                             {"int2", 3L}}},
      {2, 0, unordered_map_t{{"int1", 1L},
                             {"str1", std::string("orange")},
                             {"int2", 3L}}},
      {3, 0, unordered_map_t{{"int1", 1L},
                             {"str1", std::string("apple")},
                             {"int2", 2L}}},
      {4, 0, unordered_map_t{{"int1", 1L},
                             {"str1", std::string("banana")},
                             {"int2", 4L}}},
  };

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto limitIt = folly::make_unique<LimitIterator>(it.release(), 2, 0);
  const auto expected = std::vector<Item>{res[0], res[1]};
  ExpectIterator(limitIt.get(), expected);
}

TEST(IteratorTest, ReverseIterator) {
  const auto res = std::vector<ItemOptimized>{
      {1, 0, unordered_map_t{{"int1", 2L},
                             {"str1", std::string("banana")},
                             {"int2", 3L}}},
      {2, 0, unordered_map_t{{"int1", 1L},
                             {"str1", std::string("orange")},
                             {"int2", 3L}}},
      {3, 0, unordered_map_t{{"int1", 1L},
                             {"str1", std::string("apple")},
                             {"int2", 2L}}},
      {4, 0, unordered_map_t{{"int1", 1L},
                             {"str1", std::string("banana")},
                             {"int2", 4L}}},
  };

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto reverseIt = folly::make_unique<ReverseIterator>(it.release());
  const auto expected = std::vector<Item>{res[3], res[2], res[1], res[0]};
  ExpectIterator(reverseIt.get(), expected);
}

TEST(IteratorTest, RandomIterator) {
  const auto res = std::vector<ItemOptimized>{
      {1, 0, ordered_map_t{{"int1", 2L},
                             {"str1", std::string("banana")},
                             {"int2", 3L}}},
      {2, 0, ordered_map_t{{"int1", 1L},
                             {"str1", std::string("orange")},
                             {"int2", 3L}}},
      {3, 0, ordered_map_t{{"int1", 1L},
                             {"str1", std::string("apple")},
                             {"int2", 2L}}},
      {4, 0, ordered_map_t{{"int1", 1L},
                             {"str1", std::string("banana")},
                             {"int2", 4L}}},
  };

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto randomIt = folly::make_unique<RandomIterator>(it.release(), 2);
  randomIt->prepare();
  EXPECT_TRUE(randomIt->next());
  EXPECT_TRUE(randomIt->next());
  EXPECT_FALSE(randomIt->next());
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
