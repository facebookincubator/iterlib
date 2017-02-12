#include "iterlib/AndIterator.h"

namespace iterlib {

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
