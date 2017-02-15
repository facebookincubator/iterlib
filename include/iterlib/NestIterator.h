// Copyright 2015 - present, Facebook

#pragma once

#include "iterlib/Iterator.h"
#include "iterlib/WrappedIterator.h"

namespace iterlib {
namespace detail {

// Nest the inner iterator inside the given key
//
// If your inner iterator yields { "foo" : bar }
// and your nest key is "baz", this iterator will
// yield: { "baz" : { "foo" : bar } }
template <typename T=Item>
class NestIterator : public WrappedIterator<T> {
 public:
  explicit NestIterator(Iterator<T> *inner, dynamic key)
      : WrappedIterator<T>(inner)
      , key_(std::move(key)) {}

  const T& value() const override {
    return value_;
  }
 protected:

  bool doNext() override {
    auto ret = this->innerIter_->next();
    storeData();
    return ret;
  }

  void storeData() {
    const auto& inner = this->innerIter_->value();
    const dynamic& val = inner;
    value_ = variant::ordered_map_t{{ key_, val }};
    value_.setId(inner.id());
    value_.setTs(inner.ts());
  }

  dynamic key_;
  ItemOptimized value_;
};

}

using NestIterator = detail::NestIterator<Item>;

}
