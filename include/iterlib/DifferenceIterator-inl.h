//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

namespace iterlib {

template <typename T>
bool DifferenceIterator<T>::advanceToNextDifference() {
  if (getSecondIterator() != nullptr) {
    while (getSecondIterator()->skipTo(getFirstIterator()->id()) &&
           getFirstIterator()->id() == getSecondIterator()->id()) {
      if (!getFirstIterator()->next()) {
        this->setDone();
        return false;
      }
    }
  }

  return true;
}

template <typename T>
bool DifferenceIterator<T>::doNext() {
  if (this->done()) {
    return false;
  }

  if (!getFirstIterator()->next()) {
    this->setDone();
    return false;
  }

  return advanceToNextDifference();
}

template <typename T>
bool DifferenceIterator<T>::doSkipTo(id_t id) {
  if (this->done()) {
    return false;
  }

  if (!getFirstIterator()->skipTo(id)){
    this->setDone();
    return false;
  }

  return advanceToNextDifference();
}

}
