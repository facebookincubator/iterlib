#pragma once

#include "iterlib/WrappedIterator.h"

namespace iterlib {

// Get random results from the inner Iterator
//
// Uses reservoir sampling:
// https://en.wikipedia.org/wiki/Reservoir_sampling
//
// If the inner iterator has fewer than the
// requested items, we select all of them.
class RandomIterator : public WrappedIterator {
public:
  RandomIterator(Iterator* iter, int count)
    : WrappedIterator(iter)
    , count_(count)
    , firstTime_(true){
  }

  virtual const Item& value() const override {
    return randomsamples_.back();
  }

protected:
 bool doNext() override;

private:
  void getRandomSamples();
  int32_t count_;
  bool firstTime_;

  std::vector<Item> randomsamples_;
};

}
