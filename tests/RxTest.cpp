//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
#include "rxcpp/rx.hpp"

#include <folly/Benchmark.h>
#include <gtest/gtest.h>
#include <iterlib/Iterator.h>
#include <iterlib/LimitIterator.h>
#include <iterlib/FutureIterator.h>
#include <range/v3/core.hpp>
#include <range/v3/view/slice.hpp>
#include <vector>

using namespace iterlib;
using namespace iterlib::variant;

TEST(RxTest, Take) {
  auto values = rxcpp::observable<>::range(1, 7).skip(3);
  std::vector<int> expected;
  values.subscribe([&](int v) { expected.push_back(v); },
                   []() { printf("OnCompleted\n"); });
  int i = 4;
  for (const auto v : expected) {
    EXPECT_EQ(i++, v);
  }
}

TEST(RxTest, DynamicTake) {
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

  std::vector<ItemOptimized> expected;
  auto values = rxcpp::observable<>::iterate(res).skip(2);
  values.subscribe([&](const auto& v) { expected.push_back(v); },
                   []() { printf("OnCompleted\n"); });

  EXPECT_EQ(2, expected.size());
  EXPECT_EQ(res[2], expected[0]);
  EXPECT_EQ(res[3], expected[1]);
}

void SkipTakeRx(int iters, int size) {
  std::vector<ItemOptimized> vec;
  folly::BenchmarkSuspender suspend;
  std::vector<Item> expected;

  vec.reserve(size);
  expected.reserve(size/4);
  for (int i = 0; i < size; i++) {
    vec.push_back(ItemOptimized(i,0));
  }
  suspend.dismiss();
  for (int i = 0; i < iters; i++) {
    expected.clear();
    auto values = rxcpp::observable<>::iterate(vec).skip(size/2).take(size/4);
    values.subscribe([&](const auto& v) { expected.push_back(v); });
  }
}
BENCHMARK_PARAM(SkipTakeRx, 1000);
BENCHMARK_PARAM(SkipTakeRx, 10000);

void SkipTakeIterlib(int iters, int size) {
  std::vector<ItemOptimized> vec;
  folly::BenchmarkSuspender suspend;
  std::vector<Item> expected;

  vec.reserve(size);
  expected.reserve(size/4);
  for (int i = 0; i < size; i++) {
    vec.push_back(ItemOptimized(i,0));
  }
  suspend.dismiss();
  for (int i = 0; i < iters; i++) {
    expected.clear();
    auto it =
        folly::make_unique<FutureIterator<ItemOptimized>>(folly::makeFuture(vec));
    auto limitIt = folly::make_unique<LimitIterator>(it.release(), size/4, size/2);
    limitIt->prepare();
    while (limitIt->next()) {
      expected.push_back(limitIt->value());
    }
  }
}
BENCHMARK_PARAM(SkipTakeIterlib, 1000);
BENCHMARK_PARAM(SkipTakeIterlib, 10000);

void SkipTakeRangeV3(int iters, int size) {
  std::vector<ItemOptimized> vec;
  folly::BenchmarkSuspender suspend;
  std::vector<Item> expected;
  using namespace ranges;

  vec.reserve(size);
  expected.reserve(size/4);
  for (int i = 0; i < size; i++) {
    vec.push_back(ItemOptimized(i,0));
  }
  suspend.dismiss();
  for (int i = 0; i < iters; i++) {
    expected.clear();
    auto sliced = vec | view::slice(size/2, size/2 + size/4);
    for (const auto& i : sliced) {
      expected.push_back(i);
    }
  }
}
BENCHMARK_PARAM(SkipTakeRangeV3, 1000);
BENCHMARK_PARAM(SkipTakeRangeV3, 10000);

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  folly::runBenchmarks();
  return RUN_ALL_TESTS();
}
