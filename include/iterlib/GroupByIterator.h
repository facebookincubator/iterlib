// Copyright 2015 - present, Facebook

#pragma once

#include "iterlib/WrappedIterator.h"

namespace iterlib {

template <typename T=Item>
class GroupByIterator : public WrappedIterator<T> {
 public:
  explicit GroupByIterator(Iterator<T>* iter, AttributeNameVec groupByAttributes)
      : WrappedIterator<T>(iter), groupByAttributes_(variant::vector_dynamic_t()) {
    auto& vec = groupByAttributes_.template getNonConstRef<variant::vector_dynamic_t>();
    vec.insert(vec.begin(), std::make_move_iterator(groupByAttributes.begin()),
               std::make_move_iterator(groupByAttributes.end()));
  }

  virtual const T& key() const override { return iter_->first; }

  // TODO: Incompatible with value() in the parent class. Consider making
  // vector<T *> an Item for composability.
  const std::vector<const T*>& valueGroup() const {
    return results_.at(iter_->first);
  }

  const T& value() const override { return Item::kEmptyItem; }

 protected:
  // Runs the actual group by algorithm and fill results_ attribute
  void groupBy();

  // On first call to doNext() it will run the groupby algorithm.
  bool doNext() override final;

 private:
  // Flag used for lazy computing groupby results. If true, then results_ is
  // valid.
  bool resultsGroupedBy = false;
  // Attributes the iterator is grouping by
  T groupByAttributes_;

  // Results of groupBy()
  using MapType = std::map<T, std::vector<const T*>>;
  MapType results_;
  typename MapType::iterator iter_;
};

// Similar to GroupByIterator, but returns counts instead of vector<Item *>
// Expects input to be sorted by groupByAttributes
template <typename T=Item>
class GroupBySortedCountIterator : public WrappedIterator<T> {
 public:
  explicit GroupBySortedCountIterator(Iterator<T>* iter,
                                      AttributeNameVec groupByAttributes)
      : WrappedIterator<T>(iter), groupByAttributes_(variant::vector_dynamic_t()) {
    auto& vec = groupByAttributes_.template getNonConstRef<variant::vector_dynamic_t>();
    vec.insert(vec.begin(), std::make_move_iterator(groupByAttributes.begin()),
               std::make_move_iterator(groupByAttributes.end()));
  }

  virtual const T& key() const override { return iter_->first; }

  const T& value() const override { return iter_->second; }

 protected:
  // Runs the actual group by algorithm and fill results_ attribute
  void groupBy();

  // On first call to doNext() it will run the groupby algorithm.
  bool doNext() override final;

 private:
  // Flag used for lazy computing groupby results. If true, then results_ is
  // valid.
  bool resultsGroupedBy = false;
  // Attributes the iterator is grouping by
  T groupByAttributes_;

  // Results of groupBy()
  using MapType = std::map<T, T, std::greater<T>>;
  MapType results_;
  typename MapType::iterator iter_;
};
}
