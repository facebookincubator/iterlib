//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

namespace iterlib {
namespace detail {

template <typename T>
void MergeIterator<T>::storeData() {
  value_ = this->activeChildren_.front()->value();
  // This is where the actual merge happens
  for (auto& child : this->activeChildren_) {
    if (child != this->activeChildren_.front() && child->id() == this->id()) {
      value_.merge(child->value());
    }
  }
}

}
}
