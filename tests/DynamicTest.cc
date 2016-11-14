//  Copyright (c) 2016, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
#include <gflags/gflags.h>
#include <gtest/gtest.h>

#include <folly/Benchmark.h>
#include <folly/io/async/EventBaseManager.h>

#include "iterlib/dynamic.h"

using namespace iterlib;
using namespace iterlib::variant;

TEST(Dynamic, DefaultValues) {
  dynamic d;
  EXPECT_EQ(d.which(), 0);
  EXPECT_TRUE(d.is_of<boost::blank>());
}

TEST(Dynamic, Initialization) {
  dynamic d1 = 0;
  EXPECT_TRUE(d1.is_of<int64_t>());
  dynamic d2 = 0L;
  EXPECT_TRUE(d2.is_of<int64_t>());
  dynamic d3 = true;
  EXPECT_TRUE(d3.is_of<bool>());
  dynamic d4 = 0.0;
  EXPECT_TRUE(d4.is_of<double>());
  dynamic d5 = folly::StringPiece("foo");
  EXPECT_TRUE(d5.is_of<folly::StringPiece>());
  dynamic d6 = std::string("foo");
  EXPECT_TRUE(d6.is_of<std::string>());
  dynamic d7;
  d7 = "foo";
  EXPECT_TRUE(d7.is_of<folly::StringPiece>());
}

TEST(Dynamic, ContainerInit) {
  dynamic d1{std::vector<int64_t>{ 1L, 2L, 3L }};
  EXPECT_TRUE(d1.is_of<std::vector<int64_t>>());
  auto& v1 = d1.getRef<std::vector<int64_t>>();
  EXPECT_EQ(3, v1.size());

  dynamic d2{std::vector<folly::StringPiece>{"foo", "bar", "baz" }};
  EXPECT_TRUE(d2.is_of<std::vector<folly::StringPiece>>());
  auto& v2 = d2.getRef<std::vector<folly::StringPiece>>();
  EXPECT_EQ(3, v2.size());

  dynamic d3{std::vector<dynamic>{ 1L, "foo", 3L }};
  EXPECT_TRUE(d3.is_of<vector_dynamic_t>());
  auto& v3 = d3.getRef<vector_dynamic_t>();
  EXPECT_EQ(3, v3.size());
  // Safe only if sizeof(dynamic) == sizeof(dynamic_variant)
  // and there are no other layout changes
  auto v4 = reinterpret_cast<std::vector<dynamic> const *>(&v3);
  EXPECT_TRUE((*v4)[0].is_of<int64_t>());
  //EXPECT_TRUE((*v4)[1].is_of<std::string>());
  EXPECT_TRUE((*v4)[1].which() == 5);
  EXPECT_TRUE((*v4)[2].is_of<int64_t>());
}

TEST(Dynamic, CompareMixedValuesMap) {
  dynamic d = unordered_map_t{{"foo", std::string{"xy"}}, {"bar", 1L}};
  EXPECT_EQ(d, d);
}

TEST(Dynamic, CompareVectorPair) {
  {
    auto vec = vector_dynamic_t(
        { 1L, static_cast<std::string>("foo"), 3L });
    auto keyVec = std::vector<std::string>({"one", "two", "three"});
    dynamic d = std::make_pair(&keyVec, vec);
    EXPECT_EQ(d, d);
  }
  {
    int64_t i1 = 1;
    int64_t i2 = 2;
    vector_dynamic_t vec;
    vec.push_back(i1);
    auto keyVec = std::vector<std::string>({"one"});
    dynamic d1 = std::make_pair(&keyVec, vec);
    vector_dynamic_t vec2;
    vec2.push_back(i2);
    auto keyVec2 = std::vector<std::string>({"one"});
    dynamic d2 = std::make_pair(&keyVec2, vec2);
    EXPECT_LT(d1, d2);
  }
  {
    auto vec = vector_dynamic_t(
        { static_cast<std::string>("1") });
    auto keyVec = std::vector<std::string>({"one"});
    dynamic d1 = std::make_pair(&keyVec, vec);
    auto vec2 =
      vector_dynamic_t({ static_cast<std::string>("2") });
    auto keyVec2 = std::vector<std::string>({"two"});
    dynamic d2 = std::make_pair(&keyVec2, vec2);
    EXPECT_LT(d1, d2);
  }
  {
    auto vec = vector_dynamic_t(
        { 1L, static_cast<std::string>("foo"), 3L });
    auto keyVec = std::vector<std::string>({"one", "two", "three"});
    dynamic d1 = std::make_pair(&keyVec, vec);
    auto vec2 =
      vector_dynamic_t({ static_cast<std::string>("2") });
    auto keyVec2 = std::vector<std::string>({"two"});
    dynamic d2 = std::make_pair(&keyVec2, vec2);
    EXPECT_THROW(static_cast<void>(d1 < d2), std::logic_error);
  }
  {
    // Tests comparison for same keys, multiple values
    auto vec = vector_dynamic_t(
        { 1L, static_cast<std::string>("foo"), 3L });
    auto keyVec = std::vector<std::string>({"one", "two", "three"});
    dynamic d1 = std::make_pair(&keyVec, vec);
    auto vec2 =
      vector_dynamic_t({ 1L, static_cast<std::string>("z"), 3L });
    dynamic d2 = std::make_pair(&keyVec, vec2);
    EXPECT_LT(d1, d2);
  }
  {
    // Tests comparison for different keys
    auto vec = vector_dynamic_t(
        { 11L, static_cast<std::string>("foo"), 3L });
    auto keyVec = std::vector<std::string>({"one", "two", "three"});
    dynamic d1 = std::make_pair(&keyVec, vec);
    auto keyVec2 = std::vector<std::string>({"zzz", "two", "three"});
    auto vec2 =
      vector_dynamic_t({ 1L, static_cast<std::string>("aa"), 1L });
    dynamic d2 = std::make_pair(&keyVec2, vec2);
    EXPECT_LT(d1, d2);
  }
  {
    // Tests comparison for different malformed vector pairs
    auto vec = vector_dynamic_t(
        { 11L, static_cast<std::string>("foo"), 3L });
    auto keyVec = std::vector<std::string>({"one", "two", "three"});
    dynamic d1 = std::make_pair(&keyVec, vec);
    auto keyVec2 = std::vector<std::string>({"one", "two", "three"});
    auto vec2 =
      vector_dynamic_t({ 11L, static_cast<std::string>("foo")});
    dynamic d2 = std::make_pair(&keyVec2, vec2);
    EXPECT_THROW(static_cast<void>(d1 < d2), std::logic_error);
  }
}

TEST(Dynamic, CompareStringsMap) {
  dynamic d =
      unordered_map_t{{"foo", std::string{"xy"}}, {"bar", std::string{"yz"}}};
  EXPECT_EQ(d, d);
}

TEST(Dynamic, CompareIntMap) {
  dynamic d = unordered_map_t{{"foo", 1L}, {"bar", 2L}};
  EXPECT_EQ(d, d);
}

TEST(Dynamic, CompareDifferentTypes) {
  dynamic map = unordered_map_t{{"foo", 1L}};
  dynamic vec = std::vector<int64_t>{1L, 2L, 3L};
  dynamic boolean = false;
  dynamic str = std::string{"foo"};

  EXPECT_NE(map, vec);
  EXPECT_NE(map, boolean);
  EXPECT_NE(map, str);

  EXPECT_NE(vec, boolean);
  EXPECT_NE(vec, str);

  EXPECT_NE(boolean, str);
}

TEST(Dynamic, VectorOfDynamicsLessComparator) {
  auto v1 = std::vector<dynamic>{std::string{"x"}, 2L};
  auto v2 = std::vector<dynamic>{std::string{"y"}, 2L};
  auto v3 = std::vector<dynamic>{std::string{"x"}, 3L};
  EXPECT_LT(v1, v2);
  EXPECT_LT(v1, v3);
  EXPECT_LT(v3, v2);

  auto v4 = std::vector<dynamic>{std::string{"x"}, 3L, 4L};
  EXPECT_LT(v1, v4) << "Lexicographical order expected";
  EXPECT_GT(v4, v1) << "Lexicographical order expected";
  EXPECT_LT(v3, v4) << "Lexicographical order expected";
  EXPECT_GT(v4, v3) << "Lexicographical order expected";
}

TEST(Dynamic, VectorEmplaceBackTest) {
  std::vector<dynamic> dynVec;
  dynVec.emplace_back("abcdef");
  dynVec.emplace_back(3945872L);
  dynVec.emplace_back(true);
  EXPECT_EQ(dynVec[0].toString(), "abcdef");
  EXPECT_EQ(dynVec[1].toString(), "3945872");
}

TEST(Dynamic, BoolBasics) {
  dynamic b = true;
  EXPECT_TRUE(b.is_of<bool>());
  EXPECT_TRUE(b.get<bool>());
  dynamic b1 = false;
  EXPECT_TRUE(b1.is_of<bool>());
  EXPECT_FALSE(b1.get<bool>());
  EXPECT_NE(b, b1);
}

TEST(Dynamic, StringPieceBasics) {
  dynamic str;
  str = "hello world";
  EXPECT_EQ(11, str.get<folly::StringPiece>().size());
  EXPECT_FALSE(str.empty());
  str = "";
  EXPECT_TRUE(str.get<folly::StringPiece>().empty());
  EXPECT_TRUE(str.is_of<folly::StringPiece>());
}

TEST(Dynamic, StringBasics) {
  dynamic str;
  str = std::string("hello world");
  EXPECT_EQ(11, str.get<std::string>().size());
  EXPECT_FALSE(str.empty());
  str = std::string("");
  EXPECT_TRUE(str.get<std::string>().empty());
  EXPECT_TRUE(str.is_of<std::string>());
}

TEST(Dynamic, StringComparison) {
  {
    dynamic str = std::string("aaaaaa");
    dynamic str2 = std::string("hello world");
    dynamic str3 = std::string("hello world");
    EXPECT_TRUE(str3 == str2);
    EXPECT_TRUE(str < str3);
  }
  {
    dynamic str = std::string("aaaa");
    std::string testStr = "hello world";
    dynamic sp = folly::StringPiece(testStr);
    dynamic s = std::string("hello world");
    EXPECT_TRUE(sp == s);
    EXPECT_TRUE(s == sp);
    EXPECT_TRUE(str < sp);
    EXPECT_TRUE(sp > str);
  }
}

TEST(Dynamic, BasicTypeEquality) {
  {
    folly::StringPiece fsp = "folly::StringPiece";
    dynamic dfsp = fsp;
    EXPECT_TRUE(dfsp == fsp);
    EXPECT_TRUE(fsp == dfsp);
    auto sp = folly::StringPiece("cat");
    EXPECT_FALSE(sp == dfsp);
  }
  {
    const char *csp = "char-star";
    dynamic dcsp = csp;
    EXPECT_TRUE(dcsp == csp);
    EXPECT_TRUE(csp == dcsp);
    EXPECT_TRUE(dcsp == "char-star");
    EXPECT_FALSE("const-char" == dcsp);
  }
  {
    std::string str = "std::string";
    dynamic dstr = str;
    EXPECT_TRUE(dstr == str);
    EXPECT_TRUE(str == dstr);
    EXPECT_FALSE(std::string("fbstring") == dstr);
  }
  {
    bool truth = true;
    dynamic dtruth = truth;
    EXPECT_TRUE(dtruth == truth);
    EXPECT_TRUE(truth == dtruth);
    EXPECT_FALSE(false == dtruth);
  }
  {
    int64_t xll = UINT_MAX+1;
    int64_t xll2 = UINT_MAX+2;
    dynamic dxll = xll;
    EXPECT_TRUE(dxll == xll);
    EXPECT_TRUE(xll == dxll);
    EXPECT_FALSE(xll2 == dxll);
  }
  {
    int x = INT_MAX-1;
    dynamic dx = x;
    EXPECT_TRUE(dx == x);
    EXPECT_TRUE(x == dx);
    EXPECT_FALSE(INT_MAX-2 == dx);
  }
}

TEST(Dynamic, BasicTypeEqualityWillFail_1) {
  {
    int64_t xll = UINT_MAX+1;
    std::string str = "compare-will-fail";
    dynamic dxll = xll;
    // This will fail with boost::bad_get
    EXPECT_THROW(static_cast<void>(dxll == str), boost::bad_get);
  }
}

TEST(Dynamic, BasicTypeEqualityWillFail_2) {
  {
    int64_t xll = UINT_MAX+1;
    std::string str = "compare-will-fail";
    dynamic dstr = str;
    // This will fail with boost::bad_get
    EXPECT_THROW(static_cast<void>(xll == dstr), boost::bad_get);
  }
}

TEST(Dynamic, IntegerBasics) {
  dynamic v1 = 10L;
  dynamic v2 = 20L;
  dynamic v3 = 10L;

  EXPECT_TRUE(v1.is_of<int64_t>());
  EXPECT_FALSE(v1 == v2);
  EXPECT_TRUE(v1 == v3);
  int64_t v1L = v1.get<int64_t>();
  EXPECT_FALSE(v2.get<int64_t>() == v1L);
  EXPECT_TRUE(v1L == v3.get<int64_t>());

  EXPECT_TRUE(v1 < v2);
  EXPECT_TRUE(v1 <= v2);
  EXPECT_TRUE(v1 <= v3);
  EXPECT_TRUE(v2 > v3);
  EXPECT_TRUE(v2 >= v3);
}

TEST(Dynamic, Empty) {
  dynamic v;
  EXPECT_TRUE(v.empty());

  v = static_cast<int64_t>(10);
  EXPECT_FALSE(v.empty());

  v = false;
  EXPECT_FALSE(v.empty());

  v = static_cast<double>(10);
  EXPECT_FALSE(v.empty());

  vector_dynamic_t strVector = {static_cast<std::string>("hello"),
                                            static_cast<std::string>("world")};

  v = strVector;
  EXPECT_FALSE(v.empty());
  strVector.clear();
  v = strVector;
  EXPECT_TRUE(v.empty());

  unordered_map_t m = {
      {"b", 20L}, {"a", 10L}, {"c", static_cast<std::string>("hello")}};
  v = m;
  EXPECT_FALSE(v.empty());
  m.clear();
  v = m;
  EXPECT_TRUE(v.empty());

  ordered_map_t om{{std::string{"b"}, 20L},
                   {std::string{"c"}, static_cast<std::string>("hello")},
                   {std::string{"a"}, 10L}};
  v = om;
  EXPECT_FALSE(v.empty());
  om.clear();
  v = om;
  EXPECT_TRUE(v.empty());

  // Lazy map
  auto vec =
      vector_dynamic_t({1L, static_cast<std::string>("foo"), 3L});
  auto keyVec = std::vector<std::string>({"one", "two", "three"});
  v = std::make_pair(&keyVec, vec);
  EXPECT_FALSE(v.empty());

  vec = vector_dynamic_t({});
  keyVec = std::vector<std::string>({});
  v = std::make_pair(&keyVec, vec);
  EXPECT_TRUE(v.empty());
}

TEST(Dynamic, Length) {
  dynamic v;
  EXPECT_THROW(v.length(), std::logic_error);

  v = static_cast<int64_t>(10);
  EXPECT_THROW(v.length(), std::logic_error);

  v = false;
  EXPECT_THROW(v.length(), std::logic_error);

  v = static_cast<double>(10);
  EXPECT_THROW(v.length(), std::logic_error);

  v = std::string("hello");
  EXPECT_EQ(v.length(), 5);

  v = folly::StringPiece("hi");
  EXPECT_EQ(v.length(), 2);

  vector_dynamic_t strVector = {static_cast<std::string>("one"),
                                            static_cast<std::string>("two"),
                                            static_cast<std::string>("three")};

  v = strVector;
  EXPECT_EQ(v.length(), 3);
  strVector.push_back(static_cast<std::string>("four"));
  v = strVector;
  EXPECT_EQ(v.length(), 4);

  unordered_map_t m = {
      {"b", 20L}, {"a", 10L}, {"c", static_cast<std::string>("hello")}};
  v = m;
  EXPECT_EQ(v.length(), 3);
  m.insert(std::make_pair("x", 5.0));
  v = m;
  EXPECT_EQ(v.length(), 4);

  ordered_map_t om{{std::string{"b"}, 20L},
                   {std::string{"c"}, static_cast<std::string>("hello")},
                   {std::string{"a"}, 10L}};
  v = om;
  EXPECT_EQ(v.length(), 3);
  om.insert({std::string{"q"}, 40L});
  v = om;
  EXPECT_EQ(v.length(), 4);

  auto vec =
      vector_dynamic_t({1L, static_cast<std::string>("foo"), 3L});
  auto keyVec = std::vector<std::string>({"one", "two", "three"});
  v = std::make_pair(&keyVec, vec);
  EXPECT_EQ(v.length(), 3);
}

TEST(Dynamic, toString) {
  dynamic v;
  EXPECT_EQ("", v.toString());
  EXPECT_EQ("null", v.toJson());

  v = static_cast<int64_t>(10);
  EXPECT_EQ("10", v.toString());
  EXPECT_EQ(v.toString(), v.toJson());

  v = std::string("hello");
  EXPECT_EQ("hello", v.toString());
  EXPECT_EQ("\"hello\"", v.toJson());

  v = folly::StringPiece("world");
  EXPECT_EQ("world", v.toString());
  EXPECT_EQ("\"world\"", v.toJson());

  v = true;
  EXPECT_EQ("1", v.toString());
  EXPECT_EQ(v.toString(), v.toJson());

  const vector_dynamic_t strVector = {static_cast<std::string>("hello"),
                    static_cast<std::string>("world")};

  v = strVector;
  EXPECT_EQ(v.toJson(), v.toString());

  const unordered_map_t m = {
      {"b", 20L}, {"a", 10L}, {"c", static_cast<std::string>("hello")}};
  v = m;
  EXPECT_EQ(v.toJson(), v.toString());

  const ordered_map_t om{{std::string{"b"}, 20L},
                {std::string{"c"}, static_cast<std::string>("hello")},
                {std::string{"a"}, 10L}};
  v = om;
  EXPECT_EQ(v.toJson(), v.toString());

  // Vector pair
  auto vec = vector_dynamic_t(
      { 1L, static_cast<std::string>("foo"), 3L });
  auto keyVec = std::vector<std::string>({"one", "two", "three"});
  v = std::make_pair(&keyVec, vec);
  EXPECT_EQ(v.toJson(), v.toString());
}

TEST(Dynamic, FollyDynamicConversion) {
  dynamic v;
  folly::dynamic dyn = toFollyDynamic(v);
  EXPECT_EQ(dyn.type(), dyn.OBJECT);
  EXPECT_TRUE(dyn == folly::dynamic::object);

  v = static_cast<int64_t>(10);
  dyn = toFollyDynamic(v);
  EXPECT_EQ(dyn.type(), dyn.INT64);
  EXPECT_EQ(dyn.asInt(), 10);

  v = false;
  dyn = toFollyDynamic(v);
  EXPECT_EQ(dyn.type(), dyn.BOOL);
  EXPECT_EQ(dyn.asBool(), false);

  v = static_cast<double>(10);
  dyn = toFollyDynamic(v);
  EXPECT_EQ(dyn.type(), dyn.DOUBLE);
  EXPECT_EQ(dyn.asDouble(), 10);

  vector_dynamic_t strVector{"hello", "world"};
  v = strVector;
  dyn = toFollyDynamic(v);
  EXPECT_TRUE(dyn.isArray());
  EXPECT_TRUE(dyn.at(0) == "hello");
  EXPECT_TRUE(dyn.at(1) == "world");

  const unordered_map_t m =
  {{ "b", 20L}, { "a", 10L}, {"c", static_cast<std::string>("hello")}};
  v = m;
  dyn = toFollyDynamic(v);
  EXPECT_TRUE(dyn["a"] == 10);
  EXPECT_TRUE(dyn["b"] == 20);
  EXPECT_TRUE(dyn["c"] == "hello");

  const ordered_map_t om{{ std::string{"b"}, 20L},
                         { std::string{"c"}, static_cast<std::string>("hello")},
                         { std::string{"a"}, 10L}};
  v = om;
  dyn = toFollyDynamic(v);
  EXPECT_TRUE(dyn["a"] == 10);
  EXPECT_TRUE(dyn["b"] == 20);
  EXPECT_TRUE(dyn["c"] == "hello");

  // Vector pair
  auto vec = vector_dynamic_t(
      { 1L, static_cast<std::string>("foo"), 3L });
  auto keyVec = std::vector<std::string>({"one", "two", "three"});
  v = std::make_pair(&keyVec, vec);
  dyn = toFollyDynamic(v);
  EXPECT_TRUE(dyn["one"] == 1L);
  EXPECT_TRUE(dyn["two"] == "foo");
  EXPECT_TRUE(dyn["three"] == 3L);
}

TEST(Dynamic, Json) {
  // maps
  const unordered_map_t v1 = {{ "b", 20L},
                                                               { "a", 10L}};
  const dynamic v2 = v1;
  EXPECT_EQ(toJson(v2), "{\"a\":10, \"b\":20}");
  EXPECT_EQ(v2.toJson(), "{\"a\":10, \"b\":20}");

  // vectors
  const std::vector<int64_t> v3 = {5, 4, 3, 2, 1};
  const dynamic v4 = v3;
  EXPECT_EQ(toJson(v4), "[5, 4, 3, 2, 1]");

  // strings
  const std::vector<folly::StringPiece> v5 = {"sfo", "lax", "jfk"};
  const dynamic v6 = v5;
  EXPECT_EQ(toJson(v6), "[\"sfo\", \"lax\", \"jfk\"]");
}

TEST(Dynamic, OrderedMap) {
  // maps
  const ordered_map_t v1{{ std::string{"b"}, 20L},
                         { std::string{"c"}, 15L},
                         { std::string{"a"}, 10L}};
  const dynamic v2 = v1;
  EXPECT_TRUE(v2.is_of<ordered_map_t>());
  EXPECT_EQ(v2.toJson(), "{\"a\":10, \"b\":20, \"c\":15}");
  dynamic v3 = folly::sorted_vector_map<dynamic, dynamic>(
    {{ std::string{"b"}, 20L},
     { std::string{"c"}, 15L},
     { std::string{"a"}, 10L}});
  EXPECT_TRUE(v3.is_of<ordered_map_t>());
  EXPECT_EQ(v3.toJson(), "{\"a\":10, \"b\":20, \"c\":15}");
}

TEST(Dynamic, OrderedMapCommonComplex) {
  // maps
  const ordered_map_t key1{{std::string{"b"}, 20L},
                           {std::string{"a"}, 10L}};
  const ordered_map_t key2{{std::string{"x"}, 10L}};
  const ordered_map_t value{{std::string{"d"}, 100L}};
  const ordered_map_t v1{{key1, value}, {key2, value}};

  const dynamic v2 = v1;
  EXPECT_TRUE(v2.is_of<ordered_map_t>());
  EXPECT_EQ(v2.toJson(),
            "{{\"a\":10, \"b\":20}:{\"d\":100}, {\"x\":10}:{\"d\":100}}");
}

TEST(Dynamic, SizeAssert) {
  dynamic v1 = 10L;
  static_assert(sizeof(v1) <= 40, "unexpected size");
}

TEST(Dynamic, CopyConstructor) {
  dynamic *d1 = new dynamic(boost::blank());
  const dynamic &d1ref = *d1;
  dynamic d2(d1ref);
  delete d1;
  EXPECT_TRUE(d2.is_of<boost::blank>());
}

// Used by a couple of tests below
typedef std::tuple<int64_t, float, dynamic> test_t;

TEST(Dynamic, TupleTest) {
  test_t foo = std::make_tuple(10, 20.0, 100L);
  int64_t l;
  float f;
  dynamic d;
  std::tie(l, f, d) = foo;
  EXPECT_EQ(l, 10);
  EXPECT_EQ(f, 20.0);
  EXPECT_TRUE(d.get<int64_t>() == 100L);
  EXPECT_TRUE(d.is_of<int64_t>());
}

TEST(Dynamic, TupleTestDerived) {
  class DerivedTuple : public test_t {
  public:
    DerivedTuple(int64_t x, float y, dynamic d = std::string()) {
      std::get<0>(*this) = x;
      std::get<1>(*this) = y;
      std::get<2>(*this) = d;
    }
  };
  DerivedTuple foo = DerivedTuple(10, 20.0);
  int64_t l;
  float f;
  dynamic d;
  std::tie(l, f, d) = foo;
  EXPECT_EQ(l, 10);
  EXPECT_EQ(f, 20.0);
  EXPECT_TRUE(d.is_of<std::string>());
}

TEST(Dynamic, MoveTest) {
  dynamic str;
  str = std::string("hello world");
  EXPECT_FALSE(str.empty());
  dynamic str2 = std::move(str);
  EXPECT_TRUE(str2.is_of<std::string>());
  EXPECT_EQ(str2.get<std::string>(), "hello world");
}

TEST(Dynamic, Warts) {
  dynamic d3 = 0L;
  dynamic d2 = 0;
  dynamic d4 = d3;
  dynamic d5 = d4;
  d5 = std::move(d4);
  EXPECT_EQ(d2, d3);
  EXPECT_EQ(d4, d3);
  dynamic d0 = vector_dynamic_t();
  dynamic d1 = std::vector<dynamic>();
  std::pair<std::string, dynamic> p = {"foo", d3};
}

TEST(Dynamic, CloneAndUpdate) {
  dynamic d1 = 10;
  dynamic d2 = d1;
  d2.update("100");
  EXPECT_TRUE(d2 == dynamic(100));
  EXPECT_TRUE(d2.is_of<int64_t>());
  dynamic d3 = "foo";
  {
    std::string s = "bar";
    d3.update(folly::StringPiece(s));
  }
  // s is dead, but d3 is alive
  EXPECT_TRUE(d3 == dynamic("bar"));
}

TEST(Dynamic, ConstRefConstructor) {
  const dynamic d = "100";
  dynamic d2(d);
  EXPECT_TRUE(d2 == dynamic("100"));
}

TEST(Dynamic, castTo) {
  dynamic d1 = 10;
  dynamic d2 = std::string("100");
  d2.castTo(d1);
  EXPECT_TRUE(d2 == dynamic(100));
  dynamic d3 = folly::StringPiece("100");
  d3.castTo(d1);
  EXPECT_TRUE(d3 == dynamic(100));
}

TEST(Dynamic, VectorPair) {
  {
    vector_dynamic_t vec =
      vector_dynamic_t({ 1L, std::string("foo"), 3L });
    std::vector<std::string> keyVec =
      std::vector<std::string>({"one", "two", "three"});
    dynamic d = std::make_pair(&keyVec, vec);
    EXPECT_TRUE(d.is_of<vector_pair_t>());
    EXPECT_EQ(d.toJson(), "{\"one\":1, \"two\":\"foo\", \"three\":3}");
  }
}

TEST(DynamicAt, Unordered) {
  const dynamic d = unordered_map_t{{"foo", std::string{"xy"}}, {"bar", 1L}};
  dynamic td = std::string("xy");
  EXPECT_TRUE(td == d.at("foo"));
  EXPECT_TRUE(td == d["foo"]);
  td = 1L;
  EXPECT_TRUE(td == d.at("bar"));
  EXPECT_TRUE(td == d["bar"]);
  dynamic key = std::string("bar");
  EXPECT_TRUE(td == d[key]);
  key = folly::StringPiece("bar");
  EXPECT_TRUE(td == d[key]);
  EXPECT_THROW(d.at("undef"), std::out_of_range);
  EXPECT_THROW(d.at(td), std::out_of_range);
  EXPECT_THROW(d[td], std::out_of_range);
  td = 5L;
  EXPECT_THROW(d[td], std::out_of_range);
}

TEST(DynamicAt, Ordered) {
  const dynamic d = ordered_map_t{{std::string{"foo"}, std::string{"xyz"}},
                            {std::string{"bar"}, 4L}};
  dynamic td = std::string("xyz");
  dynamic key = std::string("foo");
  EXPECT_TRUE(td == d.at(key));
  EXPECT_TRUE(td == d[key]);
  td = 4L;
  key = std::string("bar");
  EXPECT_TRUE(td == d.at(key));
  EXPECT_TRUE(td == d[key]);
  EXPECT_TRUE(td == d[std::string("bar")]);
  const dynamic d1 = ordered_map_t{{4L, 6L}, {5L, 6L}};
  td = 6L;
  key = 4L;
  EXPECT_TRUE(td == d1.at(key));
  EXPECT_TRUE(td == d1[key]);
  key = std::string("undef");
  EXPECT_THROW(d.at(key).toString(), std::out_of_range);
  EXPECT_THROW(d.at(folly::StringPiece("undef")).toString(), std::out_of_range);
}

TEST(DynamicAt, Vector) {
  const dynamic d = vector_dynamic_t({ 1L, std::string("foo"), 3L });
  dynamic td = 1L;
  EXPECT_TRUE(d.at(0) == td);
  td = std::string("foo");
  EXPECT_TRUE(d[1] == td);
  td = 3L;
  EXPECT_TRUE(d.at(2) == td);
}

TEST(DynamicAt, VectorPair) {
  // Vector pair
  vector_dynamic_t vec =
   vector_dynamic_t({ 1L, static_cast<std::string>("foo"), 3L });
  std::vector<std::string> keyVec =
    std::vector<std::string>({"one", "two", "three"});
  const dynamic d = std::make_pair(&keyVec, vec);
  dynamic td = 1L;
  EXPECT_TRUE(d.at("one") == td);
  EXPECT_TRUE(d["one"] == td);
  td = std::string("foo");
  EXPECT_TRUE(d.at("two") == td);
  EXPECT_TRUE(d["two"] == td);
  td = 3L;
  EXPECT_TRUE(d.at("three") == td);
  EXPECT_TRUE(d["three"] == td);
  dynamic key = std::string("three");
  EXPECT_TRUE(td == d[key]);
  key = folly::StringPiece("three");
  EXPECT_TRUE(td == d[key]);
  EXPECT_THROW(d.at("four").toString(), std::out_of_range);
  EXPECT_THROW(d["four"], std::out_of_range);
}

TEST(DynamicAt, SquareBracketsOp) {
  dynamic d = ordered_map_t{{std::string{"foo"}, std::string{"xyz"}}};
  dynamic key = std::string("nonexistent");
  dynamic value;
  EXPECT_THROW(d.at(key), std::out_of_range);
  EXPECT_TRUE(d[key] == value); // this inserts the key
  EXPECT_TRUE(d.at(key) == value); // so it is accesible here
  dynamic key2 = std::string("k");
  dynamic val2 = std::string("v");
  d[key2] = val2;
  EXPECT_TRUE(d.at(key2) == val2);

  d = unordered_map_t{{std::string{"foo"}, std::string{"xyz"}}};
  EXPECT_THROW(d.at(key), std::out_of_range);
  EXPECT_TRUE(d[key] == value);
  EXPECT_TRUE(d.at(key) == value);

  vector_dynamic_t vec =
   vector_dynamic_t({ static_cast<std::string>("xyz")});
  std::vector<std::string> keyVec =
    std::vector<std::string>({"foo"});
  d = std::make_pair(&keyVec, vec);
  EXPECT_THROW(d.at(key), std::out_of_range);
  EXPECT_THROW(d[key], std::logic_error);
}

TEST(DynamicAt, Unsupported) {
  // Unsupported ops
  dynamic d = 1L;
  EXPECT_THROW(d.at("key"), std::logic_error);
  d = std::string("hello");
  EXPECT_THROW(d["key"], std::logic_error);
  d = folly::StringPiece("hello");
  EXPECT_THROW(d.at("key"), std::logic_error);
  d = std::vector<int64_t>({1,2,3});
  EXPECT_THROW(d.at("key"), std::logic_error);
}

TEST(DynamicAt, SelfAssignment) {
  dynamic d = folly::sorted_vector_map<dynamic, dynamic>(
    {{ "b", 20L},
     { "c", 15L},
     { "a", 10L}});
  // TODO(asharma): d = d.at(..) triggers an ASAN bug
  dynamic key = std::string("c");
  auto d1 = d.at(key);
  EXPECT_TRUE(d1.get<int64_t>() == 15L);
}

TEST(DynamicAt, Assign) {
  {
    auto vec =
        vector_dynamic_t({1L, static_cast<std::string>("foo"), 3L});
    auto keyVec = std::vector<std::string>({"one", "two", "three"});
    dynamic d = std::make_pair(&keyVec, vec);
    dynamic td = 3L;
    EXPECT_EQ(d["three"], td);
    EXPECT_THROW((d["four"] = 4L), std::logic_error);
  }
  {
    dynamic d = ordered_map_t{{std::string{"foo"}, std::string{"xyz"}},
                              {std::string{"bar"}, 4L}};
    dynamic key = std::string("foo");
    d[key] = std::string("abc");
    dynamic td = std::string("abc");
    EXPECT_EQ(d[key], td);
  }
  {
    dynamic d = unordered_map_t{{std::string{"foo"}, std::string{"xyz"}},
                                {std::string{"bar"}, 4L}};
    d["foo"] = std::string("abc");
    dynamic td = std::string("abc");
    EXPECT_EQ(d["foo"], td);
  }
}

TEST(Dynamic, Object) {
  {
    auto vec = vector_dynamic_t(1L);
    auto keyVec = std::vector<std::string>({"one"});
    dynamic d = std::make_pair(&keyVec, vec);
    EXPECT_TRUE(d.isObject());
  }
  {
    dynamic d = unordered_map_t{};
    EXPECT_TRUE(d.isObject());
  }
  {
    dynamic d = ordered_map_t{};
    EXPECT_TRUE(d.isObject());
  }
  {
    dynamic d = vector_dynamic_t{};
    EXPECT_FALSE(d.isObject());
  }
  {
    dynamic d = 5L;
    EXPECT_FALSE(d.isObject());
    d = std::string("xyz");
    EXPECT_FALSE(d.isObject());
  }
}

TEST(Dynamic, Array) {
  {
    dynamic d = vector_dynamic_t{};
    EXPECT_TRUE(d.isArray());
  }
  {
    dynamic d = std::vector<int64_t>{};
    EXPECT_TRUE(d.isArray());
  }
  {
    dynamic d = std::vector<folly::StringPiece>{};
    EXPECT_TRUE(d.isArray());
  }
  {
    auto vec = vector_dynamic_t(1L);
    auto keyVec = std::vector<std::string>({"one"});
    dynamic d = std::make_pair(&keyVec, vec);
    EXPECT_FALSE(d.isArray());
  }
  {
    dynamic d = unordered_map_t{};
    EXPECT_FALSE(d.isArray());
  }
  {
    dynamic d = ordered_map_t{};
    EXPECT_FALSE(d.isArray());
  }
  {
    dynamic d = 5L;
    EXPECT_FALSE(d.isArray());
    d = std::string("xyz");
    EXPECT_FALSE(d.isArray());
  }
}

TEST(Dynamic, EmptyVectorIterationTest) {
  dynamic d = vector_pair_t();
  auto it = d.begin();
  EXPECT_TRUE(it == d.end());
  d = unordered_map_t();
  auto it2 = d.begin();
  EXPECT_TRUE(it2 == d.end());
  d = ordered_map_t();
  auto it3 = d.begin();
  EXPECT_TRUE(it3 == d.end());
}

TEST(Dynamic, IteratorTest) {
  {
    // Vector pair
    vector_dynamic_t vec =
        vector_dynamic_t({1L, static_cast<std::string>("foo"), 3L});
    std::vector<std::string> keyVec =
        std::vector<std::string>({"one", "two", "three"});
    dynamic d = std::make_pair(&keyVec, vec);
    EXPECT_TRUE(d.isIterable());
    auto it = d.begin();
    auto end = d.end();
    int i = 0;
    for (;it != end; ++it,++i) {
      EXPECT_TRUE((*it).first == keyVec[i]);
      EXPECT_TRUE((*it).second.get() == vec[i]);

      EXPECT_TRUE(it->first == keyVec[i]);
      EXPECT_TRUE(it->second.get() == vec[i]);
    }
    EXPECT_TRUE(i == vec.size());
  }
  {
    // Ordered map
    dynamic d = ordered_map_t{{"foo", std::string{"xyz"}}, {"bar", 4L}};
    EXPECT_TRUE(d.isIterable());
    auto it = d.begin();
    auto end = d.end();
    auto mIt = d.getRef<ordered_map_t>().begin();
    for (;it != end; ++it,++mIt) {
      EXPECT_TRUE((*it).first == (*mIt).first);
      EXPECT_TRUE((*it).second.get() == (*mIt).second);
    }
    EXPECT_TRUE(mIt == d.getRef<ordered_map_t>().end());
  }
  {
    // Unordered map
    dynamic d = unordered_map_t{{"foo", std::string{"xyz"}}, {"bar", 4L}};
    EXPECT_TRUE(d.isIterable());
    auto it = d.begin();
    auto end = d.end();
    auto mIt = d.getRef<unordered_map_t>().begin();
    for (;it != end; ++it,++mIt) {
      EXPECT_TRUE((*it).first == (*mIt).first);
      EXPECT_TRUE((*it).second.get() == (*mIt).second);
    }
    EXPECT_TRUE(mIt == d.getRef<unordered_map_t>().end());
  }
  {
    // Vector of dynamics
    dynamic d = vector_dynamic_t({std::string{"xyz"}, 4L, boost::blank()});
    EXPECT_TRUE(d.isIterable());
    auto it = d.begin();
    auto end = d.end();
    int i = 0;
    for (;it != end; ++it,++i) {
      EXPECT_TRUE((*it).first == static_cast<int64_t>(i));
      EXPECT_TRUE((*it).second.get() == d.at(i));
    }
    EXPECT_TRUE(i == d.getRef<vector_dynamic_t>().size());
  }
  {
    // Generic Iterator tests
    dynamic d1 = unordered_map_t{{"foo", std::string{"xyz"}}, {"bar", 4L}};
    auto it1 = d1.begin();
    auto it2 = d1.begin();
    auto end = d1.end();
    EXPECT_THROW(++end, std::logic_error);
    EXPECT_NE(it1, end);
    EXPECT_EQ(it1, it2);
    ++it1;
    EXPECT_NE(it1, it2);
    ++it2;
    EXPECT_EQ(it1, it2);
    ++it1;
    EXPECT_THROW(*it1, std::logic_error);
    EXPECT_THROW(++it1, std::out_of_range);
    dynamic d2 = ordered_map_t{{"hello", std::string{"xyz"}}, {"bar", 4L}};
    auto it3 = d2.begin();
    it1 = d1.begin();
    EXPECT_NE(it1, it3);
  }
  {
    // std::for_each
    vector_dynamic_t vec =
        vector_dynamic_t({static_cast<std::string>("one"),
                                      static_cast<std::string>("two"),
                                      static_cast<std::string>("three")});
    std::vector<std::string> keyVec =
        std::vector<std::string>({"one", "two", "three"});
    dynamic d = std::make_pair(&keyVec, vec);

    std::for_each(d.begin(), d.end(),
                  [](const DynIterValue& pair) {
                    EXPECT_TRUE(pair.first == pair.second.get());
                  });
  }
  {
    // Range for loops
    vector_dynamic_t vec =
        vector_dynamic_t({static_cast<std::string>("one"),
                                      static_cast<std::string>("two"),
                                      static_cast<std::string>("three")});
    std::vector<std::string> keyVec =
        std::vector<std::string>({"one", "two", "three"});
    dynamic d = std::make_pair(&keyVec, vec);
    for (const auto& pair : d) {
      EXPECT_TRUE(pair.first == pair.second.get());
    }
  }
  {
    // Invalid types
    dynamic d = std::string("hello");
    EXPECT_FALSE(d.isIterable());
    EXPECT_THROW(d.begin(), std::logic_error);
    d = folly::StringPiece("hello");
    EXPECT_FALSE(d.isIterable());
    EXPECT_THROW(d.begin(), std::logic_error);
    d = static_cast<int64_t>(1);
    EXPECT_FALSE(d.isIterable());
    EXPECT_THROW(d.begin(), std::logic_error);
    d = 3.14159;
    EXPECT_FALSE(d.isIterable());
    EXPECT_THROW(d.begin(), std::logic_error);
    d = boost::blank();
    EXPECT_FALSE(d.isIterable());
    EXPECT_THROW(d.begin(), std::logic_error);
    d = std::vector<int64_t>({1,2,3});
    EXPECT_FALSE(d.isIterable());
    EXPECT_THROW(d.begin(), std::logic_error);
  }
}

TEST(DynamicMerge, ints) {
  dynamic d1 = unordered_map_t{{"a", 10L}};
  dynamic d2 = unordered_map_t{{"a", 20L}};
  dynamic expected = unordered_map_t{{"a", 30L}};
  d1.merge(d2);
  EXPECT_EQ(expected, d1);
}

TEST(DynamicMerge, array) {
  dynamic d1 = unordered_map_t{{"a", vector_dynamic_t{{10L}}}};
  dynamic d2 = unordered_map_t{{"a", vector_dynamic_t{{20L}}}};
  dynamic expected = unordered_map_t{{"a", vector_dynamic_t{{10L, 20L}}}};
  d1.merge(d2);
  EXPECT_EQ(expected, d1);
}

TEST(DynamicMerge, maps) {
  dynamic d1 = unordered_map_t{{"a", ordered_map_t{{"count", 10L}}}};
  dynamic d2 = unordered_map_t{{"a", ordered_map_t{{"count", 20L}}}};
  dynamic d3 = unordered_map_t{{"a", ordered_map_t{{"count", 30L}}}};
  dynamic expected = unordered_map_t{{"a",
    vector_dynamic_t{{
      ordered_map_t{{"count", 10L}},
      ordered_map_t{{"count", 20L}},
      ordered_map_t{{"count", 30L}},
    }}
  }};
  d1.merge(d2);
  d1.merge(d3);
  EXPECT_EQ(expected, d1);
}

TEST(DynamicDotProduct, basic) {
  dynamic d1 = 1L;
  dynamic d2 = 1.0;
  double expected = 1.0;
  EXPECT_EQ(1L, d1 * d1);
  EXPECT_EQ(expected, d1 * d2);
  EXPECT_EQ(expected, d2 * d1);
  EXPECT_EQ(expected, d2 * d2);
}

TEST(DynamicDotProduct, vector) {
  dynamic d1 = vector_dynamic_t{1.0, 2.0, 3.0};
  dynamic d2 = vector_dynamic_t{1.0, 2.0, 3.0};
  dynamic d3 = vector_dynamic_t{1.0, 2.0};
  dynamic d4 = vector_dynamic_t{};
  double expected = 14.0;
  EXPECT_EQ(expected, d1 * d2);
  EXPECT_THROW(d1 * d3, std::logic_error);
  EXPECT_THROW(d1 * d4, std::logic_error);
  EXPECT_EQ(0.0, d4 * d4);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  auto ret = RUN_ALL_TESTS();
}
