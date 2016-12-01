//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
#include <gtest/gtest.h>

#include "ExpectIterator.h"

#include "iterlib/FutureIterator.h"
#include "iterlib/NestIterator.h"

using namespace iterlib;
using iterlib::variant::unordered_map_t;
using iterlib::variant::ordered_map_t;

TEST(NestIterator, basic) {
  const std::string countKey = "count";
  const auto res = std::vector<ItemOptimized>{{
    {0, 0, unordered_map_t{{countKey, 10L}}},
    {0, 0, unordered_map_t{{countKey, 20L}}},
    {0, 0, unordered_map_t{{countKey, 30L}}},
  }};
  const auto expected = std::vector<ItemOptimized>{{
    {0, 0, ordered_map_t{
      {std::string("assoc1"),
       unordered_map_t{{countKey, 10L}}}}},
    {0, 0, ordered_map_t{
      {std::string("assoc2"),
       unordered_map_t{{countKey, 20L}}}}},
    {0, 0, ordered_map_t{
      {std::string("assoc3"),
       unordered_map_t{{countKey, 30L}}}}},
  }};

  IteratorVector iters;
  int i = 1;
  for (const auto& v : res) {
    std::vector<ItemOptimized> vec = { v };
    auto it = folly::make_unique<FutureIterator<ItemOptimized>>(
      folly::makeFuture(vec));
    iters.emplace_back(folly::make_unique<NestIterator>(
                         it.release(),
                         folly::stringPrintf("assoc%d", i++)));
  }

  i = 0;
  for (const auto& it : iters) {
    std::vector<ItemOptimized> v = {expected[i++]};
    ExpectIterator(it.get(), v);
  }
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
