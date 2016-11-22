//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
#pragma once

#include "iterlib/Iterator.h"

namespace iterlib {

/**
 * Provides Iterator interface for arbitrary future which returns
 * std::vector<Item> result.
 * Particularly useful for data source APIs which do not have iterator interface
 * and in tests.
 */
template <typename T = Item> class FutureIterator : public Iterator {
 public:
  explicit FutureIterator(folly::Future<std::vector<T>>&& f,
                          Item key = Item::kEmptyItem)
      : Iterator(IteratorType::FUTURE), idx_(0), fResult_(std::move(f)) {
    key_ = key;
  }

  folly::Future<folly::Unit> prepare() override final {
    if (prepared_) {
      return folly::makeFuture();
    }

    return fResult_
        .then([this](const std::vector<T>& res) {
          result_ = res;
          prepared_ = true;
        })
        .onError([](const std::exception& ex) {
          LOG(ERROR) << "Failed to prepare FutureIterator: " << ex.what();
          throw ex;
        });
  }

  virtual const Item& value() const override {
    if (idx_ != 0) {
      return result_[idx_ - 1];
    }
    return Item::kEmptyItem;
  }

  bool doNext() override final {
    if (idx_ == result_.size()) {
      setDone();
    }
    if (done()) {
      return false;
    }
    idx_++;
    return true;
  }

  void setOrderCols(AttributeNameVec orderByColumns,
                    std::vector<bool> isDescending) {
    orderByColumns_ = std::move(orderByColumns);
    isDescending_ = std::move(isDescending);
  }

  const AttributeNameVec& orderByColumns() const { return orderByColumns_; }

  const std::vector<bool>& isDescending() { return isDescending_; }

 private:
  size_t idx_;
  std::vector<T> result_;
  AttributeNameVec orderByColumns_;
  std::vector<bool> isDescending_;

  folly::Future<std::vector<T>> fResult_;
};
}
