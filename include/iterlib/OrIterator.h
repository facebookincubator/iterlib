#pragma once
#include "iterlib/Iterator.h"

namespace iterlib {

template <typename T=Item>
class OrIteratorBase : public CompositeIterator<T> {
 public:
  explicit OrIteratorBase(IteratorVector<T>& children);
  void updateActiveChildren();

  virtual const T& value() const override {
    if (!this->done() && activeChildren_.front()) {
      return activeChildren_.front()->value();
    }
    return Item::kEmptyItem;
  }

 protected:
  // unmanaged pointers. Memory is managed via
  // children_ in the parent class
  std::vector<Iterator<T> *> activeChildren_;
};

// Assuming that input iterators are sorted, performs
// a merge sort to compute a new iterator. If you
// don't care about ordering, consider using the
// ConcatIterator below. If input is not sorted,
// consider making them sorted via OrderbyIterator
template <class Comparator, typename T=Item>
class OrIterator: public OrIteratorBase<T> {
public:
  explicit OrIterator(IteratorVector<T>& children);

protected:
  bool doNext() override;
  bool doSkipTo(id_t id) override;

  void doFirst();
  bool firstTime_;

  using OrIteratorBase<T>::activeChildren_;
  using Iterator<T>::id;
  using Iterator<T>::max;
};

// OrIterator that dedups by id() if requested
// Output order is undefined. Typically performs
// a simple concatenation of child iterators
template <typename T=Item>
class ConcatIterator: public OrIteratorBase<T> {
public:
  explicit ConcatIterator(IteratorVector<T>& children, bool dedup=true);

  virtual const T& value() const override {
    if (!this->done()) {
      return this->activeChildren_[idx_]->value();
    }
    return Item::kEmptyItem;
  }

  virtual folly::Future<folly::Unit> prepare() override;

protected:
  Iterator<T>* currentChild() {
    return this->done() ? nullptr : this->activeChildren_[idx_];
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
  using OrIteratorBase<T>::activeChildren_;

  // a set of returned ids so far
  std::unordered_set<id_t> results_;
  size_t idx_;
  bool dedup_;
};

template <typename T=Item>
using UnionIterator = OrIterator<IdLessComp<T>, T>;
template <typename T=Item>
using SortedMergeIterator = OrIterator<StdLessComp<T>, T>;

}

#include "iterlib/OrIterator-inl.h"
