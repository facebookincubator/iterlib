#pragma once

#include <folly/io/async/EventBaseManager.h>
#include <gtest/gtest.h>

#include "iterlib/Iterator.h"

namespace iterlib {

template <typename T>
void ExpectIterator(Iterator* it, const std::vector<T>& expected) {
  EXPECT_FALSE(it->prepared()) << "Expected not prepared iterator";
  EXPECT_FALSE(it->prepare()
                   .waitVia(folly::EventBaseManager::get()->getEventBase())
                   .getTry()
                   .hasException());
  EXPECT_TRUE(it->prepared()) << "Iterator should be prepared by now";

  for (const auto& v : expected) {
    ASSERT_TRUE(it->next()) << "Missing result from iterator";
    const T& value = static_cast<const T&>(it->value());
    EXPECT_EQ(v, value);
  }
  EXPECT_FALSE(it->next()) << "Iterator returned more results than expected";
}

} // namespace iterlib
