//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
//
// Code to convert to json
#pragma once

#include <folly/json.h>

namespace iterlib { namespace variant {

std::string toJson(const dynamic& v);

struct JsonPrinter : public boost::static_visitor<> {
  explicit JsonPrinter(std::ostream& out) : out_(out) {}
  virtual ~JsonPrinter() {}

  void operator() (const boost::blank v) const {
    out_ << "null";
  }

  void operator() (const bool v) const {
    out_ << v;
  }

  void operator() (const int64_t v) const {
    out_ << v;
  }

  void operator() (const double v) const {
    out_ << v;
  }

  void operator() (const folly::StringPiece v) const {
    printString(v);
  }

  void operator() (const vector_dynamic_t& vec) const {
    vectorBegin();
    bool first = true;
    for (const auto& v : vec) {
      if (first) {
        first = false;
      } else {
        separator();
      }
      boost::apply_visitor(*this, v);
    }
    vectorEnd();
  }

  template <typename T>
  void operator() (const std::vector<T>& vec) const {
    vectorBegin();
    bool first = true;
    for (const auto& v : vec) {
      if (first) {
        first = false;
      } else {
        separator();
      }
      (*this)(v);
    }
    vectorEnd();
  }

  void operator() (const unordered_map_t& m) const {
    mapBegin();
    bool first = true;
    for (const auto& item : m) {
      if (first) {
        first = false;
      } else {
        separator();
      }

      unorderedMapKey(item.first);
      keyValueSeparator();
      unorderedMapValue(item.second);
    }
    mapEnd();
  }

  void operator() (const ordered_map_t& m) const {
    mapBegin();
    bool first = true;
    for (const auto& item : m) {
      if (first) {
        first = false;
      } else {
        separator();
      }

      orderedMapKey(item.first);
      keyValueSeparator();
      orderedMapValue(item.second);
    }
    mapEnd();
  }

  void operator() (const vector_pair_t& pair) const {
    // This is a map with keys in pair.first and values in pair.second
    if (pair.first == nullptr) {
      throw std::logic_error("Malformed dynamic, key vector was null");
    }
    mapBegin();
    bool first = true;
    for (int i = 0; i < pair.second.size(); ++i) {
      if (first) {
        first = false;
      } else {
        separator();
      }

      unorderedMapKey((*pair.first)[i]);
      keyValueSeparator();
      unorderedMapValue(pair.second[i]);
    }
    mapEnd();
  }

protected:
  virtual void printString(const folly::StringPiece v) const {
    std::string escaped;
    folly::json::escapeString(
        v,
        escaped,
        folly::json::serialization_opts());
    out_ << escaped;
  }

  virtual void vectorBegin() const {
    out_ << "[";
  }

  virtual void vectorEnd() const {
    out_ << "]";
  }

  virtual void mapBegin() const {
    out_ << "{";
  }

  virtual void mapEnd() const {
    out_ << "}";
  }

  void separator() const {
    out_ << ", ";
  }

  virtual void keyValueSeparator() const {
    out_ << ":";
  }

  virtual void unorderedMapKey(const std::string& key) const {
    (*this)(key);
  }

  void unorderedMapValue(const dynamic& value) const {
      boost::apply_visitor(*this, value);
  }

  virtual void orderedMapKey(const dynamic& key) const {
      boost::apply_visitor(*this, key);
  }

  void orderedMapValue(const dynamic& value) const {
      boost::apply_visitor(*this, value);
  }

  std::ostream& out_;
};

inline std::string toJson(const dynamic& v) {
  std::stringstream json;
  boost::apply_visitor(JsonPrinter(json), v);
  return json.str();
}

inline std::string dynamic::toJson() const {
  return iterlib::variant::toJson(*this);
}

inline std::ostream& operator<<(std::ostream& out, const dynamic& v) {
  boost::apply_visitor(JsonPrinter(out), v);
  return out;
}

}}
