//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
#include "iterlib/ProjectIterator.h"

namespace iterlib {

bool ProjectIterator::doNext() {
  return innerIter_->next();
}

const Item& ProjectIterator::value() const {
  try {
    const auto& item = innerIter_->value();
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

bool ProjectIterator::doSkipTo(id_t id) {
  if (!innerIter_->skipTo(id)) {
    setDone();
    return false;
  }
  return true;
}

}
