// Copyright 2015 - present, Facebook

#pragma once

#include "iterlib/OrIterator.h"

namespace iterlib {
namespace detail {

// This performs an id() based merge of
// items among the child iterators
//
// The actual merge algorithm for children
// having the same id() is given by
// dynamic::merge()
template <typename T=Item>
class MergeIterator : public UnionIterator<T> {
 public:
  explicit MergeIterator(IteratorVector<T>& children)
      : UnionIterator<T>(children) {}

  virtual const T& value() const override {
    return value_;
  }

 protected:
  bool doNext() override {
    auto ret = UnionIterator<T>::doNext();
    storeData();
    return ret;
  }

  void storeData();

  ItemOptimized value_;
};

}

using MergeIterator = detail::MergeIterator<Item>;
extern template class detail::MergeIterator<Item>;

}
#include "iterlib/MergeIterator-inl.h"
