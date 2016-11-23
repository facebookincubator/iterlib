#pragma once

#include "iterlib/WrappedIterator.h"

namespace iterlib {

// reverse the order of the WrappedIterator
class ReverseIterator : public WrappedIterator {
public:
  explicit ReverseIterator(Iterator* iter)
    : WrappedIterator(iter)
    , firstTime_(true){
  }

  virtual const Item& value() const override {
    return results_.back();
  }

protected:
 bool doNext() override;

private:
  // first time load all data into memory
  void load();

  bool firstTime_;

  std::vector<Item> results_;
};

}
