Iterlib - A zero copy iterator library
======================================

Are you trying to build functional programming style operators
on top of a key value store? Are you trying to compete with
simple in-memory stores such as memcache on throughput, while
providing richer functionality and durability? Then this library
may be of interest to you.

If you're happy with a distributed hash table or if you are happy
with improving the throughput of a spinning disk based system by
only a factor of 10, you can skip reading.

Many key value stores implement a string -> string sorted map.
Often applications want to implement "tables" (not necessarily in
the relational database sense) on top of these stores by interpreting
the strings as typed tuples based on a schema.

Iterlib provides you a generic iterator from Item -> Item. An Item
is a `iterlib::dynamic` with some "well known" attributes (such as
`id()` - object identifier and `timestamp()` - typical ordering key).
We've found it beneficial to memoize the well known attributes for
better performance on real world workloads.

Iterlib intends to provide the following composable iterators.

1. limit - skip + take
1. reverse
1. random
1. count
1. and - id() based intersection
1. or - id() based union
1. difference - id() based set difference
1. flatten - flatten nested inputs.
1. merge - id() based merge using `iterlib::dynamic::merge()`
1. project - only select certain attributes of the Item
1. nest - nest the inner iterator using a key Item.
1. let - rename a key Item
1. sorted - sorting
1. merge-sorted - merging sorted inputs
1. group-by - grouping
1. exists - emit key if the value is not null
1. filter - filter based on a predicate
1. literal - id() based literals
1. json-literal - supports Items expressed as JSON literals

Not all of these are supported yet. Some of them are committed,
some in-review and more coming.

Iterlib supports a pull based implementation to start with, but
could be generalized to support push based implementation as well.

More implementation details in [Internals docs](doc/Internals.md).

iterlib::dynamic
================

Iterlib implements a dynamic similar to folly::dynamic in that
it's dynamically typed. Unlike folly::dynamic it uses boost::variant
to select from a list of commonly used types. This comes with some
conveniences, but makes error reporting complex.

Hopefully there will be a std::variant someday that gives us the
best of both worlds.

Why another dynamic?
===================

folly::dynamic was designed to be an easy to use dynamic that
holds the in-memory representation of json objects. It was not
intended to be used in database software or query engines sitting
on top of key value stores, which is an explicit design goal for
this module.

Some comparison points below:

```
folly::dynamic d1 = true;       // 56 bytes
dynamic d2 = true;              // 32 bytes
```

This optimization matters for the common use case of PODs.

When more complex data types such as vectors, maps and nested
vectors of maps are involved, we have some evidence to indicate
that total dynamic memory allocation is smaller with this dynamic.

For a vector of maps where each map had two string keys and the value
was an int or string, we measured:

```
folly::dynamic Maximum resident set size (kbytes): 105748
This dynamic   Maximum resident set size (kbytes):  70272
```

The dynamic also allows lazy parsing optimizations where you may read
a table with many columns from a key value store, but the query only
touches a small number of columns. In those cases everything could be
parsed into a vector of StringPiece (cheap) and only the columns which
are processed can be parsed via dynamic.castTo().

Compatibility
=============

Compatibility is provided with folly::dynamic, so existing software
can be easily converted. Some of the same design principles (easy 
conversion to native types) are used here.

TODO:
====

* Faster conversion from dynamic to json. Right now, requires conversion
  to folly::dynamic and then to json

* Better glue for language runtimes such as python or lua
