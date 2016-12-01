//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
#include <gtest/gtest.h>

#include "ExpectIterator.h"

#include "iterlib/FutureIterator.h"
#include "iterlib/MergeIterator.h"

using namespace iterlib;
using iterlib::variant::unordered_map_t;

TEST(MergeIterator, ints) {
  const std::string countKey = "count";
  const auto res = std::vector<ItemOptimized>{{
      {0, 0, unordered_map_t{{countKey, 10L}}},
      {0, 0, unordered_map_t{{countKey, 20L}}},
      {0, 0, unordered_map_t{{countKey, 30L}}},
  }};
  const auto expected = std::vector<ItemOptimized>{{
      {0, 0, unordered_map_t{{countKey, 60L}}},
  }};

  IteratorVector iters;
  for (const auto& v : res) {
    std::vector<ItemOptimized> vec = { v };
    auto it = folly::make_unique<FutureIterator<ItemOptimized>>(
      folly::makeFuture(vec));
    iters.emplace_back(it.release());
  }
  auto mergedIt = MergeIterator(iters);
  ExpectIterator(&mergedIt, expected);
}

TEST(MergeIterator, maps) {
  const std::string countKey = "count";
  const auto res = std::vector<ItemOptimized>{{
      {0, 0, unordered_map_t{{
        "assoc1", unordered_map_t{{countKey, 10L}}
      }}},
      {0, 0, unordered_map_t{{
        "assoc2", unordered_map_t{{countKey, 20L}}
      }}},
      {0, 0, unordered_map_t{{
        "assoc3", unordered_map_t{{countKey, 30L}}
      }}},
  }};
  const auto expected = std::vector<ItemOptimized>{{
      {0, 0, unordered_map_t{
        {"assoc1", unordered_map_t{{countKey, 10L}}},
        {"assoc2", unordered_map_t{{countKey, 20L}}},
        {"assoc3", unordered_map_t{{countKey, 30L}}}
      }}
  }};

  IteratorVector iters;
  for(const auto& v : res) {
    std::vector<ItemOptimized> vec = { v };
    auto it = folly::make_unique<FutureIterator<ItemOptimized>>(
      folly::makeFuture(vec));
    iters.emplace_back(it.release());
  }
  auto mergedIt = MergeIterator(iters);
  ExpectIterator(&mergedIt, expected);
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
