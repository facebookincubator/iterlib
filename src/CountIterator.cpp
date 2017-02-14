#include "iterlib/CountIterator.h"

namespace iterlib {

template <typename T>
const T CountIterator<T>::kCountKey{{folly::StringPiece("count")}};

template class CountIterator<Item>;

}
