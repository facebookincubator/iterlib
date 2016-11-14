//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#pragma once

#include <boost/variant.hpp>
#include <boost/blank.hpp>
#include <boost/variant/recursive_variant.hpp>

#include <map>
#include <vector>
#include <type_traits>
#include <folly/dynamic.h>
#include <folly/sorted_vector_types.h>
#include <folly/String.h>

namespace iterlib { namespace variant {

struct dynamic;
class dynamic_iterator;
typedef std::vector<dynamic> vector_dynamic_t;
typedef folly::sorted_vector_map<dynamic, dynamic> ordered_map_t;
typedef std::unordered_map<std::string, dynamic> unordered_map_t;
typedef std::pair<const std::vector<std::string>*, std::vector<dynamic>>
  vector_pair_t;

// Add a corresponding test to make sure the which() is consistent.
typedef boost::variant<
          boost::blank,
          bool,
          int64_t,
          double,
          folly::StringPiece,
          std::string,                                                     // 5
          boost::recursive_wrapper<vector_dynamic_t>,
          std::vector<int64_t>,
          std::vector<folly::StringPiece>,
          boost::recursive_wrapper<unordered_map_t>,
          boost::recursive_wrapper<ordered_map_t>,
          boost::recursive_wrapper<vector_pair_t>
          > dynamic_variant;

class dynamic : public dynamic_variant {
public:
  using dynamic_variant::operator=;
  using dynamic_variant::operator==;

  dynamic(const dynamic& d) = default;
  dynamic& operator=(const dynamic& d) = default;
  dynamic& operator=(dynamic&& d) = default;

  static const dynamic kNullDynamic;

  // Constructors
  dynamic() : dynamic_variant() {}
  /* implicit */ dynamic(folly::dynamic f);

  // Explicit conversions for things that the compiler has trouble
  // converting implicitly
  //
  // int32_t
  /* implicit */ dynamic(int32_t v) : dynamic_variant(int64_t(v)) {}
  // string literals
  /* implicit */ dynamic(const char * const v)
    : dynamic_variant(std::string(v)) {}

  // Do not use as a copy constructor
  template<typename T, class = typename std::enable_if<
      !std::is_same<T, const dynamic>::value
      && !std::is_same<T, dynamic>::value
      && !std::is_same<T, dynamic &>::value
      && !std::is_base_of<dynamic, T>::value
    >::type>
  /* implicit */ dynamic(const T& v) : dynamic_variant(v) {}

  // Do not use as a copy constructor
  template<typename T, class = typename std::enable_if<
      !std::is_same<T, const dynamic>::value
      && !std::is_same<T, dynamic>::value
      && !std::is_same<T, dynamic &>::value
      && !std::is_base_of<dynamic, T>::value
    >::type>
  /* implicit */ dynamic(T&& v) : dynamic_variant(std::forward<T>(v)) {}

  template <typename T>
  bool is_of() const { return type() == typeid(T); }

  // So we don't have to use boost::get<> all over
  // the code base
  template <typename T>
  T get() const {
    return boost::get<T>(*this);
  }

  template <typename T>
  const T& getRef() const {
    return boost::get<const T&>(*this);
  }

  template <typename T>
  T& getNonConstRef() {
    return boost::get<T&>(*this);
  }

  dynamic& operator=(const char* const v) {
    *this = folly::StringPiece(v);
    return *this;
  }

  template <typename T>
  /* implicit */ operator T() const {
    return get<T>();
  }

  // Updates while preserving the type of the dynamic
  void update(folly::StringPiece str) {
    if (is_of<int64_t>()) {
      *this = folly::to<int64_t>(str);
    } else if (is_of<double>()) {
      *this = folly::to<double>(str);
    } else if (is_of<std::string>() ||
               is_of<folly::StringPiece>()) {
      *this = str.toString();
    } else {
      auto what = folly::stringPrintf("Can't convert to: %s", type().name());
      throw std::logic_error(what);
    }
  }

  bool operator<(const dynamic& other) const;
  bool operator==(const dynamic& other) const;

  //
  // For basic types
  // template <typename T>
  // bool operator==(const T& other) const {
  //   return this->get<T>() == other;
  // }
  //
  // For these set of basic type support operator==, we are going to get
  // a bad_get exeception if the types do not match (boost::bad_get)
  // eg. comparing dynamic(string) with int64_t is not allowed
  // relies on get<T>() to do the typeid check
  //
  friend bool operator==(const dynamic& lhs, const std::string& rhs) {
    return lhs.get<std::string>() == rhs;
  }
  friend bool operator==(const std::string& lhs, const dynamic& rhs) {
    return rhs.get<std::string>() == lhs;
  }

  friend bool operator==(const dynamic& lhs, folly::StringPiece& rhs) {
    return lhs.get<folly::StringPiece>() == rhs;
  }
  friend bool operator==(folly::StringPiece& lhs, const dynamic& rhs) {
    return rhs.get<folly::StringPiece>() == lhs;
  }

  friend bool operator==(const dynamic& lhs, const char* rhs) {
    return lhs.get<std::string>() == rhs;
  }
  friend bool operator==(const char* lhs, const dynamic& rhs) {
    return rhs.get<std::string>() == lhs;
  }

  friend bool operator==(const dynamic& lhs, const bool& rhs) {
    return lhs.get<bool>() == rhs;
  }
  friend bool operator==(const bool& lhs, const dynamic& rhs) {
    return rhs.get<bool>() == lhs;
  }

  friend bool operator==(const dynamic& lhs, const double& rhs) {
    return lhs.get<double>() == rhs;
  }
  friend bool operator==(const double& lhs, const dynamic& rhs) {
    return rhs.get<double>() == lhs;
  }

  friend bool operator==(const dynamic& lhs, const int64_t& rhs) {
    return lhs.get<int64_t>() == rhs;
  }
  friend bool operator==(const int64_t& lhs, const dynamic& rhs) {
    return rhs.get<int64_t>() == lhs;
  }

  friend bool operator==(const dynamic& lhs, const int& rhs) {
    return lhs.get<int64_t>() == static_cast<int64_t>(rhs); //sign?
  }
  friend bool operator==(const int& lhs, const dynamic& rhs) {
    return rhs.get<int64_t>() == static_cast<int64_t>(lhs);
  }

  bool operator !=(const dynamic& other) const {
    return !(*this == other);
  }
  bool operator >(const dynamic& other) const { return other < *this; }
  bool operator <=(const dynamic& other) const { return !(other < *this); }
  bool operator >=(const dynamic& other) const { return !(*this < other); }
  dynamic& operator+=(const dynamic& other);
  dynamic operator*(const dynamic& other) const;

  dynamic& append(const dynamic& other) {
    return operator+=(other);
  }

  bool empty() const;
  size_t length() const;

  size_t size() const {
    return length();
  }

  std::string toJson() const;

  // If this dynamic is a primitive type, return a string representation.
  // If its a complex type (map or vector) return the json representation.
  std::string toString() const {
    if (this->is_of<std::string>()) {
      return this->get<std::string>();
    } else if (this->is_of<folly::StringPiece>()) {
      return this->get<folly::StringPiece>().toString();
    } else if (this->is_of<int64_t>()) {
      return folly::to<std::string>(this->get<int64_t>());
    } else if (this->empty()) {
      return std::string();
    } else if (this->is_of<bool>()) {
      return folly::to<std::string>(this->get<bool>());
    } else {
      return toJson();
    }
  }

  // True if the underlying dynamic type is a map type
  // (supports key value pairs), false otherwise
  bool isObject() const {
    return (this->is_of<vector_pair_t>() || this->is_of<unordered_map_t>() ||
            this->is_of<ordered_map_t>());
  }

  // True if the underlying dynamic type is an array type, false otherwise
  bool isArray() const {
    return (this->is_of<vector_dynamic_t>() ||
            this->is_of<std::vector<int64_t>>() ||
            this->is_of<std::vector<folly::StringPiece>>());
  }

  // Keep in sync with dynamic_iterator
  bool isIterable() const {
    return isObject() || this->is_of<vector_dynamic_t>();
  }

  // Change the type of this to be the same as other.
  // Only this->is_of<string-ish> is supported.
  //
  // An exception is thrown for unsupported types
  void castTo(const dynamic& other) {
    if (is_of<std::string>()) {
      std::string piece = get<std::string>();
      *this = other;
      this->update(piece);
    } else {
      folly::StringPiece piece = get<folly::StringPiece>();
      *this = other;
      this->update(piece);
    }
  }

  dynamic& at(size_t idx) {
    // TODO: ameyag: Support other vector types
    // Currently we only support returning a const dynamic&
    if (this->is_of<vector_dynamic_t>()) {
      return this->getNonConstVectorItemRef<vector_dynamic_t>(idx);
    } else {
      try {
        dynamic key = (int64_t)(idx);
        return at(key);
      } catch (const std::out_of_range& ex) {
        throw ex;  // Rethrowing the out_of_range
      } catch (const std::exception& ex) {
        throw std::logic_error(
          folly::stringPrintf("Operation not supported for dynamic type: %d",
          this->which()));
      }
    }
  }

  const dynamic& at(size_t idx) const {
    // TODO: ameyag: Support other vector types
    // Currently we only support returning a const dynamic&
    if (this->is_of<vector_dynamic_t>()) {
      return this->getVectorItemRef<vector_dynamic_t>(idx);
    } else {
      try {
        dynamic key = (int64_t)(idx);
        return at(key);
      } catch (const std::exception& ex) {
        throw std::logic_error(
          folly::stringPrintf("Operation not supported for dynamic type: %d",
          this->which()));
      }
    }
  }

  // We have multiple map types, some with strings as key and others
  // with dynamic as key. [] and .at() attempt to abstract this detail
  // into the class by accepting a dynamic as key.
  // Some caveats: for map types using string keys, at will only work
  // with dynamics whose underlying type is a string or stringpiece else they
  // throw.
  // For map types with dynamic as key at will only work if the lookup dynamic
  // is of the same underlying type as the keys for this map.
  // e.g. dynamic(folly::StringPiece("bar")) will not match
  // dynamic(std::string("bar")) and vice versa.
  dynamic& at(const dynamic& key);
  const dynamic& at(const dynamic& key) const;
  const dynamic& atNoThrow(const dynamic& key) const {
    try {
      return at(key);
    } catch(const std::exception& e) {
      return kNullDynamic;
    }
  }

  // We mimic the behaviour from STL: for non-const reference,
  // square brackets operator for maps automatically inserts
  // default-constructed value if key does not exist.
  dynamic& operator[](const dynamic& key) { return atWithInsert(key); }
  const dynamic& operator[](const dynamic& key) const { return at(key); }

  dynamic& operator[](const char* key) { return atWithInsert(dynamic(key)); }
  const dynamic& operator[](const char* key) const { return at(dynamic(key)); }

  dynamic& operator[](size_t idx) {
    if (this->is_of<ordered_map_t>()) {
      dynamic d = (int64_t)idx;
      return atWithInsert(d);
    }
    return at(idx);
  }
  const dynamic& operator[](size_t idx) const { return at(idx); }

  dynamic_iterator begin() const;
  dynamic_iterator end() const;

  // This is needed for Lua bindings,
  // (memory will be managed by Lua)
  dynamic_iterator* begin_ptr() const;

  // Merge another dynamic into this one. If more customization
  // is needed, please add a merge_with (similar to std::accumulate)
  // that takes a lambda
  void merge(const dynamic& other);

private:
 // Used internally for operator[]. Behaves like at,
 // but when key is not found, it inserts it with
 // default constructed value if possible.
 // Can still throw, e.g. when keys are const
 // or provided key is of wrong type.
 dynamic& atWithInsert(const dynamic& key);

 void mergeImpl(const dynamic& other);

 template <typename T>
 const dynamic& getItemRef(const dynamic& key) const;

 template <typename T>
 dynamic& getNonConstItemRef(const dynamic& key);

 template <typename T>
 dynamic& getNonConstItemRefWithInsert(const dynamic& key);

 template <typename T>
 const dynamic& getVectorItemRef(size_t idx) const {
   const T& vec = this->getRef<T>();
   return static_cast<const dynamic&>(vec.at(idx));
 }

 template <typename T>
 dynamic& getNonConstVectorItemRef(size_t idx) {
   T& vec = this->getNonConstRef<T>();
   return static_cast<dynamic&>(vec.at(idx));
 }
};
// Ensure the size is cache line efficient.
// The biggest variable currently is std::string
// which varies between 8, 24 and 32 bytes depending on implementation.
// TODO: Investigate std::vector<char> as an alternative.
static_assert(sizeof(dynamic) <= 40, "too large");

// Add other kinds of makeMap's as needed.
inline dynamic makeOrderedMap(const dynamic& key, dynamic&& value) {
  dynamic ret = ordered_map_t();
  ret[key] = value;
  return ret;
}

void PrintTo(const dynamic& dyn, std::ostream* os);

}}

#include "iterlib/variant/dynamic-detail.h"
#include "iterlib/variant/dynamic-inl.h"
#include "iterlib/variant/dynamic-folly.h"
#include "iterlib/variant/dynamic-iter.h"
#include "iterlib/variant/dynamic-json.h"
