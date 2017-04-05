#pragma once

#include "iterlib/Iterator.h"

namespace iterlib {
namespace detail {

template <typename T=Item>
class AndIterator: public CompositeIterator<T> {
public:
  explicit AndIterator(IteratorVector<T>& iters)
    : CompositeIterator<T>(iters) {}
  AndIterator();

  virtual const T& key() const override { return last()->key(); }

  virtual const T& value() const override { return last()->value(); }

protected:
  bool advanceToLast();
  bool doNext() override;
  bool doSkipTo(id_t id) override;

private:
  inline Iterator<T>* last() const {
    return this->iterators_.back().get();
  }
};

}

using AndIterator = detail::AndIterator<Item>;
extern template class detail::AndIterator<Item>;

}

#include "iterlib/AndIterator-inl.h"
