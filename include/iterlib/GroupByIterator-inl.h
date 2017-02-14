//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

namespace iterlib {

template <typename T>
void GroupByIterator<T>::groupBy() {

  while (this->innerIter_->next()) {
    const auto& v = this->innerIter_->value();
    auto key = std::vector<dynamic>{};
    for (const auto& attr : groupByAttributes_) {
      key.push_back(v.at(attr.second.get()));
    }
    T itemKey{dynamic(std::move(key))};
    results_[itemKey].emplace_back(&v);
  }
  iter_ = results_.begin();
}

// On first call to doNext() it will run the groupby algorithm.
template <typename T>
bool GroupByIterator<T>::doNext() {
  if (this->done()) {
    return false;
  }
  if (!resultsGroupedBy) {
    groupBy();
    resultsGroupedBy = true;
  } else {
    iter_++;
  }
  if (iter_ == results_.end()) {
    this->setDone();
    return false;
  }
  return true;
}

template <typename T>
void GroupBySortedCountIterator<T>::groupBy() {
  while (this->innerIter_->next()) {
    const auto& v = this->innerIter_->value();
    auto key = std::vector<dynamic>{};
    for (const auto& attr : groupByAttributes_) {
      key.push_back(v.at(attr.second.get()));
    }
    Item itemKey{dynamic(std::move(key))};
    if (results_.find(itemKey) != results_.end()) {
      auto& count = results_[itemKey].template getNonConstRef<int64_t>();
      count++;
    } else {
      results_[itemKey] = Item{{1}};
    }
  }
  iter_ = results_.begin();
}

// On first call to doNext() it will run the groupby algorithm.
template <typename T>
bool GroupBySortedCountIterator<T>::doNext() {
  if (this->done()) {
    return false;
  }
  if (!resultsGroupedBy) {
    groupBy();
    resultsGroupedBy = true;
  } else {
    iter_++;
  }
  if (iter_ == results_.end()) {
    this->setDone();
    return false;
  }
  return true;
}
}
