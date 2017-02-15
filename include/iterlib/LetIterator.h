// Copyright 2015 - present, Facebook

#pragma once

#include "iterlib/Iterator.h"
#include "iterlib/WrappedIterator.h"

namespace iterlib {
namespace detail {

template <typename T=Item>
class LetIterator : public WrappedIterator<T> {
 public:
  explicit LetIterator(Iterator<T> *inner,
                       dynamic newKey,
                       dynamic oldKey)
      : WrappedIterator<T>(inner)
      , newKey_(std::move(newKey))
      , oldKey_(std::move(oldKey)) {}

  const T& value() const override {
    const auto& item = this->innerIter_->value();
    auto omap = item.asRenamedMap(oldKey_, newKey_);
    value_.reset();
    value_.setId(item.id());
    value_.setTs(item.ts());
    value_ = std::move(omap);
    value_.syncIdTs();
    return value_;
  }

  bool doNext() override {
    return this->innerIter_->next();
  }

  virtual bool orderPreserving() const { return true; }

 protected:

  dynamic newKey_;
  dynamic oldKey_;
  // TODO: revisit the contract that all references returned
  // from the iterator need to be valid even after next().
  // Do we want to modify the item in-place instead of making
  // a copy?
  mutable ItemOptimized value_;
};

}

using LetIterator = detail::LetIterator<Item>;

}
