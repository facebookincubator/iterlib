//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

namespace iterlib {
namespace detail {

template <typename T>
OrIteratorBase<T>::OrIteratorBase(IteratorVector<T>& iters)
  : CompositeIterator<T>(iters) {
  for (auto& child : this->children()) {
    if (!child) {
      continue;
    }
    this->activeChildren_.push_back(child.get());
  }
}

template <typename T>
void OrIteratorBase<T>::updateActiveChildren() {
  auto removeIf = std::remove_if(
      activeChildren_.begin(),
      activeChildren_.end(),
      [](const Iterator<T>* it) { return it->done(); });
  this->activeChildren_.erase(removeIf, activeChildren_.end());
}

template <class Comparator, typename T>
OrIterator<Comparator, T>::OrIterator(IteratorVector<T>& children)
  : OrIteratorBase<T>(children), firstTime_(true) {}

template <class Comparator, typename T>
void OrIterator<Comparator, T>::doFirst() {
  firstTime_ = false;
  for (auto& child : activeChildren_) {
    if (child->id() == max()) {
      child->next();
    }
  }
  this->updateActiveChildren();

  if (this->activeChildren_.empty()) {
    this->setDone();
    return;
  }
  std::make_heap(activeChildren_.begin(), activeChildren_.end(), Comparator());
}

template <class Comparator, typename T>
bool OrIterator<Comparator, T>::doNext() {
  if (this->done()) {
    return false;
  }

  auto current = this->id();
  if (firstTime_) {
    this->doFirst();
    current = max();
  }

  while ((!activeChildren_.empty()) &&
          ((activeChildren_.front()->id() == current) ||
          (activeChildren_.front()->id() == max()))) {
    std::pop_heap(activeChildren_.begin(), activeChildren_.end(), Comparator());
    auto iter = activeChildren_.back();
    activeChildren_.pop_back();
    if (iter->next()) {
      activeChildren_.push_back(iter);
      std::push_heap(
        activeChildren_.begin(), activeChildren_.end(), Comparator());
    }
  }
  if (activeChildren_.empty()) {
    this->setDone();
    return false;
  }

  return true;
}

template <class Comparator, typename T>
bool OrIterator<Comparator, T>::doSkipTo(id_t id) {
  if (this->done()) {
    return false;
  }

  if (firstTime_) {
    this->doFirst();
  }

  while ((!activeChildren_.empty()) &&
         (((std::is_same<Comparator, IdLessComp<T>>::value) &&
           (activeChildren_.front()->id() > id)) ||
          (!(std::is_same<Comparator, IdLessComp<T>>::value) &&
           (activeChildren_.front()->id() == id)))) {
    std::pop_heap(activeChildren_.begin(), activeChildren_.end(), Comparator());
    auto iter = activeChildren_.back();
    activeChildren_.pop_back();
    if (iter->skipTo(id)) {
      activeChildren_.push_back(iter);
      std::push_heap(
        activeChildren_.begin(), activeChildren_.end(), Comparator());
    }
  }

  if (activeChildren_.empty()) {
    this->setDone();
    return false;
  }

  return true;
}

template <typename T>
ConcatIterator<T>::ConcatIterator(IteratorVector<T>& children,
                                  bool dedup)
  : OrIteratorBase<T>(children)
  , idx_(0)
  , dedup_(dedup) {}

template <typename T>
folly::Future<folly::Unit> ConcatIterator<T>::prepare() {
  if (this->prepared_) {
    return folly::makeFuture();
  }

  std::vector<folly::Future<folly::Unit>> fs;
  fs.reserve(activeChildren_.size());
  for (auto& iter : activeChildren_) {
    fs.emplace_back(iter->prepare());
  }

  return folly::collectAll(fs)
    .then([this](std::vector<folly::Try<folly::Unit>>&& vec) {
        for (auto& t : vec) {
          t.throwIfFailed();
        }
        bool isDone = true;
        for (auto& child : activeChildren_) {
          if (child && !child->done()) {
            isDone = false;
          }
        }
        if (isDone) {
          this->setDone();
        }
        this->updateActiveChildren();
      })
    .onError([](const std::exception& ex) {
        LOG(ERROR) << "Failed to prepare ConcatIterator's child iters: "
                   << ex.what();
        throw ex;
      })
    .ensure([this]() {
      this->prepared_ = true;
    });
}

template <typename T>
bool ConcatIterator<T>::doNext() {
  if (this->done()) {
    return false;
  }

  // advance to next unique id
  auto* iter = activeChildren_[idx_];
  do {
    while (!iter->next()) {
      idx_++;
      if (idx_ >= activeChildren_.size()) {
        iter = nullptr;
        break;
      } else {
        iter = activeChildren_[idx_];
      }
    }
  } while (iter != nullptr && isDuplicate(iter->id()));

  if (iter == nullptr) {
    this->setDone();
    return false;
  }

  this->key_ = iter->key();
  this->results_.insert(this->id());
  return true;
}

}
}
