#pragma once

#include "iterlib/Iterator.h"

namespace iterlib {

class AndIterator: public CompositeIterator {
public:
  explicit AndIterator(IteratorVector& iters)
    : CompositeIterator(iters) {}
  AndIterator();

  virtual const Item& key() const override { return last()->key(); }

  virtual const Item& value() const override { return last()->value(); }

protected:
  bool advanceToLast();
  bool doNext() override;
  bool doSkipTo(id_t id) override;

private:
  inline Iterator* last() const {
    return iterators_.back().get();
  }
};

}
