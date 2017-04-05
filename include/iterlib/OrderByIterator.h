// Copyright 2015 - present, Facebook

#pragma once

#include "iterlib/WrappedIterator.h"

namespace iterlib {
namespace detail {

template <typename T=Item>
class OrderByIterator : public WrappedIterator<T> {
 public:
  OrderByIterator(Iterator<T>* iter, AttributeNameVec orderByColumns,
                  std::vector<bool> isColumnDescending)
      : WrappedIterator<T>(iter), orderByColumns_(std::move(orderByColumns)),
        isColumnDescending_(std::move(isColumnDescending)), first_(true) {
    CHECK_EQ(orderByColumns_.size(), isColumnDescending_.size());
    if ((this->innerIter_ == nullptr) || (this->innerIter_->done())) {
      this->prepared_ = true;
      this->setDone();
      return;
    }
  }

  OrderByIterator(Iterator<T>* iter, AttributeNameVec orderByColumns)
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
    bool operator()(const std::pair<const T*, int>& v1,
                    const std::pair<const T*, int>& v2) const;
  };

  virtual const T& value() const override {
    if (!results_.empty()) {
      return *results_.front().first;
    } else {
      return Item::kEmptyItem;
    }
  }

  bool doNext() override {
    if (this->done()) {
      return false;
    }

    if (first_) {
      this->load();
      first_ = false;
    } else {
      std::pop_heap(results_.begin(), results_.end(),
                    Comparator(orderByColumns_, isColumnDescending_));
      results_.pop_back();
    }

    if (results_.empty()) {
      this->setDone();
      return false;
    }

    return true;
  }

  const AttributeNameVec& orderByColumns() const { return orderByColumns_; }

  const std::vector<bool>& isDescending() const { return isColumnDescending_; }

 protected:
  void load() {
    int sequenceNum = 0;
    while (this->innerIter_->next()) {
      results_.push_back(std::make_pair(&(this->innerIter_->value()), sequenceNum));
      ++sequenceNum;
    }

    std::make_heap(results_.begin(), results_.end(),
                   Comparator(orderByColumns_, isColumnDescending_));
  }

 private:
  std::vector<std::pair<const T*, int>> results_;
  AttributeNameVec orderByColumns_;
  std::vector<bool> isColumnDescending_;
  bool first_;
};

}

using OrderByIterator = detail::OrderByIterator<Item>;
extern template class detail::OrderByIterator<Item>;

}
#include "iterlib/OrderByIterator-inl.h"
