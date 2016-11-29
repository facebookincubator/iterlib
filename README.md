This module implements a dynamic similar to folly::dynamic in that
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

* Implement zero-copy iterators on top of the dynamic

* Faster conversion to json. Right now, requires conversion to folly::dynamic
  and then to json

* Better glue for language runtimes such as python or lua
