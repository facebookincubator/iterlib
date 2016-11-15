//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include "iterlib/Iterator.h"

namespace iterlib {

class WrappedIterator : public Iterator {
 public:
  explicit WrappedIterator(Iterator* iter)
      : Iterator(IteratorType::WRAPPED), innerIter_(iter) {}

  virtual const Item& key() const override { return innerIter_->key(); }

  virtual const Item& value() const override { return innerIter_->value(); }

  virtual folly::Future<folly::Unit> prepare() override;

  virtual bool orderPreserving() const { return false; }

 protected:
  std::unique_ptr<Iterator> innerIter_;
};
}
