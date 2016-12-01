//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
#include "ExpectIterator.h"

#include "iterlib/FutureIterator.h"
#include "iterlib/LimitIterator.h"
#include "iterlib/RandomIterator.h"
#include "iterlib/ReverseIterator.h"
#include "iterlib/CountIterator.h"

#include "iterlib/AndIterator.h"
#include "iterlib/OrIterator.h"
#include "iterlib/DifferenceIterator.h"

#include "iterlib/LetIterator.h"
#include "iterlib/MergeIterator.h"
#include "iterlib/NestIterator.h"
#include "iterlib/ProjectIterator.h"

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
  const auto expected = std::vector<ItemOptimized>{res[0], res[1]};
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

TEST(IteratorTest, CountIterator) {
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
  auto countIt = folly::make_unique<CountIterator>(it.release());
  countIt->prepare();
  EXPECT_TRUE(countIt->next());
  EXPECT_EQ(CountIterator::kCountKey, countIt->key());
  EXPECT_EQ(4, countIt->value());
  EXPECT_FALSE(countIt->next());
}

// returns [end, start] in descending order
std::unique_ptr<Iterator> getRange(int64_t start, int64_t end) {
  std::vector<ItemOptimized> res;
  res.reserve(end - start);
  for (uint64_t i = end; i <= start; --i) {
    res.emplace_back(ItemOptimized{i, 0, ""});
  }
  return folly::make_unique<FutureIterator<ItemOptimized>>(
    folly::makeFuture(res));
}

// vec is assumed to be in descending order
std::unique_ptr<Iterator> getVector(const std::vector<iterlib::id_t>& vec) {
  std::vector<ItemOptimized> res;
  res.reserve(vec.size());
  for (const auto& v : vec) {
    res.emplace_back(ItemOptimized{v, 0, ""});
  }
  return folly::make_unique<FutureIterator<ItemOptimized>>(
    folly::makeFuture(res));
}

TEST(IteratorTest, UnionIterator) {
  auto it1 = std::move(getRange(1, 10));
  auto it2 = std::move(getRange(11, 20));
  auto it3 = std::move(getRange(1, 20));
  IteratorVector iters;
  iters.emplace_back(std::move(it1));
  iters.emplace_back(std::move(it2));
  auto unionIt = folly::make_unique<UnionIterator>(iters);
  unionIt->prepare();
  it3->prepare();
  while(unionIt->next()) {
    EXPECT_TRUE(it3->next());
    EXPECT_EQ(it3->key(), unionIt->key());
    EXPECT_EQ(it3->value(), unionIt->value());
  }
  EXPECT_FALSE(it3->next());
}

TEST(IteratorTest, ConcatIterator) {
  auto it1 = std::move(getVector({3, 5, 2, 1}));
  auto it2 = std::move(getVector({4, 2, 1}));
  auto it3 = std::move(getVector({3, 5, 2, 1, 4}));
  IteratorVector iters;
  iters.emplace_back(std::move(it1));
  iters.emplace_back(std::move(it2));
  auto concatIt = folly::make_unique<ConcatIterator>(iters);
  concatIt->prepare();
  it3->prepare();
  while(concatIt->next()) {
    EXPECT_TRUE(it3->next());
    EXPECT_EQ(it3->key(), concatIt->key());
    EXPECT_EQ(it3->value(), concatIt->value());
  }
  EXPECT_FALSE(it3->next());
}

TEST(IteratorTest, AndIterator) {
  auto it1 = std::move(getVector({5, 3, 2, 1}));
  auto it2 = std::move(getVector({4, 2, 1}));
  auto it3 = std::move(getVector({2, 1}));
  IteratorVector iters;
  iters.emplace_back(std::move(it1));
  iters.emplace_back(std::move(it2));
  auto andIt = folly::make_unique<AndIterator>(iters);
  andIt->prepare();
  it3->prepare();
  while(andIt->next()) {
    EXPECT_TRUE(it3->next());
    EXPECT_EQ(it3->key(), andIt->key());
    EXPECT_EQ(it3->value(), andIt->value());
  }
  EXPECT_FALSE(it3->next());
}

TEST(IteratorTest, DifferenceIterator) {
  auto it1 = std::move(getVector({5, 3, 2, 1}));
  auto it2 = std::move(getVector({4, 2, 1}));
  auto it3 = std::move(getVector({5, 3}));
  IteratorVector iters;
  iters.emplace_back(std::move(it1));
  iters.emplace_back(std::move(it2));
  auto diffIt = folly::make_unique<DifferenceIterator>(iters);
  diffIt->prepare();
  it3->prepare();
  while(diffIt->next()) {
    EXPECT_TRUE(it3->next());
    EXPECT_EQ(it3->key(), diffIt->key());
    EXPECT_EQ(it3->value(), diffIt->value());
  }
  EXPECT_FALSE(it3->next());
}

TEST(IteratorTest, SortedMergeIterator) {
  auto it1 = std::move(getVector({5, 3, 2, 1}));
  auto it2 = std::move(getVector({4, 2, 1}));
  auto it3 = std::move(getVector({5, 4, 3, 2, 1}));
  IteratorVector iters;
  iters.emplace_back(std::move(it1));
  iters.emplace_back(std::move(it2));
  auto smIt = folly::make_unique<SortedMergeIterator>(iters);
  smIt->prepare();
  it3->prepare();
  while(smIt->next()) {
    EXPECT_TRUE(it3->next());
    EXPECT_EQ(it3->key(), smIt->key());
    EXPECT_EQ(it3->value().id(), smIt->value().id());
  }
  EXPECT_FALSE(it3->next());
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
