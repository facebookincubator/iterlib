//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include "iterlib/OrderByIterator.h"

namespace iterlib {
namespace detail {

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
}
