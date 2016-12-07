#include <gtest/gtest.h>

#include "ExpectIterator.h"

#include "iterlib/FilterIterator.h"
#include "iterlib/FutureIterator.h"

using std::unique_ptr;
using namespace iterlib;
using namespace iterlib::variant;

TEST(FilterIteratorTest, FilterOneAttr) {
  const auto res = std::vector<ItemOptimized>{
      {1, 0, unordered_map_t{{"int1", 1L}, {"int2", 2L}}},
      {2, 0, unordered_map_t{{"int1", 1L}, {"int2", 3L}}},
      {3, 0, unordered_map_t{{"int1", 2L}, {"int2", 3L}}},
  };
  const auto filteredResult = std::vector<ItemOptimized>{res[0], res[1]};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto filterIt = folly::make_unique<FilterIterator>(it.release());
  filterIt->setFilter({"int1"}, {"1"}, FilterType::EQ);

  ExpectIterator(filterIt.get(), filteredResult);
}

TEST(FilterIteratorTest, FilterSecondAttr) {
  const auto res = std::vector<ItemOptimized>{
      {1, 0, unordered_map_t{{"int1", 1L},
                             {"int2", 1L},
                             {"string", std::string{"foo"}}}},
      {2, 0, unordered_map_t{{"int1", 1L},
                             {"int2", 1L},
                             {"string", std::string{"bar"}}}},
      {2, 0, unordered_map_t{{"int1", 1L},
                             {"int2", 2L},
                             {"string", std::string{"foo"}}}},
      {2, 0, unordered_map_t{{"int1", 2L},
                             {"int2", 2L},
                             {"string", std::string{"baz"}}}},
  };
  const auto filteredResult = std::vector<ItemOptimized>{res[2], res[3]};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto filterIt = folly::make_unique<FilterIterator>(it.release());
  filterIt->setFilter({"int2"}, {"2"}, FilterType::EQ);

  ExpectIterator(filterIt.get(), filteredResult);
}

// Demonstrates how tuple comparison is not the same as
// AND of two filters. See RowwiseFilter test below.
TEST(FilterIteratorTest, FilterTwoAttrs) {
  const auto res = std::vector<ItemOptimized>{
      {1, 0, unordered_map_t{{"int1", 15L}, {"int2", 6L}}},
      {2, 0, unordered_map_t{{"int1", 10L}, {"int2", 3L}}},
      {2, 0, unordered_map_t{{"int1", 10L}, {"int2", 5L}}},
      {2, 0, unordered_map_t{{"int1", 5L}, {"int2", 4L}}},
  };
  const auto filteredResult = std::vector<ItemOptimized>{res[0]};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto filterIt = folly::make_unique<FilterIterator>(it.release());
  filterIt->setFilter({"int1"}, {"10"}, FilterType::GT);
  auto filterIt2 = folly::make_unique<FilterIterator>(filterIt.release());
  filterIt2->setFilter({"int2"}, {"3"}, FilterType::GT);

  ExpectIterator(filterIt2.get(), filteredResult);
}

TEST(FilterIteratorTest, RowwiseFilter) {
  const auto res = std::vector<ItemOptimized>{
      {1, 0, unordered_map_t{{"int1", 15L}, {"int2", 6L}}},
      {2, 0, unordered_map_t{{"int1", 10L}, {"int2", 3L}}},
      {2, 0, unordered_map_t{{"int1", 10L}, {"int2", 5L}}},
      {2, 0, unordered_map_t{{"int1", 5L}, {"int2", 4L}}},
  };
  const auto filteredResult = std::vector<ItemOptimized>{res[0], res[2]};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto filterIt = folly::make_unique<FilterIterator>(it.release());
  filterIt->setFilter({"int1", "int2"}, std::vector<dynamic>{{10L, 3L}},
                      FilterType::GT);
  ExpectIterator(filterIt.get(), filteredResult);
}

TEST(FilterIteratorTest, FilterSecondStringAttr) {
  const auto res = std::vector<ItemOptimized>{
      {1, 0, unordered_map_t{{"int1", 1L},
                             {"string1", std::string{"a"}},
                             {"string2", std::string{"foo"}}}},
      {2, 0, unordered_map_t{{"int1", 1L},
                             {"string1", std::string{"a"}},
                             {"string2", std::string{"bar"}}}},
      {2, 0, unordered_map_t{{"int1", 1L},
                             {"string1", std::string{"b"}},
                             {"string2", std::string{"foo"}}}},
      {2, 0, unordered_map_t{{"int1", 2L},
                             {"string1", std::string{"b"}},
                             {"string2", std::string{"baz"}}}},
  };
  const auto filteredResult = std::vector<ItemOptimized>{res[0], res[2]};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto filterIt = folly::make_unique<FilterIterator>(it.release());
  filterIt->setFilter({"string2"}, {"foo"}, FilterType::EQ);

  ExpectIterator(filterIt.get(), filteredResult);
}

TEST(FilterByPrefix, EmptyPrefix) {
  const auto res = std::vector<ItemOptimized>{
      {1, 0, unordered_map_t{{"field", std::string{"baz"}}}},
      {2, 0, unordered_map_t{{"field", std::string{"foo"}}}},
      {3, 0, unordered_map_t{{"field", std::string{"bar"}}}},
  };

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto filterIt = folly::make_unique<FilterIterator>(it.release());
  filterIt->setFilter({"field"}, {""}, FilterType::PREFIX);
  ExpectIterator(filterIt.get(), res);
}

TEST(FilterByPrefix, NonEmptyPrefix) {
  const auto res = std::vector<ItemOptimized>{
      {1, 0, unordered_map_t{{"field", std::string{"bar"}}}},
      {2, 0, unordered_map_t{{"field", std::string{"foo"}}}},
      {3, 0, unordered_map_t{{"field", std::string{"foobar"}}}},
  };
  const auto expectedRes = std::vector<ItemOptimized>{res[1], res[2]};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto filterIt = folly::make_unique<FilterIterator>(it.release());
  filterIt->setFilter({"field"}, {"foo"}, FilterType::PREFIX);
  ExpectIterator(filterIt.get(), expectedRes);
}

TEST(FilterContains, Simple) {
  const auto res = std::vector<ItemOptimized>{
      {1, 0, unordered_map_t{{"field", std::string{"bar"}}}},
      {2, 0, unordered_map_t{{"field", std::string{"foo"}}}},
      {3, 0, unordered_map_t{{"field", std::string{"foobar"}}}},
  };
  const auto expectedRes = std::vector<ItemOptimized>{res[1], res[2]};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto filterIt = folly::make_unique<FilterIterator>(it.release());
  filterIt->setFilter({"field"}, {"oo"}, FilterType::CONTAINS);
  ExpectIterator(filterIt.get(), expectedRes);
}

TEST(FilterIteratorTest, InsetOneElement) {
  const auto res = std::vector<ItemOptimized>{
      {1, 0, unordered_map_t{{"field", std::string{"bar"}}}},
      {2, 0, unordered_map_t{{"field", std::string{"foo"}}}},
      {3, 0, unordered_map_t{{"field", std::string{"foobar"}}}},
  };
  const auto expectedRes = std::vector<ItemOptimized>{res[1]};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto filterIt = folly::make_unique<FilterIterator>(it.release());
  filterIt->setFilter({"field"}, {"foo"}, FilterType::INSET);
  ExpectIterator(filterIt.get(), expectedRes);
}

TEST(FilterIteratorTest, InsetManyElements) {
  const auto res = std::vector<ItemOptimized>{
      {1, 0, unordered_map_t{{"field", std::string{"bar"}}}},
      {2, 0, unordered_map_t{{"field", std::string{"foo"}}}},
      {3, 0, unordered_map_t{{"field", std::string{"foobar"}}}},
      {4, 0, unordered_map_t{{"field", std::string{"bar"}}}},
      {5, 0, unordered_map_t{{"field", std::string{"ruby"}}}},
  };
  const auto expectedRes = std::vector<ItemOptimized>{res[0], res[3], res[4]};

  auto it =
      folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(res));
  auto filterIt = folly::make_unique<FilterIterator>(it.release());
  filterIt->setFilter({"field"}, {"bar", "ruby"}, FilterType::INSET);
  ExpectIterator(filterIt.get(), expectedRes);
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
