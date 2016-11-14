//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
//
// Visitors to implement comparators and some operators
#pragma once

namespace iterlib { namespace variant {

namespace detail {
  struct less_visitor : boost::static_visitor<bool> {

    // Find a way to make this less verbose using std::enable_if
    bool operator()(const int64_t& a, const int64_t& b) const {
      return a < b;
    }

    bool operator()(const double& a, const double& b) const {
      return a < b;
    }

    bool operator()(const std::string& a, const std::string& b) const {
      return a < b;
    }

    bool operator()(const folly::StringPiece a,
                    const folly::StringPiece b) const {
      return a < b;
    }

    bool operator()(const std::string& a, const folly::StringPiece b)
      const {
      return a < b;
    }

    bool operator()(const folly::StringPiece a, const std::string& b)
      const {
      return a < b;
    }

    bool operator()(const boost::blank& a, const boost::blank& b)
      const {
      return false;
    }

    bool operator()(const vector_dynamic_t& a,
                    const vector_dynamic_t& b) const {
      auto minSize = std::min(a.size(), b.size());
      for (size_t i = 0; i < minSize; i++) {
        if (a[i] == b[i]) {
          continue;
        } else {
          return a[i] < b[i];
        }
      }
      return a.size() < b.size();
    }

    bool operator()(const unordered_map_t& a, const unordered_map_t& b) const {
      throw std::logic_error("attempt to compare unordered maps");
    }

    bool operator()(const ordered_map_t& a, const ordered_map_t& b) const {
      return a < b;
    }

    /*
     * Customized comparator
     * If the number of keys are different, we throw
     * If the keys are different, return a comparison of keys
     * If the keys are same, return a comparison of values
     */
    bool operator()(const vector_pair_t& a, const vector_pair_t& b) const {
      if (a.first == nullptr || b.first == nullptr) {
        throw std::logic_error("Malformed vector pair, key vector is null");
      }
      // Keys in vector_pair a
      const auto& aKeys = *a.first;
      // Keys in vector pair b
      const auto& bKeys = *b.first;
      // Values in vector pair a
      const auto& aValues = a.second;
      // Values in vector pair b
      const auto& bValues = b.second;
      if (aKeys.size() != aValues.size() ||
          bKeys.size() != bValues.size()) {
        throw std::logic_error(
            "Malformed vector pair, key and value vector size does not match");
      }
      if (aKeys.size() != bKeys.size()) {
        throw std::logic_error("Mismatch in the number of keys");
      }
      for (int i = 0; i < aKeys.size(); ++i) {
        if (aKeys[i] != bKeys[i]) {
          return (aKeys[i] < bKeys[i]);
        }
        if (!(aValues[i] == bValues[i])) {
          return boost::apply_visitor(*this, aValues[i], bValues[i]);
        }
      }
      return false;
    }

    template <typename T, typename U>
    bool operator() (T v1, U v2) const {
      throw std::logic_error(
          folly::stringPrintf("Not supported '<' comparison: %s against %s",
                              typeid(v1).name(),
                              typeid(v2).name()));
    }
  };

  struct equal_visitor : boost::static_visitor<bool> {

    template <typename T>
    bool operator()(const T& a, const T& b) const {
      return a == b;
    }

    bool operator()(const std::string& a, const folly::StringPiece b)
      const {
      return a == b;
    }

    bool operator()(const folly::StringPiece a, const std::string& b)
      const {
      return a == b;
    }

    bool operator()(const unordered_map_t& a, const unordered_map_t& b)
      const {
      return a == b;
    }

    bool operator()(const vector_pair_t& a, const vector_pair_t& b)
      const {
      return (*a.first == *b.first) && (a.second == b.second);
    }

    template <typename T, typename U>
    bool operator()(T v1, U v2) const {
      return false;
    }
  };

  struct comparator_less {
    less_visitor visitor;

    bool operator() (const dynamic& v1, const dynamic& v2) const {
      return boost::apply_visitor(visitor, v1, v2);
    }
  };

  struct comparator_equal {
    equal_visitor visitor;

    bool operator() (const dynamic& v1, const dynamic& v2) const {
      return boost::apply_visitor(visitor, v1, v2);
    }
  };

  struct append_visitor : boost::static_visitor<dynamic> {
    dynamic operator()(const int64_t a, const int64_t b) const {
      return a + b;
    }

    dynamic operator()(const std::string& a, const std::string& b) const {
      return a + b;
    }

    dynamic operator()(const double& a, const double& b) const {
      return a + b;
    }

    dynamic operator()(const int64_t& a, const double& b) const {
      return a + b;
    }

    dynamic operator()(const double& a, const int64_t& b) const {
      return a + b;
    }

    dynamic operator()(const vector_dynamic_t& a, const vector_dynamic_t& b)
      const {
      dynamic tmp = a;
      auto& tmpVec = tmp.getNonConstRef<vector_dynamic_t>();
      // Optimize this if it turns out to be in the critical path
      std::copy(std::make_move_iterator(b.begin()),
                std::make_move_iterator(b.end()),
                std::back_inserter(tmpVec));
      return tmp;
    }

    template<typename T>
    dynamic operator()(const vector_dynamic_t& a, const T& b) const {
      dynamic tmp = a;
      tmp.getNonConstRef<vector_dynamic_t>().emplace_back(b);
      return tmp;
    }

    // We could add some code here to define semantics on how
    // to append dissimilar types vector and a map for example

    template <typename T, typename U>
    T operator()(const T& v1, const U& v2) const {
      throw std::logic_error(folly::stringPrintf(
        "can't append: %s %s", typeid(v1).name(), typeid(v2).name()));
    }
  };

  /*
   * The mul_visitor implements the logic operator* for dynamic
   * For int and float number, it returns the product of two numbers
   * For vector of numbers, it computes the inner product of two vectors
   */
  struct mul_visitor : boost::static_visitor<dynamic> {
    dynamic operator()(const int64_t a, const int64_t b) const {
      return a * b;
    }

    dynamic operator()(const double& a, const double& b) const {
      return a * b;
    }

    dynamic operator()(const double& a, const int64_t& b) const {
      return a * b;
    }

    dynamic operator()(const int64_t& a, const double& b) const {
      return a * b;
    }

    dynamic operator()(const vector_dynamic_t& a, const vector_dynamic_t& b)
      const {
      dynamic tmp = 0.0;
      if (a.size() != b.size()) {
        throw std::logic_error("Size of the vectors mismatch");
      }
      for (size_t i = 0 ; i < a.size() ; i++) {
        const dynamic& i1 = a[i];
        const dynamic& i2 = b[i];
        tmp += i1*i2;
      }
      return tmp;
    }

    template <typename T, typename U>
    dynamic operator()(const T& v1, const U& v2) const {
      throw std::logic_error(folly::stringPrintf(
        "not able to compute: %s %s", typeid(v1).name(), typeid(v2).name()));
    }
  };

  struct vector_pair_index {
    int operator()(const vector_pair_t& pair, const dynamic& key) {
      if (pair.first == nullptr) {
        throw std::logic_error("Malformed vector pair, key vector is null");
      }
      if (!key.is_of<std::string>() && !key.is_of<folly::StringPiece>()) {
        throw std::out_of_range("Key not found");
      }
      const auto it =
          std::find(pair.first->begin(),
                    pair.first->end(),
                    (key.is_of<std::string>()
                         ? key.getRef<std::string>()
                         : key.get<folly::StringPiece>().toString()));
      if (it == pair.first->end()) {
        throw std::out_of_range("Key not found");
      }
      return (it - pair.first->begin());
    }
  };

  struct empty_visitor : boost::static_visitor<bool> {
    bool operator()(const boost::blank v) const {
      return true;
    }

    bool operator()(const bool v) const {
      return false;
    }

    bool operator()(const int64_t v) const {
      return false;
    }

    bool operator()(const double v) const {
      return false;
    }

    bool operator()(const folly::StringPiece v) const {
      return v.empty();
    }

    bool operator()(const vector_dynamic_t& vec) const {
      return vec.empty();
    }

    template <typename T>
    bool operator()(const std::vector<T>& vec) const {
      return vec.empty();
    }

    bool operator()(
        const unordered_map_t& m) const {
      return m.empty();
    }

    bool operator()(const ordered_map_t& m) const {
      return m.empty();
    }

    bool operator()(const vector_pair_t& pair) const {
      // This is a map with keys in pair.first and values in pair.second
      return pair.second.empty();
    }
  };

  struct dynamic_empty {
    empty_visitor visitor;

    bool operator() (const dynamic& v) const {
      return boost::apply_visitor(visitor, v);
    }
  };

  struct length_visitor : boost::static_visitor<size_t> {
    size_t operator()(const boost::blank v) const {
      throw std::logic_error("length operator not supported for this type");
    }

    size_t operator()(const bool v) const {
      throw std::logic_error("length operator not supported for this type");
    }

    size_t operator()(const int64_t v) const {
      throw std::logic_error("length operator not supported for this type");
    }

    size_t operator()(const double v) const {
      throw std::logic_error("length operator not supported for this type");
    }

    size_t operator()(const folly::StringPiece v) const {
      return v.size();
    }

    size_t operator()(const vector_dynamic_t& vec) const {
      return vec.size();
    }

    template <typename T>
    size_t operator()(const std::vector<T>& vec) const {
      return vec.size();
    }

    size_t operator()(
        const unordered_map_t& m) const {
      return m.size();
    }

    size_t operator()(const ordered_map_t& m) const {
      return m.size();
    }

    size_t operator()(const vector_pair_t& pair) const {
      return pair.second.size();
    }
  };

  struct dynamic_length {
    length_visitor visitor;

    size_t operator() (const dynamic& v) const {
      return boost::apply_visitor(visitor, v);
    }
  };
}

inline bool dynamic::operator<(const dynamic& other) const {
  return detail::comparator_less()(*this, other);
}

inline bool dynamic::operator==(const dynamic& other) const {
  return detail::comparator_equal()(*this, other);
}

inline dynamic& dynamic::operator+=(const dynamic& other) {
  detail::append_visitor visitor;
  auto tmp = boost::apply_visitor(visitor, *this, other);
  *this = std::move(tmp);
  return *this;
}

inline dynamic dynamic::operator*(const dynamic& other) const {
  detail::mul_visitor visitor;
  return boost::apply_visitor(visitor, *this, other);
}

}}
