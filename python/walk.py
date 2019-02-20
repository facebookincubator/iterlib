# Copyright (c) 2016-present, Facebook, Inc. All rights reserved.

"""This module contains various walkers to traverse the lazy map.

   Utilities to walk only the leaves, modify them, insert an interator
   above, materialzie all the lazy operators, print values and print types.
"""
import asyncio
import collections
import itertools
import inspect
import re
import types
from datetime import datetime
from inspect import isasyncgen, iscoroutine, isfunction, isgenerator, isasyncgenfunction
from typing import Any, AsyncGenerator, ItemsView, List, Optional, Set, Union

from item import IDKEY, Item
from resolve import async_resolve_field, resolve_field
from visitor import Visitor


class EdgeContext:
    pass


class ViewModel:
    pass


class Edge:
    pass


class PaginatedEdge:
    pass


def isawaitable(v) -> bool:
    return asyncio.iscoroutinefunction(v) or inspect.isasyncgenfunction(v)


async def is_leaf(d):
    items = (
        await resolve_parallel_dict(d)
        if isinstance(d, dict)
        # Resolve only visited edges and no keys
        else await resolve_parallel_view_model(d, None, set())
    )
    for __, v in items:
        if isinstance(v, PaginatedEdge):
            v = v.edges
        if isinstance(v, list):
            for i in v:
                if isinstance(i, ViewModel):
                    return False
                if isinstance(i, dict) and IDKEY in i:
                    return False
    return True


async def _walk(parent, key, d):  # noqa - ignore function is too complex
    if isinstance(d, AsyncGenerator):
        d_list = [i async for i in d]  # TODO: Optimize
        resolved = await resolve_parallel_iterable(d_list)
        async for g in _walk(parent, key, resolved):
            yield g
    elif isinstance(d, dict) or isinstance(d, ViewModel):
        if await is_leaf(d):
            yield (parent, key, d)
        else:
            items = (
                await resolve_parallel_dict(d)
                if isinstance(d, dict)
                # Resolve only visited edges and no keys
                else await resolve_parallel_view_model(d, None, set())
            )
            for k, v in items:
                if isinstance(v, dict):
                    parent = v
                elif isinstance(v, ViewModel):
                    parent = v
                else:
                    parent = d
                if isawaitable(v) or callable(v):
                    v = await async_resolve_field(v)
                # str needs a special case vs other primitive types which
                # are handled in the fall through case. This is mainly
                # because str is also a collections.Iterable
                if isinstance(v, str):
                    continue
                if isinstance(v, collections.Iterable) or isinstance(v, PaginatedEdge):
                    async for g in _walk(parent, k, v):
                        yield g
    elif isinstance(d, str):
        yield (parent, key, d)
    elif isinstance(d, collections.Iterable):
        for v in d:
            async for g in _walk(parent, key, v):
                yield g
    elif isinstance(d, PaginatedEdge):
        for v in d.edges:
            async for g in _walk(parent, key, v):
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


async def leaf_it(d):
    """Similar to walk above, but doesn't provide reference to
       parent"""
    async for _parent, _key, leaf in _walk({}, None, d):
        yield leaf


def _path_walk(path, d):
    if isinstance(d, dict):
        if IDKEY in d:
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


# For isinstance
primitive = (str, int, bool, float, datetime)
# For static type checking
Primitive = Union[str, int, bool, float, datetime]
# This should be insync with what json.dumps() accepts.
# Also see util/json.py
Tree = Union[Item, collections.Iterable, Primitive, None]


INDENT = "  "
WALK_LIMIT = 5  # max number of items in dict or values in iterator to print


def print_walk(d, indent=0):  # noqa - ignore print walk is too complex
    """Dumps a lazy dictionary without expanding
       any lazy data structures such as iterators, generators
    """
    # workaround to suppress 'print used in library context' lint
    # instead of suppressing on every line
    log = print

    log(INDENT * indent, end="")
    from query import (
        Query,
    )  # noqa - inner import as query depends on walk to walk query

    if isinstance(d, primitive) or d is None or isinstance(d, Query):
        log(d)
        return d
    elif isfunction(d):
        # print <function AbstractSyntaxTreeVisitor.visit_project.<locals>._func at 0x0> as PROJECT
        match = re.match(r"AbstractSyntaxTreeVisitor\.visit_(\w+)", d.__qualname__)
        if match:
            log(match.group(1).upper())
        else:
            log(d)
        return d
    elif isinstance(d, ViewModel) or isinstance(d, dict):
        if isinstance(d, ViewModel):
            log(f"{d.__class__.__module__}{d.__class__.__name__}")
        else:
            log("{")
        for k, v in itertools.islice(d.items(), WALK_LIMIT):
            log(INDENT * (indent + 1), end="")
            log(k)
            print_walk(v, indent + 2)
        if len(d) > WALK_LIMIT:
            log(INDENT * (indent + 1), end="")
            log("...")
        log(INDENT * indent, end="")
        log("}")
        return d
    elif iscoroutine(d) or isgenerator(d) or isasyncgen(d):
        log(d)
        log(INDENT * indent, end="")
        log("{")
        if iscoroutine(d):
            frame = d.cr_frame
        elif isgenerator(d):
            frame = d.gi_frame
        else:  # isasyncgen(d)
            frame = d.ag_frame
        for k, v in frame.f_locals.items():
            log(INDENT * (indent + 1), end="")
            log(k)
            print_walk(v, indent + 2)
        log(INDENT * indent, end="")
        log("}")
        return d
    elif isinstance(d, collections.Iterable):
        d, tmp_it = itertools.tee(d)
        log("[")
        for v in itertools.islice(tmp_it, WALK_LIMIT):
            print_walk(v, indent + 1)
        if any(True for _ in tmp_it):
            log(INDENT * (indent + 1), end="")
            log("...")
        log(INDENT * indent, end="")
        log("]")
        return d
    else:
        raise Exception("Invalid type: " + str(type(d)))


async def materialize_walk(d) -> Tree:
    """Returns a materialized dictionary expanding
       any lazy data structures such as iterators, generators
       encountered.
    """
    return await _materialize_walk(d)


async def materialize_walk_obj(d) -> Tree:
    """Returns an object graph expanding
       any lazy data structures such as iterators, generators
       encountered.
    """
    return await _materialize_walk_obj(d)


def is_lazy(d):
    return isawaitable(d) or isinstance(d, types.GeneratorType)


async def resolve_parallel_view_model(
    d, edges: Optional[Set[str]] = None, keys: Optional[Set[str]] = None
) -> ItemsView:
    edge_set = set(d.__visited_edges__) if edges is None else set(edges)
    if keys is not None:
        keys |= edge_set
    return await resolve_parallel_dict(d, edge_set, keys)


async def resolve_parallel_dict(
    d, edges: Optional[Set[str]] = None, keys: Optional[Set[str]] = None
) -> ItemsView:
    "If edges is not None, resolve only the keys in that set. Otherwise resolve everything"
    awaitables = []
    non_awaitables = []
    if isinstance(d, ViewModel):
        if keys is None:
            keys = await ViewModel.async_project_properties(d)
        items = ((k, d[k]) for k in keys)
        if edges is None:
            edges = set(d.keys())
    else:
        items = d.items()
        if edges is None:
            edges = d.keys() | set()
    for k, v in items:
        if is_lazy(v) and k in edges:
            awaitables.append((k, v))
        else:
            non_awaitables.append((k, v))
    out = {k: v for k, v in non_awaitables}
    if awaitables:
        _awaitables = (v for _, v in awaitables)
        results = await asyncio.gather(*_awaitables)
        for (k, _), result in zip(awaitables, results):
            out[k] = result
    return out.items()


async def resolve_parallel_iterable(d):
    awaitables = []
    non_awaitables = []
    for i, v in enumerate(d):
        if is_lazy(v):
            awaitables.append((i, v))
        else:
            non_awaitables.append((i, v))
    if awaitables:
        _awaitables = (v for _, v in awaitables)
        results = await asyncio.gather(*_awaitables)
        awaitables = [(i, r) for (i, v), r in zip(awaitables, results)]
    return (v for (_, v) in sorted(awaitables + non_awaitables))


async def _materialize_walk_obj(d) -> Tree:
    """Resolves only the visited edges in an object graph. Doesn't resolve any properties"""
    if isinstance(d, ViewModel):
        # Resolve the first level of awaitables
        edge_set = set(d.__visited_edges__)
        edges = await resolve_parallel_dict(d, edge_set)
        # Resolve all edges recursively
        vals = await asyncio.gather(*(_materialize_walk_obj(v) for k, v in edges))
        for (k, _), val in zip(edges, vals):
            if k in edge_set:
                d[k] = val
        return d
    elif isinstance(d, dict):
        # Resolve the first level of awaitables
        items = await resolve_parallel_dict(d)
        vals = await asyncio.gather(*(_materialize_walk_obj(v) for k, v in items))
        for (k, _), val in zip(items, vals):
            d[k] = val
        return d
    elif isinstance(d, primitive) or d is None:
        return d
    elif isinstance(d, PaginatedEdge):
        d.edges = await resolve_parallel_iterable(d.edges)
        return d
    elif isinstance(d, collections.Iterable):
        resolved = await resolve_parallel_iterable(d)
        return await asyncio.gather(
            *(val for val in (_materialize_walk_obj(v) for v in resolved) if val)
        )
    elif type(d) == types.AsyncGeneratorType:
        d_list = [i async for i in d]  # TODO: Optimize
        resolved = await resolve_parallel_iterable(d_list)
        return await asyncio.gather(
            *(val for val in (_materialize_walk_obj(v) for v in resolved) if val)
        )
    elif isawaitable(d) or callable(d):
        # TODO: Profile and optimize recursive call
        resolved = await async_resolve_field(d)
        return await _materialize_walk_obj(resolved)
    raise Exception("Invalid type: " + str(type(d)))


async def _materialize_walk(d) -> Tree:
    if isinstance(d, ViewModel) or isinstance(d, dict):
        out = Item({})
        trimmed_keys: List[Any] = []
        # Resolve the first level of awaitables
        keys = None
        if isinstance(d, ViewModel):
            if d._all_edges:
                keys = set(d.__keys__)
            else:
                keys = set(d.__visited_edges__ + d.__props__)
        items = await resolve_parallel_dict(d, keys)
        # Resolve the deeply nested awaitables
        vals = await asyncio.gather(*(_materialize_walk(v) for k, v in items))
        for (k, _), val in zip(items, vals):
            # trim keys where the val has been filtered out
            if val is not None:
                out[k] = val
            else:
                trimmed_keys.append(k)
        if trimmed_keys and isinstance(d, dict):
            for k in trimmed_keys:
                out.pop(k, None)
        return out
    elif isinstance(d, primitive) or d is None:
        return d
    elif isinstance(d, Edge):
        return d.to_json()
    elif isinstance(d, PaginatedEdge):
        d.edges = await _materialize_walk(d.edges)
        return {"edges": list(d.edges), "page_info": d.page_info.to_json()}
    elif isinstance(d, collections.Iterable):
        resolved = await resolve_parallel_iterable(d)
        return await asyncio.gather(
            *(val for val in (_materialize_walk(v) for v in resolved) if val)
        )
    elif type(d) == types.AsyncGeneratorType:
        d_list = [i async for i in d]  # TODO: Optimize
        resolved = await resolve_parallel_iterable(d_list)
        return await asyncio.gather(
            *(val for val in (_materialize_walk(v) for v in resolved) if val)
        )
    elif isawaitable(d) or callable(d):
        # TODO: Profile and optimize recursive call
        resolved = await async_resolve_field(d)
        return await _materialize_walk(resolved)
    raise Exception("Invalid type: " + str(type(d)))


def _materialize_walk_sync(d) -> Tree:
    if isinstance(d, ViewModel) or isinstance(d, dict):
        out = Item({})
        trimmed_keys: List[Any] = []
        if isinstance(d, ViewModel):
            items = ViewModel.items(d)
        else:
            items = d.items()
        for k, v in items:
            val = _materialize_walk_sync(v)
            # trim keys where the val has been filtered out
            if val is not None:
                out[k] = val
            else:
                trimmed_keys.append(k)
        if trimmed_keys and isinstance(d, dict):
            for k in trimmed_keys:
                out.pop(k, None)
        return out
    elif isinstance(d, primitive) or d is None:
        return d
    elif isinstance(d, PaginatedEdge):
        d.edges = _materialize_walk_sync(d.edges)
        return d
    elif isinstance(d, collections.Iterable):
        return [val for val in (_materialize_walk_sync(v) for v in d) if val]
    elif isinstance(d, types.GeneratorType):
        return _materialize_walk_sync(list(d))
    elif isawaitable(d) or callable(d):
        return resolve_field(d)
    raise Exception("Invalid type: " + str(type(d)))


class PrintASTVisitor(Visitor):

    INDENT = "    "  # four spaces

    def __init__(self, id1s):
        self.tree = ""
        self.indent = 0
        self.visited = set()

    async def visit_leaf(self, query):
        self.tree += self.INDENT * self.indent
        self.tree += str(query)
        self.tree += "\n"
        self.indent += 1
        if query in self.visited:
            # Prevent infinite recursion
            self.indent -= 1
            return
        else:
            self.visited.add(query)
        for q in query.edges:
            await self.visit(q)
        self.indent -= 1

    async def _visit_one_child(self, query):
        self.tree += self.INDENT * self.indent
        self.tree += str(query)
        self.tree += "\n"
        self.indent += 1
        await self.visit(query.child)
        self.indent -= 1

    async def _visit_multiple_children(self, query):
        self.tree += self.INDENT * self.indent
        self.tree += str(query)
        self.tree += "\n"
        self.indent += 1
        for q in query.queries:
            await self.visit(q)
        self.indent -= 1

    visit_skip = _visit_one_child
    visit_take = _visit_one_child
    visit_project = _visit_one_child
    visit_aggregate = _visit_one_child
    visit_edge = _visit_one_child

    visit_union = _visit_multiple_children
    visit_branched_union = _visit_multiple_children
