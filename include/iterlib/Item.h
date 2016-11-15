//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
#pragma once

#include <tuple>

#include "iterlib/QueryConstants.h"
#include "iterlib/dynamic.h"

namespace iterlib {

typedef uint64_t id_t;
static const id_t INVALID_ID = -1;

class Item : public dynamic {
 public:
  using dynamic::operator=;
  using dynamic::is_of;
  using dynamic::get;
  using dynamic::getRef;
  using dynamic::getNonConstRef;

  static const Item kEmptyItem;
  static const id_t kUninitializedId = 0;

  Item() : dynamic() {}
  Item(const dynamic& other) : dynamic(other) {}

  Item(const Item& other) = default;
  Item(Item&& other) = default;
  Item& operator=(const Item& other) = default;
  Item& operator=(Item&& other) = default;

  bool operator==(const Item& other) const {
    return dynamic::operator==(static_cast<const dynamic&>(other));
  }

  id_t id() const {
    try {
      return at(kIdKey).get<int64_t>();
    } catch (const std::exception& ex) {
      LOG_EVERY_N(WARNING, 1000) << ex.what();
    }
  }

  int64_t ts() const {
    try {
      return at(kTimeKey).get<int64_t>();
    } catch (const std::exception& ex) {
      LOG_EVERY_N(WARNING, 1000) << ex.what();
    }
  }

  const dynamic& value() const { return *this; }
};

// Provides O(1) access to id() and ts(). Otherwise identical to
// Item
class ItemOptimized : public Item {
 public:
  using Item::operator=;
  using Item::is_of;
  using Item::get;
  using Item::getRef;
  using Item::getNonConstRef;

  ItemOptimized() : Item(), id_(kUninitializedId), ts_(0) {}

  ItemOptimized(id_t id, int64_t ts, dynamic value = dynamic())
      : Item(std::move(value)), id_(id), ts_(ts) {}

  ItemOptimized(const ItemOptimized& other) = default;
  ItemOptimized& operator=(const ItemOptimized& other) = default;
  ItemOptimized& operator=(ItemOptimized&& other) = default;

  void reset() {
    id_ = 0;
    ts_ = 0;
    *this = dynamic();
  }

  id_t id() const { return id_; }

  void setId(id_t id) { id_ = id; }

  void setTs(int64_t ts) { ts_ = ts; }

  int64_t ts() const { return ts_; }

  void syncIdTs() {
    try {
      id_ = at(kIdKey).get<int64_t>();
      ts_ = at(kTimeKey).get<int64_t>();
    } catch (const std::exception& ex) {
      LOG_EVERY_N(WARNING, 1000) << ex.what();
    }
  }

  // This depends on value().which(). For eg: vector_pair_t can't
  // be modified.
  void syncMap() {
    try {
      (*this)[kIdKey] = dynamic(int64_t(id_));
      (*this)[kTimeKey] = dynamic(int64_t(ts_));
    } catch (const std::exception& ex) {
      LOG_EVERY_N(WARNING, 1000) << ex.what();
    }
  }

  void sync() {
    // This is a heuristic. The actual sync depends on what was
    // modified. Use syncIdTs/syncMap directly depending on the
    // context.
    if (id_ == kUninitializedId) {
      syncIdTs();
    } else {
      syncMap();
    }
  }

  const dynamic& value() const { return *this; }

  bool operator<(const ItemOptimized& other) const {
    return std::tie(value(), ts_, id_) <
           std::tie(other.value(), other.ts_, other.id_);
  }

  bool operator==(const ItemOptimized& other) const {
    return std::tie(value(), id_, ts_) ==
           std::tie(other.value(), other.id_, other.ts_);
  }

  // Weaker form of operator== which ignores types.
  // May relax the equality criteria further based on use cases.
  bool equals(const ItemOptimized& other) const {
    if (std::tie(id_, ts_) != std::tie(other.id_, other.ts_)) {
      return false;
    }
    if (value().empty() && other.value().empty()) {
      return true;
    }
    return value() == other.value();
  }

  // Used by groupby. Note that id/ts are not populated
  ItemOptimized& operator=(const dynamic& other) {
    dynamic::operator=(other);
    return *this;
  }

  ItemOptimized& operator=(dynamic&& other) {
    dynamic::operator=(other);
    return *this;
  }

  bool isAdditionalField(const std::string& key) const {
    return key == kIdKey || key == kTimeKey;
  }

  int64_t getAdditionalField(const std::string& key) const {
    if (key == kIdKey) {
      return id_;
    } else if (key == kTimeKey) {
      return ts_;
    }
    throw std::logic_error("not an additional field");
  }

  std::string toString() const {
    return folly::sformat("[{0}, {1}, {2}]", ts(), id(), dynamic::toString());
  }

 protected:
  id_t id_;
  int64_t ts_;
};

// partialCompare is a multi value function:
//
// LT  => less than
// EQ  => equal
// GT  => greater than
// NONE => Not comparable
//
// Notice that this signature is different from std:: compare (two value)
// and also different from C-style compare (greater/equal/less) which assumes
// total ordering.
//
// The function performs a projection as well as comparison and deals with
// corner cases involving some columns not being present in v1 or v2
enum class PartialOrder { NONE, LT, EQ, GT };
PartialOrder partialCompare(const Item& v1, const Item& v2,
                            const std::vector<std::string>& columns,
                            const std::vector<bool>& isColumnDescending);

PartialOrder partialCompare(const Item& v1, const Item& v2,
                            const std::vector<std::string>& columns);

inline std::ostream& operator<<(std::ostream& os, const Item& row) {
  return os << row.toString();
}
}
