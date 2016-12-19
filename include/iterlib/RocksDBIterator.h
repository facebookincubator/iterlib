//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
#pragma once

#include <list>
#include <memory>
#include <rocksdb/iterator.h>

#include "iterlib/Iterator.h"

namespace iterlib {

inline folly::StringPiece sliceToStringPiece(const rocksdb::Slice s) {
  return {s.data(), s.size()};
}

class RocksDBIterator : public Iterator {
 public:
  RocksDBIterator(rocksdb::Iterator* iter) : iter_(iter) {}

  const Item& key() const override { return keys_.back(); }

  const Item& value() const override { return values_.back(); }

 protected:
  bool doNext() override {
    if (!firstTime_) {
      iter_->Next();
    } else {
      firstTime_ = false;
    }
    auto ret = iter_->Valid();
    if (ret) {
      keys_.emplace_back(sliceToStringPiece(iter_->key()));
      values_.emplace_back(sliceToStringPiece(iter_->value()));
    }
    return ret;
  }

  /*
   * TODO: Optimize these methods

     virtual bool doSkipTo(id_t id);
     virtual bool doSkipToPredicate(AttributeNameVec predicate,
                                    const Item& target);
     virtual bool doSkip(size_t n);

   */

  // These are mutable so the corresponding methods can be
  // const and can return const references
  //
  // We need to make sure that every reference every returned
  // via key()/value() methods remain valid until the iterator
  // is destroyed. Complexity of lookup irrelevant. Hence std::list
  // vector invalidates references on re-allocation
  mutable std::list<Item> keys_;
  mutable std::list<Item> values_;

  std::unique_ptr<rocksdb::Iterator> iter_;
  bool firstTime_ = true;
};
}
