//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
#include <gtest/gtest.h>

#include "ExpectIterator.h"

#include "iterlib/FutureIterator.h"
#include "iterlib/ProjectIterator.h"

using namespace iterlib;
using iterlib::variant::unordered_map_t;
using iterlib::variant::ordered_map_t;

TEST(ProjectIterator, basic) {
  const auto res = std::vector<ItemOptimized>{{
    {1, 0, unordered_map_t{{"a", 1L}, {"b", 10L}, {"c", 20L}}},
    {2, 0, unordered_map_t{{"a", 2L}, {"b", 11L}, {"c", 21L}}},
  }};
  const auto expected = std::vector<ItemOptimized>{{
    {1, 0, ordered_map_t{{"a", 1L}, {"c", 20L}}},
    {2, 0, ordered_map_t{{"a", 2L}, {"c", 21L}}},
  }};

  auto inner = folly::make_unique<FutureIterator<ItemOptimized>>(
    folly::makeFuture(res));
  auto it = folly::make_unique<ProjectIterator>(
                       inner.release(),
                       AttributeNameVec{{"a"}, {"c"}});
  ExpectIterator(it.get(), expected);
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
