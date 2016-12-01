#include <gtest/gtest.h>

#include "ExpectIterator.h"

#include "iterlib/FutureIterator.h"
#include "iterlib/LetIterator.h"

using namespace iterlib;
using iterlib::variant::unordered_map_t;
using iterlib::variant::ordered_map_t;

TEST(LetIterator, basic) {
  const auto res = std::vector<ItemOptimized>{{
    {1, 0, unordered_map_t{{"a", 1L}, {"b", 10L}}},
    {2, 0, unordered_map_t{{"a", 2L}, {"b", 11L}}},
  }};
  const auto expected = std::vector<ItemOptimized>{{
    {1, 0, ordered_map_t{{"c", 1L}, {"b", 10L}}},
    {2, 0, ordered_map_t{{"c", 2L}, {"b", 11L}}},
  }};

  auto inner = folly::make_unique<FutureIterator<ItemOptimized>>(
    folly::makeFuture(res));
  auto it = folly::make_unique<LetIterator>(
                       inner.release(),
                       "c",
                       "a");
  ExpectIterator(it.get(), expected);
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
