// Copyright 2015 - present, Facebook

#pragma once

#include "iterlib/Iterator.h"
#include "iterlib/WrappedIterator.h"

namespace iterlib {

// Nest the inner iterator inside the given key
//
// If your inner iterator yields { "foo" : bar }
// and your nest key is "baz", this iterator will
// yield: { "baz" : { "foo" : bar } }
class NestIterator : public WrappedIterator {
 public:
  explicit NestIterator(Iterator *inner, dynamic key)
      : WrappedIterator(inner)
      , key_(std::move(key)) {}

  const Item& value() const override {
    return value_;
  }
 protected:

  bool doNext() override {
    auto ret = innerIter_->next();
    storeData();
    return ret;
  }

  void storeData() {
    const auto& inner = innerIter_->value();
    const dynamic& val = inner;
    value_ = variant::ordered_map_t{{ key_, val }};
    value_.setId(inner.id());
    value_.setTs(inner.ts());
  }

  dynamic key_;
  ItemOptimized value_;
};

}
