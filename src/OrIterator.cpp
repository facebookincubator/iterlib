#include "iterlib/OrIterator.h"

namespace iterlib {

OrIteratorBase::OrIteratorBase(IteratorVector& iters)
  : CompositeIterator(iters) {
  for (auto& child : children()) {
    if (!child) {
      continue;
    }
    activeChildren_.push_back(child.get());
  }
}

void OrIteratorBase::updateActiveChildren() {
  auto removeIf = std::remove_if(
      activeChildren_.begin(),
      activeChildren_.end(),
      [](const Iterator* it) { return it->done(); });
  activeChildren_.erase(removeIf, activeChildren_.end());
}

template <class Comparator>
OrIterator<Comparator>::OrIterator(IteratorVector& children)
  : OrIteratorBase(children), firstTime_(true) {}

template <class Comparator>
void OrIterator<Comparator>::doFirst() {
  firstTime_ = false;
  for (auto& child : activeChildren_) {
    if (child->id() == max()) {
      child->next();
    }
  }
  updateActiveChildren();

  if (activeChildren_.empty()) {
    setDone();
    return;
  }
  std::make_heap(activeChildren_.begin(), activeChildren_.end(), Comparator());
}

template <class Comparator>
bool OrIterator<Comparator>::doNext() {
  if (done()) {
    return false;
  }

  auto current = id();
  if (firstTime_) {
    doFirst();
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
    setDone();
    return false;
  }

  return true;
}

template <class Comparator>
bool OrIterator<Comparator>::doSkipTo(id_t id) {
  if (done()) {
    return false;
  }

  if (firstTime_) {
    doFirst();
  }

  while ((!activeChildren_.empty()) &&
         (((std::is_same<Comparator, IdLessComp>::value) &&
           (activeChildren_.front()->id() > id)) ||
          (!(std::is_same<Comparator, IdLessComp>::value) &&
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
    setDone();
    return false;
  }

  return true;
}

ConcatIterator::ConcatIterator(IteratorVector& children,
                                   bool dedup)
  : OrIteratorBase(children)
  , idx_(0)
  , dedup_(dedup) {}

folly::Future<folly::Unit> ConcatIterator::prepare() {
  if (prepared_) {
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
          setDone();
        }
        updateActiveChildren();
      })
    .onError([](const std::exception& ex) {
        LOG(ERROR) << "Failed to prepare ConcatIterator's child iters: "
                   << ex.what();
        throw ex;
      })
    .ensure([this]() {
      prepared_ = true;
    });
}

bool ConcatIterator::doNext() {
  if (done()) {
    return false;
  }

  // advance to next unique id
  Iterator* iter = activeChildren_[idx_];
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
    setDone();
    return false;
  }

  key_ = iter->key();
  results_.insert(id());
  return true;
}

// sorted-merge
template class OrIterator<StdLessComp>;
// id() based set union
template class OrIterator<IdLessComp>;

}
