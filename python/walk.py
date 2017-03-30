# Copyright (c) 2016-present, Facebook, Inc. All rights reserved.

"""This module contains various walkers to traverse the lazy map.

   Utilities to walk only the leaves, modify them, insert an interator
   above, materialzie all the lazy operators, print values and print types.
"""

import collections
from item import Item


def is_leaf(d):
    has_child = False
    for k, v in d.items():
        if isinstance(v, list):
            for i in v:
                if ":id" in i:
                    has_child = True
                    break
    return not has_child


def _walk(parent, key, d):
    if isinstance(d, dict):
        if is_leaf(d):
            yield (parent, key, d)
        else:
            for k, v in d.items():
                if isinstance(v, dict):
                    parent = d[k]
                else:
                    parent = d
                if not isinstance(v, collections.Iterable) or \
                        isinstance(v, str):
                    continue
                for g in _walk(parent, k, v):
                    yield g
    elif isinstance(d, str):
        yield (parent, key, d)
    elif isinstance(d, collections.Iterable):
        for v in d:
            for g in _walk(parent, key, v):
                yield g
    else:
        yield (parent, key, d)


def walk(d):
    """Walk the dictionary, yielding tuples of (root, parent, key, leaf).

       This is particularly useful if you want to iterate over
       leaves, while also expanding the tree by adding more children
    """
    for parent, key, leaf in _walk({}, None, d):
        yield (d, parent, key, leaf)


def leaf_it(d):
    """Similar to walk above, but doesn't provide reference to
       parent"""
    for parent, key, y in _walk({}, None, d):
        yield y


def _path_walk(path, d):
    if isinstance(d, dict):
        if ":id" in d:
            yield (path, d)
        else:
            for k, v in d.items():
                for g in _path_walk(path + [k], v):
                    yield g
    elif isinstance(d, str):
        yield (path, d)
    elif isinstance(d, collections.Iterable):
        for v in d:
            for g in _path_walk(path, v):
                yield g
    else:
        yield (path, d)


def dict_to_item(d):
    if isinstance(d, dict):
        return Item({k: dict_to_item(v) for k, v in d.items()})
    elif isinstance(d, collections.Iterable) and not isinstance(d, str):
        return list(map(dict_to_item, d))
    else:
        return d


def path_it(d):
    """Similar to leaf_it above, but yields a path as well"""
    for p in _path_walk([], d):
        yield p


def materialize_walk(d):
    """Returns a materialized dictionary expanding
       any lazy data structures such as iterators, generators
       encountered.
    """
    if isinstance(d, dict):
        out = Item({})
        trim_me = False
        for k, v in d.items():
            val = materialize_walk(v)
            # trim keys where the val has been filtered out
            if val:
                out[k] = val
            else:
                trim_me = True
        if trim_me:
            out.clear()
        return out
    elif isinstance(d, str):
        return d
    elif isinstance(d, collections.Iterable):
        out = []
        for v in d:
            val = materialize_walk(v)
            if val:
                out.append(val)
        return out
    else:
        return d


def print_types_walk(d):
    """Walk the lazy dict and print types."""
    if isinstance(d, dict):
        out = Item({})
        for k, v in d.items():
            print("{ %s : " % k, end="")
            print_types_walk(v)
            print("}")
    elif isinstance(d, collections.Iterable):
        print(type(d), end="")
        # TODO: descend into nested iterators such as reverse+limit
    else:
        print(type(d), end="")
