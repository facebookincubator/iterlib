//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
#include "iterlib/MergeIterator.h"

namespace iterlib {

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
