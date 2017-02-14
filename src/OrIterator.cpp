#include "iterlib/OrIterator.h"

namespace iterlib {

// sorted-merge
template class OrIterator<StdLessComp<Item>, Item>;
// id() based set union
template class OrIterator<IdLessComp<Item>, Item>;

template class ConcatIterator<Item>;

}
