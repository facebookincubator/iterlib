#include "iterlib/DifferenceIterator.h"

namespace iterlib {

template <typename T>
bool DifferenceIterator<T>::advanceToNextDifference() {
  if (getSecondIterator() != nullptr) {
    while (getSecondIterator()->skipTo(getFirstIterator()->id()) &&
           getFirstIterator()->id() == getSecondIterator()->id()) {
      if (!getFirstIterator()->next()) {
        this->setDone();
        return false;
      }
    }
  }

  return true;
}

template <typename T>
bool DifferenceIterator<T>::doNext() {
  if (this->done()) {
    return false;
  }

  if (!getFirstIterator()->next()) {
    this->setDone();
    return false;
  }

  return advanceToNextDifference();
}

template <typename T>
bool DifferenceIterator<T>::doSkipTo(id_t id) {
  if (this->done()) {
    return false;
  }

  if (!getFirstIterator()->skipTo(id)){
    this->setDone();
    return false;
  }

  return advanceToNextDifference();
}

}
