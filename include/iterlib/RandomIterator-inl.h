//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <folly/Random.h>

namespace iterlib {

template <typename T>
bool RandomIterator<T>::doNext() {
  if (this->done()) {
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
    this->setDone();
    return false;
  }

  std::pop_heap(randomsamples_.begin(), randomsamples_.end());
  return true;
}

// reservoir sampling
template <typename T>
void RandomIterator<T>::getRandomSamples() {
  if (count_ <= 0) {
    return;
  }
  // read count_
  int32_t index = 0;
  while (index < count_ ) {
    if (!this->innerIter_->next()) {
      return;
    }
    randomsamples_.push_back(this->innerIter_->value());
    index++;
  }

  // Replace elements in random samples with later results.
  // using reservoir sampling
  int skip = 0;
  while (1) {
    ++skip;
    int x = folly::Random::rand32(index+1);
    if ( x < count_) {
      if (!this->innerIter_->skip(skip)) {
        return;
      }
      // swap results
      skip = 0;
      randomsamples_[x] = this->innerIter_->value();
    }
    ++index;
  }

};

}
