//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
#pragma once

#include <cstddef>

#include "iterlib/WrappedIterator.h"

namespace iterlib {

// Collect count results starting at startOffset in iter
class LimitIterator : public WrappedIterator {
 public:
  LimitIterator(Iterator* iter, size_t count, size_t startOffset)
      : WrappedIterator(iter), count_(count), startOffset_(startOffset),
        firstTime_(true) {}

  std::string cookie() const override { return innerIter_->cookie(); }

  virtual bool orderPreserving() const override { return true; }

 protected:
  bool doNext() override;

 private:
  size_t count_;
  size_t startOffset_;

  bool firstTime_;
};
}
