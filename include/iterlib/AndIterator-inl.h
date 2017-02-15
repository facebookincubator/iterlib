//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

namespace iterlib {
namespace detail {

template <typename T>
bool AndIterator<T>::doNext() {
  if (this->done()) {
    return false;
  }

  auto* lastIt = last();
  // if last iterator is nullptr, directly return null
  if (lastIt == nullptr) {
    this->setDone();
    return false;
  }

  if (!lastIt->next()) {
    this->setDone();
    return false;
  }

  return advanceToLast();
}

template <typename T>
bool AndIterator<T>::doSkipTo(id_t id) {
  if (this->done()) {
    return false;
  }

  auto* lastIt = last();
  if (!lastIt->skipTo(id)){
    this->setDone();
    return false;
  }
  return advanceToLast();
}

template <typename T>
bool AndIterator<T>::advanceToLast() {
  auto* lastIt = last();
  Iterator<T>* it;
  size_t i = 0;

  // Try to position all iterators on the same doc
  // or return false if not possible
  id_t id;
  while ((it = this->iterators_[i].get())->id() >
      (id = lastIt->id())) {
    if (!it->skipTo(id)) {
      this->setDone();
      return false;
    }
    lastIt = it;
    if (++i == this->iterators_.size()) {
      i = 0;
    }
  }
  return true;
}

}
}
