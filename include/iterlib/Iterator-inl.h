//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

namespace iterlib {

template <typename T>
Iterator<T>::Iterator(IteratorType type)
    : IteratorTraits(), isDone_(false), key_(Item::kEmptyItem),
      advancedAtleastOnce_(false), prepared_(false), iteratorType_(type) {}

template <typename T>
bool Iterator<T>::doSkipTo(id_t target) {
  // Ids are sorted in reverse order (larger ids go first)
  while (id() > target && next())
    ;

  return !done();
}

template <typename T>
bool Iterator<T>::next() {
  throwIfUnPrepared();
  auto ret(doNext());
  if (ret) {
    advancedAtleastOnce_ = true;
  }
  return ret;
}

template <typename T>
bool Iterator<T>::doSkip(size_t n) {
  while (n > 0 && next()) {
    --n;
  }
  return !done();
}

// Simple base implementation. Not the most optimal.
template <typename T>
bool Iterator<T>::doSkipToPredicate(AttributeNameVec predicate,
                                 const T& target) {
  if (done()) {
    return false;
  }
  PartialOrder cmpVal;
  do {
    cmpVal = partialCompare(target, value(), predicate);
  } while (cmpVal == PartialOrder::LT && next());
  return !done();
}

template <typename T>
folly::Future<folly::Unit> CompositeIterator<T>::prepare() {
  if (this->prepared_) {
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
            this->setDone();
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
      this->prepared_ = true;
    });
}

}
