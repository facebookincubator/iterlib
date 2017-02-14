//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

namespace iterlib {

template <typename T>
folly::Future<folly::Unit> WrappedIterator<T>::prepare() {
  if (this->prepared_) {
    return folly::makeFuture();
  }
  if (innerIter_) {
    return innerIter_->prepare()
        .then([this]() {
          // Code to extract any data from inner iter
          // Hopefully none in a zero-copy implementation
        })
        .onError([](const std::exception& ex) {
          LOG(ERROR) << "Failed to prepare WrappedIterator's innerIter "
                     << ex.what();
          throw ex;
        })
        .ensure([this]() { this->prepared_ = true; });
  }
  this->prepared_ = true;
  return folly::makeFuture();
}

}
