#include "iterlib/AndIterator.h"

namespace iterlib {

bool AndIterator::doNext() {
  if (done()) {
    return false;
  }

  Iterator* lastIt = last();
  // if last iterator is nullptr, directly return null
  if (lastIt == nullptr) {
    setDone();
    return false;
  }

  if (!lastIt->next()) {
    setDone();
    return false;
  }

  return advanceToLast();
}

bool AndIterator::doSkipTo(id_t id) {
  if (done()) {
    return false;
  }

  Iterator* lastIt = last();
  if (!lastIt->skipTo(id)){
    setDone();
    return false;
  }
  return advanceToLast();
}

bool AndIterator::advanceToLast() {
  Iterator* lastIt = last();
  Iterator* it;
  size_t i = 0;

  // Try to position all iterators on the same doc
  // or return false if not possible
  id_t id;
  while ((it = iterators_[i].get())->id() >
      (id = lastIt->id())) {
    if (!it->skipTo(id)) {
      setDone();
      return false;
    }
    lastIt = it;
    if (++i == iterators_.size()) {
      i = 0;
    }
  }
  return true;
}

}
