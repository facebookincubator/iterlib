#include "iterlib/GroupByIterator.h"

namespace iterlib {

void GroupByIterator::groupBy() {

  while (innerIter_->next()) {
    const auto& v = innerIter_->value();
    auto key = std::vector<dynamic>{};
    for (const auto& attr : groupByAttributes_) {
      key.push_back(v.at(attr.second.get()));
    }
    Item itemKey_{dynamic(std::move(key))};
    results_[itemKey_].emplace_back(&v);
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
}
