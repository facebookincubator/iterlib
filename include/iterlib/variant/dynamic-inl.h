//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
#pragma once

namespace iterlib { namespace variant {

template <typename T>
inline const dynamic& dynamic::getItemRef(const dynamic& key) const {
  const T& container = this->getRef<T>();
  if (key.is_of<std::string>()) {
    return static_cast<const dynamic&>(container.at(key.getRef<std::string>()));
  } else if (key.is_of<folly::StringPiece>()) {
    return static_cast<const dynamic&>(
        container.at(key.get<folly::StringPiece>().toString()));
  } else {
    throw std::out_of_range("Key not found");
  }
}

template <>
inline const dynamic& dynamic::getItemRef<ordered_map_t>(
    const dynamic& key) const {
  const ordered_map_t& container = this->getRef<ordered_map_t>();
  return container.at(key);
}

template <typename T>
inline dynamic& dynamic::getNonConstItemRef(const dynamic& key) {
  T& container = this->getNonConstRef<T>();
  if (key.is_of<std::string>()) {
    return static_cast<dynamic&>(container.at(key.getRef<std::string>()));
  } else if (key.is_of<folly::StringPiece>()) {
    return static_cast<dynamic&>(
        container.at(key.get<folly::StringPiece>().toString()));
  } else {
    throw std::out_of_range("Key not found");
 }
}

template <>
inline dynamic& dynamic::getNonConstItemRef<ordered_map_t>(const dynamic& key) {
  ordered_map_t& container = this->getNonConstRef<ordered_map_t>();
  return static_cast<dynamic&>(container.at(key));
}

template <typename T>
inline dynamic& dynamic::getNonConstItemRefWithInsert(const dynamic& key) {
  T& container = this->getNonConstRef<T>();
  if (key.is_of<std::string>()) {
    return static_cast<dynamic&>(container[key.getRef<std::string>()]);
  } else if (key.is_of<folly::StringPiece>()) {
    return static_cast<dynamic&>(
        container[key.get<folly::StringPiece>().toString()]);
  } else {
    throw std::logic_error(
      "Accessing nonexistentvalues through non-string keys");
 }
}

template <>
inline dynamic& dynamic::getNonConstItemRefWithInsert<ordered_map_t>(
    const dynamic& key) {
  ordered_map_t& container = this->getNonConstRef<ordered_map_t>();
  return static_cast<dynamic&>(container[key]);
}

inline bool dynamic::empty() const {
  return detail::dynamic_empty()(*this);
}

inline size_t dynamic::length() const {
  return detail::dynamic_length()(*this);
}

inline const dynamic& dynamic::at(const dynamic& key) const {
  if (this->is_of<unordered_map_t>()) {
    return this->getItemRef<unordered_map_t>(key);
  } else if (this->is_of<ordered_map_t>()) {
    return this->getItemRef<ordered_map_t>(key);
  } else if (this->is_of<vector_pair_t>()) {
    // Assumed to be lazy/optimized map.
    const vector_pair_t& pair = this->getRef<vector_pair_t>();
    int pos = detail::vector_pair_index()(pair, key);
    if (pair.first->size() != pair.second.size()) {
      throw std::logic_error("Malformed vector pair, key vector is null");
    }
    return static_cast<const dynamic&>(pair.second[pos]);
  } else {
    throw std::logic_error(
        folly::stringPrintf("Operation not supported for dynamic type: %d",
                            this->which()));
  }
}

inline dynamic& dynamic::at(const dynamic& key) {
  if (this->is_of<unordered_map_t>()) {
    return this->getNonConstItemRef<unordered_map_t>(key);
  } else if (this->is_of<ordered_map_t>()) {
    return this->getNonConstItemRef<ordered_map_t>(key);
  } else if (this->is_of<vector_pair_t>()) {
    // Assumed to be lazy/optimized map.
    vector_pair_t& pair = this->getNonConstRef<vector_pair_t>();
    int pos = detail::vector_pair_index()(pair, key);
    if (pair.first->size() != pair.second.size()) {
      throw std::logic_error("Malformed vector pair, key vector is null");
    }
    return static_cast<dynamic&>(pair.second[pos]);
  } else {
    throw std::logic_error(folly::stringPrintf(
        "Operation not supported for dynamic type: %d", this->which()));
  }
}

inline dynamic& dynamic::atWithInsert(const dynamic& key) {
  if (this->is_of<unordered_map_t>()) {
    return this->getNonConstItemRefWithInsert<unordered_map_t>(key);
  } else if (this->is_of<ordered_map_t>()) {
    return this->getNonConstItemRefWithInsert<ordered_map_t>(key);
  } else if (this->is_of<vector_pair_t>()) {
    // Assumed to be lazy/optimized map.
    vector_pair_t& pair = this->getNonConstRef<vector_pair_t>();
    int pos;
    try {
      pos = detail::vector_pair_index()(pair, key);
    } catch(const std::out_of_range& ex) {
      throw std::logic_error(folly::stringPrintf(
        "Inserting through nonexistent key not supported for dynamic type: %d",
        this->which()));
    }
    if (pair.first->size() != pair.second.size()) {
      throw std::logic_error("Malformed vector pair, key vector is null");
    }
    return static_cast<dynamic&>(pair.second[pos]);
  } else {
    throw std::logic_error(folly::stringPrintf(
        "Operation not supported for dynamic type: %d", this->which()));
  }
}

}}
