#include "iterlib/FilterIterator.h"

namespace iterlib {

using variant::vector_dynamic_t;

bool FilterIteratorBase::doNext() {
  while (innerIter_->next()) {
    try {
      if (match(innerIter_.get())) {
        return true;
      }
    } catch (const std::exception& ex) {
      LOG_EVERY_N(WARNING, 1000) << "match failed on :id : " << innerIter_->id()
                                 << " " << ex.what();
    }
  }
  setDone();
  return false;
}

bool FilterIteratorBase::doSkipTo(id_t id) {
  if (!innerIter_->skipTo(id)) {
    setDone();
    return false;
  }
  if (match(innerIter_.get())) {
    return true;
  }

  while (innerIter_->next()) {
    if (match(innerIter_.get())) {
      return true;
    }
  }

  setDone();
  return false;
}

bool FilterIterator::match(const Iterator* iter) {
  dynamic v;
  auto& values = reinterpret_cast<std::vector<dynamic>&>(
      values_.getNonConstRef<vector_dynamic_t>());
  if (fields_.is_of<std::string>()) {
    if (fields_ == dynamic(kIdKey)) {
      v = static_cast<int64_t>(iter->id());
    } else if (fields_ == dynamic(kTimeKey)) {
      v = iter->value().ts();
    } else {
      v = iter->value().atNoThrow(fields_);
      if (firstTime_ && !v.empty()) {
        // Try to learn the type of the attribute
        firstTime_ = false;
        for (auto& val : values) {
          val.castTo(v);
        }
      }
    }
  } else {
    auto vec = vector_dynamic_t();
    for (const auto& field : fields_) {
      if (dynamic(field.second.get()) == kIdKey) {
        vec.emplace_back(static_cast<int64_t>(iter->id()));
      } else if (dynamic(field.second.get()) == kTimeKey) {
        vec.emplace_back(iter->value().ts());
      } else {
        vec.push_back(iter->value().atNoThrow(field.second.get()));
      }
    }
    v = std::move(vec);
  }

  const dynamic& rhs = values.size() > 1 ? values_ : values[0];
  switch (filterType_) {
  case FilterType::GE:
    return v >= rhs;
  case FilterType::GT:
    return v > rhs;
  case FilterType::LE:
    return v <= rhs;
  case FilterType::LT:
    return v < rhs;
  case FilterType::RANGE:
    return v >= values.at(0) && v <= values.at(1);
  case FilterType::EQ:
    return std::find(values.begin(), values.end(), v) != values.end();
  case FilterType::NE:
    return (v != rhs);
  case FilterType::PREFIX:
    try {
      folly::StringPiece sp = v.is_of<folly::StringPiece>()
                                  ? v.get<folly::StringPiece>()
                                  : v.getRef<std::string>();
      return sp.startsWith(rhs.get<std::string>());
    } catch (const std::exception& e) {
      LOG(ERROR) << "Failed to compare by prefix: " << e.what();
      return false;
    }
  case FilterType::CONTAINS:
    try {
      folly::StringPiece sp = v.is_of<folly::StringPiece>()
                                  ? v.get<folly::StringPiece>()
                                  : v.getRef<std::string>();
      return sp.contains(rhs.get<std::string>());
    } catch (const std::exception& e) {
      LOG(ERROR) << "StringPiece contains exception: " << e.what();
      return false;
    }
  case FilterType::INSET:
    return std::find(values.begin(), values.end(), v) != values.end();
  default:
    return false;
  }
}
}
