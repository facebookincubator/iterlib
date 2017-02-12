// Copyright 2015 - present, Facebook

#include "iterlib/OrderByIterator.h"

namespace iterlib {

template <typename T>
bool OrderByIterator<T>::Comparator::
operator()(const std::pair<const T*, int>& v1,
           const std::pair<const T*, int>& v2) const {
  auto cmp =
      partialCompare(*v1.first, *v2.first, columns_, isColumnDescending_);
  return cmp == PartialOrder::LT ||
         (cmp == PartialOrder::EQ && v1.second > v2.second);
}
}
