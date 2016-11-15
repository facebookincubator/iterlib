//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include "iterlib/Iterator.h"

#include "iterlib/Item.h"

namespace iterlib {

Iterator::Iterator(IteratorType type)
    : IteratorTraits(), isDone_(false), key_(Item::kEmptyItem),
      advancedAtleastOnce_(false), prepared_(false), iteratorType_(type) {}

bool Iterator::doSkipTo(id_t target) {
  // Ids are sorted in reverse order (larger ids go first)
  while (value().id() > target && next())
    ;

  return !done();
}

bool Iterator::next() {
  throwIfUnPrepared();
  auto ret(doNext());
  if (ret) {
    advancedAtleastOnce_ = true;
  }
  return ret;
}

bool Iterator::doSkip(size_t n) {
  while (n > 0 && next()) {
    --n;
  }
  return !done();
}

// Simple base implementation. Not the most optimal.
bool Iterator::doSkipToPredicate(AttributeNameVec predicate,
                                 const Item& target) {
  if (done()) {
    return false;
  }
  PartialOrder cmpVal;
  do {
    cmpVal = partialCompare(target, value(), predicate);
  } while (cmpVal == PartialOrder::LT && next());
  return !done();
}
}
