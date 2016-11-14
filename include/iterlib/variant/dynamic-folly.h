//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
//
// Code to convert from/to folly::dynamic
#pragma once
#include <folly/dynamic.h>

namespace iterlib { namespace variant {

folly::dynamic toFollyDynamic(const dynamic& v);

struct FollyDynamicConverter : boost::static_visitor<folly::dynamic> {

  folly::dynamic operator() (const boost::blank v) const {
    return folly::dynamic::object;
  }

  folly::dynamic operator() (const bool v) const {
    return folly::dynamic(v);
  }

  folly::dynamic operator() (const int64_t v) const {
    return folly::dynamic(v);
  }

  folly::dynamic operator() (const double v) const {
    return folly::dynamic(v);
  }

  folly::dynamic  operator() (const folly::StringPiece v) const {
    return folly::dynamic(v);
  }

  folly::dynamic operator() (const vector_dynamic_t& vec) const {
    folly::dynamic dyn = folly::dynamic::array;
    for (const auto& val : vec) {
      dyn.push_back(boost::apply_visitor(*this, val));
    }
    return dyn;
  }

  template <typename T>
  folly::dynamic operator() (const std::vector<T>& vec) const {
    folly::dynamic dyn = folly::dynamic::array;
    for (const auto& val : vec) {
      dyn.push_back(folly::dynamic(val));
    }
    return dyn;
  }

  folly::dynamic operator() (const unordered_map_t& m) const {
    folly::dynamic dyn = folly::dynamic::object;
    for (const auto& kvp : m) {
      dyn[kvp.first] = boost::apply_visitor(*this, kvp.second);
    }
    return dyn;
  }

  folly::dynamic operator() (const ordered_map_t& m) const {
    folly::dynamic dyn = folly::dynamic::object;
    for (const auto& kvp : m) {
      dynamic key = kvp.first;
      std::string keyString = key.toJson();
      if (key.is_of<std::string>() || key.is_of<folly::StringPiece>()) {
        keyString = keyString.substr(1, keyString.size() - 2);
      }
      dyn[keyString] = boost::apply_visitor(*this, kvp.second);
    }
    return dyn;
  }

  folly::dynamic operator() (const vector_pair_t& pair) const {
    // This is a map with keys in pair.first and values in pair.second
    if (pair.first == nullptr) {
      throw std::logic_error("Malformed dynamic, key vector was null");
    }
    folly::dynamic dyn = folly::dynamic::object;
    for (int i = 0; i < pair.second.size(); ++i) {
      dyn[(*pair.first)[i]] = boost::apply_visitor(*this, pair.second[i]);
    }
    return dyn;
  }
};

inline folly::dynamic toFollyDynamic(const dynamic& v) {
  return boost::apply_visitor(FollyDynamicConverter(), v);
}

inline dynamic::dynamic(const folly::dynamic f) {
  switch(f.type()) {
  case f.NULLT:
    *this = dynamic();
    break;
  case f.BOOL:
    *this = f.asBool();
    break;
  case f.INT64:
    *this = f.asInt();
    break;
  case f.STRING:
    *this = f.asString();
    break;
  case f.DOUBLE:
    *this = f.asDouble();
    break;
  case f.ARRAY: {
      vector_dynamic_t vec;
      vec.reserve(f.size());
      for (const auto& v : f) {
        vec.push_back(dynamic(v));
      }
      *this = vec;
    }
    break;
  case f.OBJECT: {
      unordered_map_t obj;
      for (auto& i : f.items()) {
        const dynamic& d = dynamic(i.second);
        obj.insert({i.first.asString(), d});
      }
      *this = obj;
    }
    break;
  default:
    throw std::logic_error("Unknown conversion");
  }
}

}}
