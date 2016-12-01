//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
#pragma once

#include "iterlib/WrappedIterator.h"

namespace iterlib {

/**
 * Project takes an ordered set of attribute names (AttributeNameVec) and
 * returns only those attributes from the child iterator.
 *
 * The returned value() is always an ordered_map_t
 */
class ProjectIterator : public WrappedIterator {
public:
  ProjectIterator(Iterator* iter, const AttributeNameVec& attrNames)
    : WrappedIterator(iter)
    , attrNames_(attrNames) {}


  virtual const Item& value() const override;

  virtual bool orderPreserving() const override {
    return true;
  }

protected:
 bool doNext() override;

 bool doSkipTo(id_t id) override;

private:
  AttributeNameVec attrNames_;
  mutable ItemOptimized value_;
};

}
