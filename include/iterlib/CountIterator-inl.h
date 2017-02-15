//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

namespace iterlib {
namespace detail {

template <typename T>
bool CountIterator<T>::doNext() {
  auto count = countValue_.template get<int64_t>();
  if (this->done() || count > 0) {
    return false;
  }

  count = 0;
  while (this->innerIter_->next()) {
    count++;
  }
  countValue_ = count;
  return true;
}

}
}
