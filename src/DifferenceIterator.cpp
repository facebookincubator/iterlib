#include "iterlib/DifferenceIterator.h"

namespace iterlib {

bool DifferenceIterator::advanceToNextDifference() {
  if (getSecondIterator() != nullptr) {
    while (getSecondIterator()->skipTo(getFirstIterator()->id()) &&
           getFirstIterator()->id() == getSecondIterator()->id()) {
      if (!getFirstIterator()->next()) {
        setDone();
        return false;
      }
    }
  }

  return true;
}

bool DifferenceIterator::doNext() {
  if (done()) {
    return false;
  }

  if (!getFirstIterator()->next()) {
    setDone();
    return false;
  }

  return advanceToNextDifference();
}

bool DifferenceIterator::doSkipTo(id_t id) {
  if (done()) {
    return false;
  }

  if (!getFirstIterator()->skipTo(id)){
    setDone();
    return false;
  }

  return advanceToNextDifference();
}

}
