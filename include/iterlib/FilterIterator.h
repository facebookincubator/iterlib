#pragma once

#include "iterlib/WrappedIterator.h"

namespace iterlib {

enum class FilterType {
  INVALID = 0,
  GE,
  GT,
  LE,
  LT,
  EQ,
  NE,
  EXISTS,
  PREFIX,
  CONTAINS,
  // These types can take more than one argument
  RANGE,
  INSET,
};

template <typename T=Item>
class FilterIteratorBase : public WrappedIterator<T> {
 public:
  explicit FilterIteratorBase(Iterator<T>* iter) : WrappedIterator<T>(iter) {}

  bool orderPreserving() const override { return true; }

 protected:
  bool doNext() override;

  bool doSkipTo(id_t id) override;

  virtual bool match(const Iterator<T>* iter) = 0;
};

template <typename T=Item>
class FilterIterator : public FilterIteratorBase<T> {
 public:
  explicit FilterIterator(Iterator<T>* iter)
      : FilterIteratorBase<T>(iter), firstTime_(true) {}

  void setFilter(const std::vector<std::string>& fields,
                 const std::vector<dynamic>& values, FilterType filterType) {
    if (fields.size() == 1) {
      fields_ = fields[0];
    } else {
      auto fieldVec = variant::vector_dynamic_t();
      for (const auto& v : fields) {
        fieldVec.emplace_back(v);
      }
      fields_ = std::move(fieldVec);
    }

    // We support FilterType operators with:
    //  1 field, 1 value
    //  1 field, n values
    // and some combination of the two
    //
    // TODO: Better detection of incorrect arity of field/values
    // Eg: (filter (= (a  b) (1 2 3)))
    auto valueVec = variant::vector_dynamic_t();
    int i = 0;
    for (const auto& value : values) {
      // Use other type information for attributes
      // that are ints
      if ((i < fields.size()) &&
          (fields[i] == kIdKey || (fields[i] == kTimeKey))) {
        int64_t intVal;
        if (value.is_of<int64_t>()) {
          intVal = value.get<int64_t>();
        } else {
          intVal = folly::to<int64_t>(value.toString());
        }
        valueVec.emplace_back(intVal);
      } else {
        valueVec.push_back(value);
      }
      i++;
    }
    values_ = std::move(valueVec);
    filterType_ = filterType;
  }

 protected:
  virtual bool match(const Iterator<T>* iter) override;

 private:
  // filter information
  dynamic fields_;
  dynamic values_;

  bool firstTime_;

  FilterType filterType_;
};
}

#include "iterlib/FilterIterator-inl.h"
