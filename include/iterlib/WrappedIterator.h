//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include "iterlib/Iterator.h"

namespace iterlib {
namespace detail {

template <typename T=Item>
class WrappedIterator : public Iterator<T> {
 public:
  explicit WrappedIterator(Iterator<T>* iter)
      : Iterator<T>(IteratorType::WRAPPED), innerIter_(iter) {}

  virtual const T& key() const override { return innerIter_->key(); }

  virtual const T& value() const override { return innerIter_->value(); }

  virtual folly::Future<folly::Unit> prepare() override;

  virtual bool orderPreserving() const { return false; }

 protected:
  std::unique_ptr<Iterator<T>> innerIter_;
};

}
}

#include "iterlib/WrappedIterator-inl.h"
