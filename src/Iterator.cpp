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
  while (id() > target && next())
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

folly::Future<folly::Unit> CompositeIterator::prepare() {
  if (prepared_) {
    return folly::makeFuture();
  }

  std::vector<folly::Future<folly::Unit>> fs;
  fs.reserve(iterators_.size());
  for (auto& iter : iterators_) {
    if (iter) {
      fs.emplace_back(iter->prepare());
    }
  }
  return folly::collectAll(fs)
    .then([this](std::vector<folly::Try<folly::Unit>>&& vec) {
        for (auto& t : vec) {
          t.throwIfFailed();
        }
        bool hasFirstAvailable = false;
        for (auto& iter : iterators_) {
          if (!iter || iter->done()) {
            setDone();
            break;
          }
          if (!hasFirstAvailable) {
            key_ = iter->key();
            hasFirstAvailable = true;
          }
        }
      })
    .onError([](const std::exception& ex) {
        LOG(ERROR) << "Failed to prepare CompositeIterator's child iters: "
                   << ex.what();
        throw ex;
      })
    .ensure([this]() {
      prepared_ = true;
    });
}
}
