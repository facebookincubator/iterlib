#include <gtest/gtest.h>

#include "ExpectIterator.h"

#include "iterlib/FutureIterator.h"
#include "iterlib/GroupByIterator.h"

using namespace folly;
using namespace iterlib::variant;
using namespace iterlib;

void ExpectGroupByIterator(
    GroupByIterator it, const Item& groupedByAttributes,
    const std::vector<std::vector<const Item*>> expected) {
  EXPECT_FALSE(it.prepared()) << "Expected not prepared iterator";
  EXPECT_FALSE(it.prepare()
                   .waitVia(folly::EventBaseManager::get()->getEventBase())
                   .getTry()
                   .hasException());
  EXPECT_TRUE(it.prepared()) << "Iterator should be prepared by now";

  for (const auto& v : expected) {
    EXPECT_TRUE(it.next()) << "Missing result from iterator";
    // TODO: Check for the attribute names being grouped by.
    // Possible implementation: key is vector_pair_t instead of
    // vector_dynamic_t
    for (size_t i = 0; i < v.size(); i++) {
      EXPECT_EQ(*(it.valueGroup()[i]), *v[i]) << "Incorrect result at index: "
                                              << i;
    }
  }
  EXPECT_FALSE(it.next()) << "Iterator returned more results than expected";
}

TEST(GroupByIterator, ExceptionOnInvalidAttribute) {
  const auto res = std::vector<ItemOptimized>{{
      {1, 0, ordered_map_t{{"int1", 1L}}},
  }};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto groupByIt = GroupByIterator(it.release(), {"doesnt_exist"});

  groupByIt.prepare().waitVia(folly::EventBaseManager::get()->getEventBase());
  EXPECT_THROW(groupByIt.next(), std::out_of_range)
      << "Grouping by non existing field should throw";
}

TEST(GroupByIterator, GroupByOneAttr) {
  const auto res = std::vector<ItemOptimized>{{
      {1, 0, ordered_map_t{{"int1", 1L}, {"int2", 2L}}},
      {2, 0, ordered_map_t{{"int1", 1L}, {"int2", 3L}}},
      {3, 0, ordered_map_t{{"int1", 2L}, {"int2", 3L}}},
  }};
  const auto groupedResult =
      std::vector<std::vector<const Item*>>{{{&res[0], &res[1]}, {&res[2]}}};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto groupByIt = GroupByIterator{it.release(), {"int1"}};

  ExpectGroupByIterator(std::move(groupByIt), Item{vector_dynamic_t{{"int1"}}},
                        groupedResult);
}

TEST(GroupByIterator, GroupByTwoAttr) {
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
  const auto groupedResult = std::vector<std::vector<const Item*>>{
      {{&res[0]}, {&res[1]}}, {&res[2]}, {&res[3]}};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto groupByIt = GroupByIterator{it.release(), {"int1", "int2"}};

  ExpectGroupByIterator(std::move(groupByIt),
                        Item{vector_dynamic_t{{"int1", "int2"}}},
                        groupedResult);
}

TEST(GroupByIterator, GroupByTwoStringAttr) {
  const auto res = std::vector<ItemOptimized>{{
      {1, 0, ordered_map_t{{"int1", 1L},
                           {"string1", std::string{"a"}},
                           {"string2", std::string{"foo"}}}},
      {2, 0, ordered_map_t{{"int1", 1L},
                           {"string1", std::string{"a"}},
                           {"string2", std::string{"bar"}}}},
      {2, 0, ordered_map_t{{"int1", 1L},
                           {"string1", std::string{"b"}},
                           {"string2", std::string{"foo"}}}},
      {2, 0, ordered_map_t{{"int1", 2L},
                           {"string1", std::string{"b"}},
                           {"string2", std::string{"baz"}}}},
  }};
  const auto groupedResult = std::vector<std::vector<const Item*>>{
      {{&res[0]}, {&res[1]}}, {&res[2]}, {&res[3]}};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto groupByIt = GroupByIterator{it.release(), {"int1", "string1"}};

  ExpectGroupByIterator(std::move(groupByIt),
                        Item{vector_dynamic_t{{"int1", "string1"}}},
                        groupedResult);
}

// Test asserting that while partitioning results we maintain stable order, i.e.
// if A was before B in the input and end up in the same group, then A will be
// before B after partitioning.
TEST(GroupByIterator, StableOrder) {
  const auto res = std::vector<ItemOptimized>{{
      {1, 0, ordered_map_t{{"int1", 1L},
                           {"int2", 3L},
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
  const auto groupedResult = std::vector<std::vector<const Item*>>{
      {{&res[0]}, {&res[1]}, {&res[2]}}, {&res[3]}};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto groupByIt = GroupByIterator{it.release(), {"int1"}};

  ExpectGroupByIterator(std::move(groupByIt), Item{vector_dynamic_t{{"int1"}}},
                        groupedResult);
}

TEST(GroupByIterator, AttrOrderOnKey) {
  const auto res = std::vector<ItemOptimized>{{
      {2, 0, ordered_map_t{{"int1", 2L},
                           {"int2", 2L},
                           {"string", std::string{"foo"}}}},
      {1, 0, ordered_map_t{{"int1", 1L},
                           {"int2", 3L},
                           {"string", std::string{"foo"}}}},
      {2, 0, ordered_map_t{{"int1", 3L},
                           {"int2", 2L},
                           {"string", std::string{"baz"}}}},
  }};
  // Results should be ordered by values of attributes we group by
  const auto groupedResult =
      std::vector<std::vector<const Item*>>{{&res[1]}, {&res[0]}, {&res[2]}};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto groupByIt = GroupByIterator{it.release(), {"int1"}};

  ExpectGroupByIterator(std::move(groupByIt), Item{vector_dynamic_t{{"int1"}}},
                        groupedResult);
}

TEST(GroupByIterator, MultiAttrOrderOnKey) {
  const auto res = std::vector<ItemOptimized>{{
      {2, 0, ordered_map_t{{"int1", 1L},
                           {"int2", 2L},
                           {"string", std::string{"baz"}}}},
      {2, 0, ordered_map_t{{"int1", 2L},
                           {"int2", 1L},
                           {"string", std::string{"foo"}}}},
      {2, 0, ordered_map_t{{"int1", 1L},
                           {"int2", 1L},
                           {"string", std::string{"baz"}}}},
      {1, 0, ordered_map_t{{"int1", 2L},
                           {"int2", 2L},
                           {"string", std::string{"foo"}}}},
  }};
  // Results should be ordered by values of attributes we group by
  const auto groupedResult = std::vector<std::vector<const Item*>>{
      {&res[2]}, {&res[0]}, {&res[1]}, {&res[3]}};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto groupByIt = GroupByIterator{it.release(), {"int1", "int2"}};

  ExpectGroupByIterator(std::move(groupByIt),
                        Item{vector_dynamic_t{{"int1", "int2"}}},
                        groupedResult);
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
