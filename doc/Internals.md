This document contains documentation on the internals of iterlib.
If you're just trying to use the library, you can skip this doc.

Iterators
=========

The API is similar to `rocksdb::Iterator` except that the types of
`key()` and `value()` are an `Item`, instead of `std::string`.

Item can be thought of as a `std::tuple` of types defined by a 
schema maintained elsewhere. In many ways it behaves like one, 
e.g. by implementing lexicographic comparison, but is more 
efficient internally due to the use of the dynamic and various
zero-copy, lazy type casting techniques.

The Item also has three important methods: value(), ts() and id().
These are typically used when the Iterators are used to iterate
over a property graph stored in RocksDB as:

```
<source, edge-type, attr1, attr2, .., attrN, timestamp, dest> => metadata
```

Such iterators will yield:

key() => Item referencing id()=source. ts() and value() are invalid.
value() => Item referencing id()=dest, ts()=timestamp and value()=attrs.

We provide ItemOptimized which uses memoization to provide faster access
to id() and ts().

Of course, there is no requirement that the Iterators are used this way. 
You can ignore id() and ts() methods and use value() to encode whatever 
you want.
