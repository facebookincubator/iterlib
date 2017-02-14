//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

namespace iterlib {

template <typename T>
bool ProjectIterator<T>::doNext() {
  return this->innerIter_->next();
}

template <typename T>
const T& ProjectIterator<T>::value() const {
  try {
    const auto& item = this->innerIter_->value();
    auto projection = item.asProjectedMap(attrNames_);
    value_.reset();
    value_.setId(item.id());
    value_.setTs(item.ts());
    value_ = std::move(projection);
    value_.syncIdTs();
    return value_;
  } catch (std::exception& ex) {
    LOG_EVERY_N(ERROR, 500) << ex.what();
    return Item::kEmptyItem;
  }
}

template <typename T>
bool ProjectIterator<T>::doSkipTo(id_t id) {
  if (!this->innerIter_->skipTo(id)) {
    this->setDone();
    return false;
  }
  return true;
}

}
