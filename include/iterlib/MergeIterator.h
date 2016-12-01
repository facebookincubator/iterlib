// Copyright 2015 - present, Facebook

#pragma once

#include "iterlib/OrIterator.h"

namespace iterlib {

// This performs an id() based merge of
// items among the child iterators
//
// The actual merge algorithm for children
// having the same id() is given by
// dynamic::merge()
class MergeIterator : public UnionIterator {
 public:
  explicit MergeIterator(IteratorVector& children)
      : UnionIterator(children) {}

  virtual const Item& value() const override {
    return value_;
  }

 protected:
  bool doNext() override {
    auto ret = UnionIterator::doNext();
    storeData();
    return ret;
  }

  void storeData();

  ItemOptimized value_;
};

}
