#pragma once

#include "iterlib/Iterator.h"

namespace iterlib {

template <typename T=Item>
class DifferenceIterator : public CompositeIterator<T> {
public:
  DifferenceIterator(IteratorVector<T>& children)
    : CompositeIterator<T>(children) {
    DCHECK_EQ(this->numChildIters(), 2);
  }

  virtual ~DifferenceIterator() {}

  virtual const T& value() const override {
    return this->iterators_.front()->value();
  }

  const std::unique_ptr<Iterator<T>>& getFirstIterator() const {
    return this->iterators_.front();
  }

  const std::unique_ptr<Iterator<T>>& getSecondIterator() const {
    return this->iterators_.back();
  }

protected:
  bool advanceToNextDifference();
  bool doNext() override;
  bool doSkipTo(id_t id) override;
};

}
#include "iterlib/DifferenceIterator-inl.h"
