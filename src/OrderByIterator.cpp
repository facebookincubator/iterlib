// Copyright 2015 - present, Facebook

#include "iterlib/OrderByIterator.h"

namespace iterlib {

bool OrderByIterator::Comparator::
operator()(const std::pair<const Item*, int>& v1,
           const std::pair<const Item*, int>& v2) const {
  auto cmp =
      partialCompare(*v1.first, *v2.first, columns_, isColumnDescending_);
  return cmp == PartialOrder::LT ||
         (cmp == PartialOrder::EQ && v1.second > v2.second);
}
}
