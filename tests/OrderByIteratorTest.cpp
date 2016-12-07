#include <gtest/gtest.h>

#include "ExpectIterator.h"

#include "iterlib/FutureIterator.h"
#include "iterlib/OrderByIterator.h"

using namespace folly;
using namespace iterlib::variant;
using namespace iterlib;

TEST(OrderByIterator, OrderByOneAttr) {
  const auto res = std::vector<ItemOptimized>{{
      {1, 0, ordered_map_t{{"int1", 1L}, {"int2", 2L}}},
      {2, 0, ordered_map_t{{"int1", 1L}, {"int2", 3L}}},
      {3, 0, ordered_map_t{{"int1", 2L}, {"int2", 4L}}},
      {3, 0, ordered_map_t{{"int1", 2L}, {"int2", 3L}}},
  }};
  const auto orderedResult =
      std::vector<ItemOptimized>{{res[2], res[3], res[0], res[1]}};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto orderByIt = folly::make_unique<OrderByIterator>(
      it.release(), AttributeNameVec{{"int1"}});

  ExpectIterator(orderByIt.get(), orderedResult);
}

TEST(OrderByIterator, OrderByTwoAttr) {
  const auto res = std::vector<ItemOptimized>{{
      {1, 0, ordered_map_t{{"int1", 1L},
                           {"int2", 1L},
                           {"string", std::string{"foo"}}}},
      {2, 0, ordered_map_t{{"int1", 1L},
                           {"int2", 1L},
                           {"string", std::string{"bar"}}}},
      {2, 0, ordered_map_t{{"int1", 1L},
                           {"int2", 2L},
                           {"string", std::string{"foo"}}}},
      {2, 0, ordered_map_t{{"int1", 2L},
                           {"int2", 2L},
                           {"string", std::string{"baz"}}}},
  }};
  const auto orderedResult =
      std::vector<ItemOptimized>{{res[3], res[2], res[0], res[1]}};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto orderByIt = folly::make_unique<OrderByIterator>(
      it.release(), AttributeNameVec{{{"int1"}, {"int2"}}});

  ExpectIterator(orderByIt.get(), orderedResult);
}

TEST(OrderByIterator, OrderByTwoAttrMixed) {
  const auto res = std::vector<ItemOptimized>{{
      {1, 0, ordered_map_t{{"int1", 1L},
                           {"int2", 1L},
                           {"string", std::string{"foo"}}}},
      {2, 0, ordered_map_t{{"int1", 1L},
                           {"int2", 1L},
                           {"string", std::string{"bar"}}}},
      {2, 0, ordered_map_t{{"int1", 1L},
                           {"int2", 2L},
                           {"string", std::string{"foo"}}}},
      {2, 0, ordered_map_t{{"int1", 2L},
                           {"int2", 2L},
                           {"string", std::string{"baz"}}}},
  }};
  const auto orderedResult =
      std::vector<ItemOptimized>{{res[2], res[0], res[1], res[3]}};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto orderByIt = folly::make_unique<OrderByIterator>(
      it.release(), AttributeNameVec{{{"int1"}, {"int2"}}},
      // int1 ascending, int2 descending
      std::vector<bool>{{false, true}});

  ExpectIterator(orderByIt.get(), orderedResult);
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
