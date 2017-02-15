// Copyright 2015 - present, Facebook

#include "iterlib/Item.h"

namespace iterlib {

namespace {
const dynamic kEmptyItemInternal{variant::unordered_map_t{
    {{":id", int64_t(std::numeric_limits<id_t>::max())}, {":time", 0}}}};
}

const Item Item::kEmptyItem{kEmptyItemInternal};
const ItemOptimized ItemOptimized::kEmptyItem{
  std::numeric_limits<id_t>::max(), 0, kEmptyItemInternal};

// Returns the result of v1[i] < v2[i]
// The comparison is inverted if isColumnDescending[i] is false
PartialOrder partialCompare(const Item& v1, const Item& v2,
                            const std::vector<std::string>& columns,
                            const std::vector<bool>& isColumnDescending) {
  // INDEX_ORDER
  if (columns.size() == 0) {
    if (v1 == v2) {
      return PartialOrder::EQ;
    }
    try {
      return (v1 < v2) ? PartialOrder::LT : PartialOrder::GT;
    } catch (std::exception& ex) {
      LOG_EVERY_N(WARNING, 1000) << ex.what();
      return PartialOrder::NONE;
    }
  }
  const auto& m1 = v1.value();
  const auto& m2 = v2.value();
  bool comparable = true;

  CHECK_EQ(columns.size(), isColumnDescending.size());

  int index = 0;
  for (const auto& attrName : columns) {
    bool isDescending = isColumnDescending[index++];

    bool attrLess = false;
    bool attrEqual = true;

    if (attrName == kTimeKey) {
      attrEqual = (v1.ts() == v2.ts());
      attrLess = attrEqual ? false : (isDescending ? (v1.ts() < v2.ts())
                                                   : (v1.ts() > v2.ts()));
    } else if (attrName == kIdKey) {
      attrEqual = (v1.id() == v2.id());
      attrLess = attrEqual ? false : (isDescending ? (v1.id() < v2.id())
                                                   : (v1.id() > v2.id()));
    } else {
      const auto& attr1 = m1.atNoThrow(attrName);
      const auto& attr2 = m2.atNoThrow(attrName);

      try {
        attrEqual = (attr1 == attr2);
        attrLess = attrEqual ? false : (isDescending ? (attr1 < attr2)
                                                     : (attr1 > attr2));
      } catch (const std::exception& ex) {
        // == does not throw, but < does, protecting against such cases
        attrEqual = true;
        attrLess = false;

        comparable = false;
        LOG_EVERY_N(WARNING, 1000) << ex.what();
      }
    }

    if (!attrEqual) {
      return attrLess ? PartialOrder::LT : PartialOrder::GT;
    }
  }

  if (comparable) {
    return PartialOrder::EQ;
  }

  return PartialOrder::NONE;
}

PartialOrder partialCompare(const Item& v1, const Item& v2,
                            const std::vector<std::string>& columns) {
  std::vector<bool> descOrdering(columns.size(), true);
  return partialCompare(v1, v2, columns, descOrdering);
}
}
