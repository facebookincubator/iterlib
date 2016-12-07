// Copyright 2015 - present, Facebook

#pragma once

#include "iterlib/WrappedIterator.h"

namespace iterlib {

class OrderByIterator : public WrappedIterator {
 public:
  OrderByIterator(Iterator* iter, AttributeNameVec orderByColumns,
                  std::vector<bool> isColumnDescending)
      : WrappedIterator(iter), orderByColumns_(std::move(orderByColumns)),
        isColumnDescending_(std::move(isColumnDescending)), first_(true) {
    CHECK_EQ(orderByColumns_.size(), isColumnDescending_.size());
    if ((innerIter_ == nullptr) || (innerIter_->done())) {
      prepared_ = true;
      setDone();
      return;
    }
  }

  OrderByIterator(Iterator* iter, AttributeNameVec orderByColumns)
      : OrderByIterator(iter, orderByColumns,
                        std::vector<bool>(orderByColumns.size(), true)) {}

  virtual ~OrderByIterator() {}

  struct Comparator {
    Comparator(const AttributeNameVec& columns,
               const std::vector<bool>& isColumnDescending)
        : columns_(columns), isColumnDescending_(isColumnDescending) {}

    // columns_ and isColumnDescending are of the same size, and
    // isColumnDescending_[i] indicates if columns_[i] is in descending order
    const AttributeNameVec& columns_;
    const std::vector<bool>& isColumnDescending_;
    bool operator()(const std::pair<const Item*, int>& v1,
                    const std::pair<const Item*, int>& v2) const;
  };

  virtual const Item& value() const override {
    if (!results_.empty()) {
      return *results_.front().first;
    } else {
      return Item::kEmptyItem;
    }
  }

  bool doNext() override {
    if (done()) {
      return false;
    }

    if (first_) {
      load();
      first_ = false;
    } else {
      std::pop_heap(results_.begin(), results_.end(),
                    Comparator(orderByColumns_, isColumnDescending_));
      results_.pop_back();
    }

    if (results_.empty()) {
      setDone();
      return false;
    }

    return true;
  }

  const AttributeNameVec& orderByColumns() const { return orderByColumns_; }

  const std::vector<bool>& isDescending() const { return isColumnDescending_; }

 protected:
  void load() {
    int sequenceNum = 0;
    while (innerIter_->next()) {
      results_.push_back(std::make_pair(&(innerIter_->value()), sequenceNum));
      ++sequenceNum;
    }

    std::make_heap(results_.begin(), results_.end(),
                   Comparator(orderByColumns_, isColumnDescending_));
  }

 private:
  std::vector<std::pair<const Item*, int>> results_;
  AttributeNameVec orderByColumns_;
  std::vector<bool> isColumnDescending_;
  bool first_;
};
}
