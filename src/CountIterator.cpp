#include "iterlib/CountIterator.h"

namespace iterlib {

template <typename T>
const T CountIterator<T>::kCountKey{{folly::StringPiece("count")}};

template <typename T>
bool CountIterator<T>::doNext() {
  auto count = countValue_.template get<int64_t>();
  if (this->done() || count > 0) {
    return false;
  }

  count = 0;
  while (this->innerIter_->next()) {
    count++;
  }
  countValue_ = count;
  return true;
}

}
