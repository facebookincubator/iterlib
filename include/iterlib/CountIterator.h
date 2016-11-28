#pragma once

#include "iterlib/WrappedIterator.h"

namespace iterlib {

// count the number of items in iter
class CountIterator : public WrappedIterator {
public:
  explicit CountIterator(Iterator* iter)
    : WrappedIterator(iter)
    , countValue_(-1) {
  }

  virtual const Item& key() const override {
    return kCountKey;
  }

  virtual const Item& value() const override {
    return countValue_;
  }

  static const Item kCountKey;

protected:
  bool doNext() override;

private:
  mutable Item countValue_;
};

}
