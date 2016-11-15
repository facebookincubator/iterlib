//  Copyright (c) 2012, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
//
// This file contains an Iterator interface to return simple
// query results.
//
// Query results contain an id, timestamp, optional attributes
//
// Attributes could be fbobject attributes or structured assoc
// attributes as defined by fbschema.
//
//
// Sample usage:
// auto it = ConstructIteratorTree()
//
// Sync version to access the iterator:
//
// it->prepare().waitVia(folly::EventBaseManager::get()->getEventBase());
// while(it->next()) {
//   auto& key = it->key();
//   auto& value = it->value();
//   // Do something with key, value
// }
//
// Async version:
// it->prepare().then(...);
//
// It is fine to prepare() iterators from multiple threads as long as each
// iterator's prepare() is called once. If an iterator contains multiple
// children, it is fine to call each child's prepare() in different threads
// given proper synchronization is implemented in parent iter's prepare(). In
// a word, don't call prepare() on an iterator multiple times.
#pragma once

#include <folly/futures/Future.h>
#include <limits>
#include <memory>
#include <vector>

#include "iterlib/Item.h"

namespace iterlib {

enum class IteratorType : int32_t {
  NONE = 0,
  ROCKSDB,
  COMPOSITE,
  WRAPPED,
  LITERAL,
  FUTURE,
  ORDERBY,
  BINARY,
};

enum class ResultOrder {
  DEFAULT = 0, // Index order
  INDEX_ORDER = 0,
};

typedef std::vector<std::string> AttributeNameVec;
class Iterator;
typedef std::vector<std::unique_ptr<Iterator>> IteratorVector;

// These methods are useful to avoid sorting an already sorted
// list or fetching the same data twice instead of rewinding
// an existing iterator.
//
// Since it's a separate concern from core iterator interface,
// these methods exist in a class of their own.
class IteratorTraits {
 public:
  IteratorTraits() : order_(ResultOrder::INDEX_ORDER) {}
  virtual ~IteratorTraits() {}

  // A return value of true means the following assertions are true
  //
  // a) The iterator has a bounded number of entries, that are accessible
  //    cheaply.
  // b) The iterator can be rewound after reading the end.
  virtual bool cacheable() const { return false; }

  virtual ResultOrder order() const { return order_; }

  void setOrder(ResultOrder o) { order_ = o; }

  // Number of times Iterator::next() could be called without
  // expensive blocking (eg: fetching data from another service).
  //
  // -1 means non-blocking. Iterators that fetch everything in
  // prepare() must return -1.
  //
  // Iterators that only fetch a page worth of data can return non
  // zero to hint to higher level iterators for optimization purposes.
  virtual ssize_t numBuffered() const { return -1; }

 protected:
  // Order guaranteed by the iterator. May not be same as the underlying
  // index
  ResultOrder order_;
};

class Iterator : public IteratorTraits {
 public:
  explicit Iterator(IteratorType type = IteratorType::NONE);
  Iterator(const Iterator&) = delete;
  Iterator& operator=(const Iterator&) = delete;

  Iterator(Iterator&&) = default;
  Iterator& operator=(Iterator&&) = default;

  virtual ~Iterator() {}

  static id_t max() { return std::numeric_limits<id_t>::max(); }

  static id_t min() { return std::numeric_limits<id_t>::min(); }

  bool next();

  virtual const Item& key() const { return key_; }

  virtual const Item& value() const = 0;

  virtual bool done() const { return isDone_; }

  virtual folly::Future<folly::Unit> prepare() {
    prepared_ = true;
    return folly::makeFuture();
  }

  bool prepared() const { return prepared_; }

  bool skipTo(id_t id) {
    auto ret = doSkipTo(id);
    advancedAtleastOnce_ = true;
    return ret;
  }

  bool advancedAtleastOnce() const { return advancedAtleastOnce_; }

  /**
   * Progresses the pointer in the iterator to a value which is at or
   * just beyond the given value target
   *
   * @param predicate :   the list of column names that needs to be compared
   *                      based on.
   * @param target :  the value that we want to reach.
   *
   **/
  bool skipToPredicate(AttributeNameVec predicate, const Item& target) {
    auto ret = doSkipToPredicate(predicate, target);
    advancedAtleastOnce_ = true;
    return ret;
  }

  // skip n element in the iterator
  bool skip(size_t n) {
    auto ret = doSkip(n);
    advancedAtleastOnce_ = true;
    return ret;
  }

  virtual IteratorType getType() const { return iteratorType_; }

  virtual std::string cookie() const {
    return folly::stringPrintf("%ld,%lu", value().ts(), value().id());
  }

  virtual void reset() { isDone_ = false; }

  virtual const IteratorVector& children() const {
    static const IteratorVector kEmptyVec;
    return kEmptyVec;
  }

 protected:
  virtual bool doNext() = 0;
  virtual bool doSkipTo(id_t id);
  virtual bool doSkipToPredicate(AttributeNameVec predicate,
                                 const Item& target);
  virtual bool doSkip(size_t n);

  void setDone() { isDone_ = true; }

  void throwIfUnPrepared() const {
    if (UNLIKELY(!prepared_)) {
      throw std::logic_error("called without calling prepare() first");
    }
  }

  void throwIfNotAdvancedAtLeastOnce() const {
    throwIfUnPrepared();
    if (UNLIKELY(!advancedAtleastOnce())) {
      throw std::logic_error("called without calling next() first:");
    }
  }

  bool isDone_; // a flag to check if the iterator is done
  Item key_;

  bool advancedAtleastOnce_;

  // whether this iterator has been prepared. Only after the iterator has
  // been prepared, we can call next() and access all the getters.
  bool prepared_;

 private:
  IteratorType iteratorType_;
};
}
