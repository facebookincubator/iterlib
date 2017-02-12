//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
#include "iterlib/ProjectIterator.h"

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
