//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include "iterlib/RocksDBIterator.h"

#include <boost/filesystem.hpp>
#include <folly/Singleton.h>
#include <gtest/gtest.h>
#include <memory>

#include "iterlib/LimitIterator.h"
#include "rocksdb/comparator.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/table.h"

using namespace rocksdb;
using iterlib::Item;
std::string db_path;

class RocksDBIteratorTest : public ::testing::Test {
 public:
  RocksDBIteratorTest() {}

  rocksdb::DB* getDB() { return db_; }

  rocksdb::Status Put(const Slice& k, const Slice& v,
                      WriteOptions wo = WriteOptions()) {
    return getDB()->Put(wo, k, v);
  }

  std::string IterStatus(Iterator* iter);

  void SetUp() override {
    db_path = boost::filesystem::unique_path().string();
    Options options;
    BlockBasedTableOptions tableOptions;
    tableOptions.use_delta_encoding = false;
    options.table_factory.reset(NewBlockBasedTableFactory(tableOptions));
    options.comparator = ReverseBytewiseComparator();

    options.create_if_missing = true;
    options.error_if_exists = true;
    Status status = DB::Open(options, db_path, &db_);
    ASSERT_TRUE(status.ok());
  }

  void TearDown() override {
    delete db_;
    boost::filesystem::remove_all(db_path);
  }

 protected:
  rocksdb::DB* db_;
};

std::string RocksDBIteratorTest::IterStatus(Iterator* iter) {
  std::string result;
  if (iter->Valid()) {
    result = iter->key().ToString() + "->" + iter->value().ToString();
  } else {
    result = "(invalid)";
  }
  return result;
}

#define ASSERT_OK(s) ASSERT_TRUE((s).ok())
#define P(s) folly::StringPiece(s)

TEST_F(RocksDBIteratorTest, Simple) {
  ASSERT_OK(Put("a", "1"));
  ASSERT_OK(Put("b", "2"));
  ASSERT_OK(Put("c", "3"));
  ReadOptions ro;
  ro.pin_data = true;
  auto riter = getDB()->NewIterator(ro);
  riter->SeekToFirst();
  EXPECT_TRUE(riter->Valid());
  auto iter = folly::make_unique<iterlib::RocksDBIterator>(riter);
  iter->prepare();
  EXPECT_TRUE(iter->next());
  std::vector<std::pair<Item, Item>> expected{{
    {Item(P("c")), Item(P("3"))},
    {Item(P("b")), Item(P("2"))},
    {Item(P("a")), Item(P("1"))},
  }};
  // Test that the Slice returned by it->key()
  // remains valid after advancing the iterator
  std::vector<std::pair<std::reference_wrapper<const Item>,
                        std::reference_wrapper<const Item>>> actual;
  actual.emplace_back(iter->key(), iter->value());
  EXPECT_TRUE(iter->next());
  actual.emplace_back(iter->key(), iter->value());
  EXPECT_TRUE(iter->next());
  actual.emplace_back(iter->key(), iter->value());
  EXPECT_FALSE(iter->next());
  int i = 0;
  for (const auto& p : expected) {
    std::pair<Item, Item> actual_pair = std::make_pair(actual[i].first.get(),
                                                       actual[i].second.get());
    EXPECT_EQ(p, actual_pair);
    i++;
  }
}

TEST_F(RocksDBIteratorTest, Composed) {
  ASSERT_OK(Put("a", "1"));
  ASSERT_OK(Put("b", "2"));
  ASSERT_OK(Put("c", "3"));
  ASSERT_OK(Put("d", "4"));
  ASSERT_OK(Put("e", "5"));
  ReadOptions ro;
  ro.pin_data = true;
  auto riter = getDB()->NewIterator(ro);
  riter->SeekToFirst();
  EXPECT_TRUE(riter->Valid());
  auto inner = folly::make_unique<iterlib::RocksDBIterator>(riter);
  auto iter = folly::make_unique<iterlib::LimitIterator>(
      static_cast<iterlib::Iterator*>(inner.release()), 2, 1);
  iter->prepare();
  EXPECT_TRUE(iter->next());
  std::vector<std::pair<Item, Item>> expected{{
    {Item(P("d")), Item(P("4"))},
    {Item(P("c")), Item(P("3"))},
  }};
  // Test that the Slice returned by it->key()
  // remains valid after advancing the iterator
  std::vector<std::pair<std::reference_wrapper<const Item>,
                        std::reference_wrapper<const Item>>> actual;
  actual.emplace_back(iter->key(), iter->value());
  EXPECT_TRUE(iter->next());
  actual.emplace_back(iter->key(), iter->value());
  EXPECT_FALSE(iter->next());
  int i = 0;
  for (const auto& p : expected) {
    std::pair<Item, Item> actual_pair = std::make_pair(actual[i].first.get(),
                                                       actual[i].second.get());
    EXPECT_EQ(p, actual_pair);
    i++;
  }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
