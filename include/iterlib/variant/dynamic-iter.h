//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
//
// Iterator for dynamic that provides underlying type agnostic iteration
// In case of vector-like types, it returns index as a key.
// (Note that it returns you copy of a key,
// but a reference_wrapper to the actual value).
// Keep in sync with isIterable() function!
#pragma once

namespace iterlib { namespace variant {

typedef std::pair<dynamic,
                  std::reference_wrapper<const dynamic>> DynIterValue;
class dynamic_iterator
    : public std::iterator<std::forward_iterator_tag,
                           DynIterValue> {
  friend class dynamic;
  static const dynamic kNullDynamic;

 public:
  enum ItType { UNKNOWN, VECTOR_PAIR, UNORDERED_MAP,
                ORDERED_MAP, VECTOR_DYNAMIC};

  // The end() iterator
  dynamic_iterator() : index_(0), values_(nullptr), max_(0), type_(UNKNOWN),
                       current_(kNullDynamic, kNullDynamic) {}

  // The begin iterator is initialized based on the underlying type
  explicit dynamic_iterator(const dynamic* value) :
      index_(0),
      current_(kNullDynamic, kNullDynamic) {
    values_ = value;
    if (values_ == nullptr) {
      max_ = 0;
      type_ = UNKNOWN;
      return;
    }
    if (values_->is_of<vector_pair_t>()) {
      type_ = VECTOR_PAIR;
      max_ = values_ && values_->getRef<vector_pair_t>().first
                 ? values_->getRef<vector_pair_t>().first->size()
                 : 0;
      if (max_ == 0) {
        values_ = nullptr;
        return;
      }
      auto& pair = values_->getRef<vector_pair_t>();
      current_.first = pair.first->at(index_);
      current_.second = std::ref(pair.second[index_]);
    } else if (values_->is_of<unordered_map_t>()) {
      type_ = UNORDERED_MAP;
      mIter_ = values_->getRef<unordered_map_t>().begin();
      if (mIter_ == values_->getRef<unordered_map_t>().end()) {
        values_ = nullptr;
        return;
      }
      current_.first = (*mIter_).first;
      current_.second = std::ref((*mIter_).second);
    } else if (values_->is_of<ordered_map_t>()) {
      type_ = ORDERED_MAP;
      omIter_ = values_->getRef<ordered_map_t>().begin();
      if (omIter_ == values_->getRef<ordered_map_t>().end()) {
        values_ = nullptr;
        return;
      }
      current_.first = (*omIter_).first;
      current_.second = std::ref((*omIter_).second);
    } else if (values_->is_of<vector_dynamic_t>()) {
      type_ = VECTOR_DYNAMIC;
      max_ = values_->getRef<vector_dynamic_t>().size();
      if(max_ == 0) {
        values_ = nullptr;
        return;
      }
      current_.second = std::ref(values_->getRef<vector_dynamic_t>()[index_]);
    } else {
      throw std::logic_error(folly::stringPrintf(
          "Iteration not supported for dynamic type: %d", values_->which()));
    }
  }

  // Iterator increments differently based on the underlying type
  // When all items are done, the iterator state will match that of
  // dynamic_iterator()
  const dynamic_iterator& operator++() {
    if (!values_) {
      throw std::out_of_range("Cannot increment past end");
    }
    index_++;
    switch (type_) {
    case VECTOR_PAIR: {
      if (index_ >= max_) {
        values_ = nullptr;
        index_ = 0;
        return *this;
      }
      auto& pair = values_->getRef<vector_pair_t>();
      current_.first = pair.first->at(index_);
      current_.second = std::ref(pair.second[index_]);
      break;
    }
    case UNORDERED_MAP: {
      ++mIter_;
      if (mIter_ == values_->getRef<unordered_map_t>().end()) {
        values_ = nullptr;
        index_ = 0;
      } else {
        current_.first = (*mIter_).first;
        current_.second = std::ref((*mIter_).second);
      }
      break;
    }
    case ORDERED_MAP: {
      ++omIter_;
      if (omIter_ == values_->getRef<ordered_map_t>().end()) {
        values_ = nullptr;
        index_ = 0;
      } else {
        current_.first = (*omIter_).first;
        current_.second = std::ref((*omIter_).second);
      }
      break;
    }
    case VECTOR_DYNAMIC: {
      if (index_ >= max_) {
        values_ = nullptr;
        index_ = 0;
        return *this;
      }
      current_.second = std::ref(values_->getRef<vector_dynamic_t>()[index_]);
      break;
    }
    default: {
      const auto& what = folly::stringPrintf(
          "Iteration not supported for dynamic type: %d", values_->which());
      throw std::logic_error(what);
    }
    } // Switch
    return *this;
  }

  // Iterators will be considered equal when they point to the same dynamic
  // and are at the same index of iteration
  bool operator==(dynamic_iterator const& other) const {
    return ((values_ == other.values_) && (index_ == other.index_));
  }

  bool operator!=(dynamic_iterator const& other) const {
    return !(*this == other);
  }

  const dynamic& getKey() {
    return static_cast<const dynamic&>(current_.first);
  }

  const dynamic& getValue() {
    return static_cast<const dynamic&>(current_.second.get());
  }

  // dereferencing the iterator returns you your own copy
  // of DynIterValue - copy of the key, and reference_wrapper of the value
  DynIterValue operator*() const {
    if (values_) {
      switch (type_) {
        case VECTOR_PAIR:
          return DynIterValue(
            values_->getRef<vector_pair_t>().first->at(index_),
            current_.second
          );
        case UNORDERED_MAP:
          return DynIterValue((*mIter_).first,
                              current_.second);
        case ORDERED_MAP:
          return DynIterValue((*omIter_).first,
                              current_.second);
        case VECTOR_DYNAMIC:
          return DynIterValue(static_cast<int64_t>(index_),
                              current_.second);
        default: {
          const auto& what = folly::stringPrintf(
              "Iteration not supported for dynamic type: %d", values_->which());
          throw std::logic_error(what);
        }
      }
    } else {
      throw std::out_of_range("Invalid dereference");
    }
  }

  DynIterValue* operator->() {
    current_ = operator*();
    return &current_;
  }

  const DynIterValue* operator->() const {
    current_ = operator*();
    return &current_;
  }
 private:
  uint64_t index_;
  const dynamic* values_;
  uint64_t max_;
  unordered_map_t::const_iterator mIter_;
  ordered_map_t::const_iterator omIter_;
  ItType type_;
  mutable DynIterValue current_;
};

}}
