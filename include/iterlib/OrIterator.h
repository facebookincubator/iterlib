#pragma once
#include "iterlib/Iterator.h"

namespace iterlib {

class OrIteratorBase : public CompositeIterator {
 public:
  explicit OrIteratorBase(IteratorVector& children);
  void updateActiveChildren();

  virtual const Item& value() const override {
    if (!done() && activeChildren_.front()) {
      return activeChildren_.front()->value();
    }
    return Item::kEmptyItem;
  }

 protected:
  // unmanaged pointers. Memory is managed via
  // children_ in the parent class
  std::vector<Iterator *> activeChildren_;
};

// Assuming that input iterators are sorted, performs
// a merge sort to compute a new iterator. If you
// don't care about ordering, consider using the
// ConcatIterator below. If input is not sorted,
// consider making them sorted via OrderbyIterator
template <class Comparator>
class OrIterator: public OrIteratorBase {
public:
  explicit OrIterator(IteratorVector& children);

protected:
  bool doNext() override;
  bool doSkipTo(id_t id) override;

  void doFirst();
  bool firstTime_;
};

// OrIterator that dedups by id() if requested
// Output order is undefined. Typically performs
// a simple concatenation of child iterators
class ConcatIterator: public OrIteratorBase {
public:
  explicit ConcatIterator(IteratorVector& children, bool dedup=true);

  virtual const Item& value() const override {
    if (!done()) {
      return activeChildren_[idx_]->value();
    }
    return Item::kEmptyItem;
  }

  virtual folly::Future<folly::Unit> prepare() override;

protected:
  Iterator* currentChild() {
    return done() ? nullptr : activeChildren_[idx_];
  }

  bool doNext() override;
  bool doSkipTo(id_t id) override { return false; }

  bool isDuplicate(id_t id) {
    if (!dedup_) {
      return false;
    }

    return results_.find(id) != results_.end();
  }

private:
  // a set of returned ids so far
  std::unordered_set<id_t> results_;
  size_t idx_;
  bool dedup_;
};

using UnionIterator = OrIterator<IdLessComp>;
using SortedMergeIterator = OrIterator<StdLessComp>;

}
