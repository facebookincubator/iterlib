//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
#include "iterlib/MergeIterator.h"

namespace iterlib {

void MergeIterator::storeData() {
  value_ = activeChildren_.front()->value();
  // This is where the actual merge happens
  for (auto& child : activeChildren_) {
    if (child != activeChildren_.front() && child->id() == id()) {
      value_.merge(child->value());
    }
  }
}

}
