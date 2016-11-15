//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include "iterlib/variant/dynamic.h"

namespace iterlib {
namespace variant {
const dynamic dynamic::kNullDynamic;
const dynamic dynamic_iterator::kNullDynamic;

dynamic_iterator dynamic::begin() const { return dynamic_iterator(this); }

dynamic_iterator dynamic::end() const { return dynamic_iterator(); }

dynamic_iterator* dynamic::begin_ptr() const {
  return new dynamic_iterator(this);
}

void PrintTo(const dynamic& dyn, std::ostream* os) { *os << dyn.toString(); }

void dynamic::merge(const dynamic& other) {
  try {
    mergeImpl(other);
  } catch (const std::exception& e) {
    LOG(ERROR) << "Error merging: " << toJson() << " " << other.toJson() << " "
               << e.what();
    throw e;
  }
}

void dynamic::mergeImpl(const dynamic& other) {
  DCHECK(other.isObject());
  for (const auto& element : other) {
    const auto first = dynamic(element.first);
    const auto second = dynamic(element.second.get());
    VLOG(1) << "Merging: " << first.toJson() << " : " << second.toJson();
    if (atNoThrow(first) != kNullDynamic) {
      if (at(first).isObject()) {
        // Example:
        // aggregate(k => { "id" : v1 }, k => { "id" : v2 })
        // will result in
        // k => [ { "id" : v1 }, { "id" : v2 } ]
        const dynamic_variant& val = (*this)[first];
        auto tmp = vector_dynamic_t({val});
        (*this)[first] = std::move(tmp);
      }
      (*this)[first] += second;
    } else {
      (*this)[first] = second;
    }
  }
}
}
}
