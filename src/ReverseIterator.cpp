#include "iterlib/ReverseIterator.h"

namespace iterlib {

void ReverseIterator::load() {
  while (innerIter_->next()) {
    results_.emplace_back(innerIter_->value());
  }
}

bool ReverseIterator::doNext() {
  if (done()) {
    return false;
  }

  // only get samples in the first time
  if (firstTime_) {
    firstTime_ = false;
    load();
  } else {
    results_.pop_back();
  }

  if (results_.empty()) {
    setDone();
    return false;
  }

  return true;
}

}
