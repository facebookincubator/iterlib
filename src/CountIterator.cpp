#include "iterlib/CountIterator.h"

namespace iterlib {
namespace detail {

template <typename T>
const T CountIterator<T>::kCountKey{{folly::StringPiece("count")}};

template class CountIterator<Item>;

}
}
