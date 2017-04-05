#pragma once

#include "iterlib/WrappedIterator.h"

namespace iterlib {
namespace detail {

// Get random results from the inner Iterator
//
// Uses reservoir sampling:
// https://en.wikipedia.org/wiki/Reservoir_sampling
//
// If the inner iterator has fewer than the
// requested items, we select all of them.
template <typename T=Item>
class RandomIterator : public WrappedIterator<T> {
public:
  RandomIterator(Iterator<T>* iter, int count)
    : WrappedIterator<T>(iter)
    , count_(count)
    , firstTime_(true){
  }

  virtual const T& value() const override {
    return randomsamples_.back();
  }

protected:
 bool doNext() override;

private:
  void getRandomSamples();
  int32_t count_;
  bool firstTime_;

  std::vector<T> randomsamples_;
};

}

using RandomIterator = detail::RandomIterator<Item>;
extern template class detail::RandomIterator<Item>;

}

#include "iterlib/RandomIterator-inl.h"
