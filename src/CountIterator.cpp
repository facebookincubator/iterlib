#include "iterlib/CountIterator.h"

namespace iterlib {

const Item CountIterator::kCountKey{{folly::StringPiece("count")}};

bool CountIterator::doNext() {
  auto count = countValue_.get<int64_t>();
  if (done() || count > 0) {
    return false;
  }

  count = 0;
  while (innerIter_->next()) {
    count++;
  }
  countValue_ = count;
  return true;
}

}
