#include "iterlib/GroupByIterator.h"

namespace iterlib {

void GroupByIterator::groupBy() {

  while (innerIter_->next()) {
    const auto& v = innerIter_->value();
    auto key = std::vector<dynamic>{};
    for (const auto& attr : groupByAttributes_) {
      key.push_back(v.at(attr.second.get()));
    }
    Item itemKey{dynamic(std::move(key))};
    results_[itemKey].emplace_back(&v);
  }
  iter_ = results_.begin();
}

// On first call to doNext() it will run the groupby algorithm.
bool GroupByIterator::doNext() {
  if (done()) {
    return false;
  }
  if (!resultsGroupedBy) {
    groupBy();
    resultsGroupedBy = true;
  } else {
    iter_++;
  }
  if (iter_ == results_.end()) {
    setDone();
    return false;
  }
  return true;
}

void GroupBySortedCountIterator::groupBy() {
  while (innerIter_->next()) {
    const auto& v = innerIter_->value();
    auto key = std::vector<dynamic>{};
    for (const auto& attr : groupByAttributes_) {
      key.push_back(v.at(attr.second.get()));
    }
    Item itemKey{dynamic(std::move(key))};
    if (results_.find(itemKey) != results_.end()) {
      auto& count = results_[itemKey].getNonConstRef<int64_t>();
      count++;
    } else {
      results_[itemKey] = Item{{1}};
    }
  }
  iter_ = results_.begin();
}

// On first call to doNext() it will run the groupby algorithm.
bool GroupBySortedCountIterator::doNext() {
  if (done()) {
    return false;
  }
  if (!resultsGroupedBy) {
    groupBy();
    resultsGroupedBy = true;
  } else {
    iter_++;
  }
  if (iter_ == results_.end()) {
    setDone();
    return false;
  }
  return true;
}
}
