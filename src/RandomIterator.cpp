#include "iterlib/RandomIterator.h"

#include <folly/Random.h>

namespace iterlib {

bool RandomIterator::doNext() {
  if (done()) {
    return false;
  }

  // only get samples in the first time
  if (firstTime_) {
    firstTime_ = false;
    getRandomSamples();
    std::make_heap(randomsamples_.begin(), randomsamples_.end());
  } else {
    randomsamples_.pop_back();
  }

  if (randomsamples_.empty()) {
    setDone();
    return false;
  }

  std::pop_heap(randomsamples_.begin(), randomsamples_.end());
  return true;
}

// reservoir sampling
void RandomIterator::getRandomSamples() {
  if (count_ <= 0) {
    return;
  }
  // read count_
  int32_t index = 0;
  while (index < count_ ) {
    if (!innerIter_->next()) {
      return;
    }
    randomsamples_.push_back(innerIter_->value());
    index++;
  }

  // Replace elements in random samples with later results.
  // using reservoir sampling
  int skip = 0;
  while (1) {
    ++skip;
    int x = folly::Random::rand32(index+1);
    if ( x < count_) {
      if (!innerIter_->skip(skip)) {
        return;
      }
      // swap results
      skip = 0;
      randomsamples_[x] = innerIter_->value();
    }
    ++index;
  }

};

}
