//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include "iterlib/LimitIterator.h"

namespace iterlib {

bool LimitIterator::doNext() {
  if (count_ <= 0 || done()) {
    VLOG(1) << "Limit iterator count reached: " << cookie();
    setDone();
    return false;
  }

  // first time, we skip the iterator past the given offset
  bool ret;
  if (firstTime_ && startOffset_ > 0) {
    firstTime_ = false;
    ret = innerIter_->skip(startOffset_ + 1);
  } else {
    ret = innerIter_->next();
  }

  if (!ret) {
    setDone();
    return false;
  }

  count_--;
  return ret;
}
}
