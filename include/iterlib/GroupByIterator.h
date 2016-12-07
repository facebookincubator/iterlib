// Copyright 2015 - present, Facebook

#pragma once

#include "iterlib/WrappedIterator.h"

namespace iterlib {

class GroupByIterator : public WrappedIterator {
 public:
  explicit GroupByIterator(Iterator* iter, AttributeNameVec groupByAttributes)
      : WrappedIterator(iter), groupByAttributes_(variant::vector_dynamic_t()) {
    auto& vec = groupByAttributes_.getNonConstRef<variant::vector_dynamic_t>();
    vec.insert(vec.begin(), std::make_move_iterator(groupByAttributes.begin()),
               std::make_move_iterator(groupByAttributes.end()));
  }

  virtual const Item& key() const override { return iter_->first; }

  // TODO: Incompatible with value() in the parent class. Consider making
  // vector<Item *> an Item for composability.
  const std::vector<const Item*>& valueGroup() const {
    return results_.at(iter_->first);
  }

  const Item& value() const override { return Item::kEmptyItem; }

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
  Item groupByAttributes_;

  // Results of groupBy()
  using MapType = std::map<Item, std::vector<const Item*>>;
  MapType results_;
  MapType::iterator iter_;
};

// Similar to GroupByIterator, but returns counts instead of vector<Item *>
// Expects input to be sorted by groupByAttributes
class GroupBySortedCountIterator : public WrappedIterator {
 public:
  explicit GroupBySortedCountIterator(Iterator* iter,
                                      AttributeNameVec groupByAttributes)
      : WrappedIterator(iter), groupByAttributes_(variant::vector_dynamic_t()) {
    auto& vec = groupByAttributes_.getNonConstRef<variant::vector_dynamic_t>();
    vec.insert(vec.begin(), std::make_move_iterator(groupByAttributes.begin()),
               std::make_move_iterator(groupByAttributes.end()));
  }

  virtual const Item& key() const override { return iter_->first; }

  const Item& value() const override { return iter_->second; }

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
  Item groupByAttributes_;

  // Results of groupBy()
  using MapType = std::map<Item, Item, std::greater<Item>>;
  MapType results_;
  MapType::iterator iter_;
};
}
