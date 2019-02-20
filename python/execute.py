# Copyright (c) 2016-present, Facebook, Inc. All rights reserved.
import aitertools
import asyncio
import collections
import heapq
import itertools
import operator
from functools import partial
from typing import Any, AsyncGenerator, Callable, Iterable, List

from item import IDKEY, Item
from resolve import VISITED_EDGES_KEY
from visitor import Visitor
from walk import leaf_it, materialize_walk


def project_item(keys: List[str]) -> Callable[[Item], Iterable]:
    """Like projection, but operates on a single item instead of a list"""
    return lambda x: {k: x[k] for k in keys}


def rename(env: dict) -> Callable[[Item], Iterable]:
    """Return a lambda that adds some aliases specified by env"""
    return lambda x: (x.update({k: x[v] for k, v in env}), x)[1]


def projection_cmp(keys: List[str]) -> Callable[[Item], List]:
    return lambda x: [x[k] for k in keys]


async def flatten(iterable):
    "Flatten one level of nesting"
    async for aiter in aitertools.iterate(iterable):
        async for i in aitertools.iterate(aiter):
            yield i


def count_it(iter: Iterable) -> int:
    return sum(1 for _ in iter)


def intersect(its: List[Iterable]) -> Iterable:
    source = heapq.merge(*its, reverse=True)
    return (
        k
        for k, g in itertools.groupby(source, project_item([IDKEY]))
        if count_it(g) == len(its)
    )


def union(its: List[Iterable]) -> Iterable:
    source = heapq.merge(*its, reverse=True)
    return (k for k, g in itertools.groupby(source, project_item([IDKEY])))


def reduce(function, iterable, initializer=None):
    it = iter(iterable)
    if initializer is None:
        value = next(it)
    else:
        value = initializer
    for element in it:
        value = function(value, element)
    return value


def merge_dicts(x: dict, y: dict) -> dict:
    common_keys = set(x.keys()) & set(y.keys())
    z = {k: operator.add(x[k], y[k]) for k in common_keys}
    z = dict(list(x.items()) + list(y.items()) + list(z.items()))
    return z


def merge_dicts_iter(x, y):
    x = next(x)
    y = next(y)
    z = merge_dicts(x, y)
    yield z


async def union_iters(x):
    result = {}
    async for i in aitertools.iterate(x):
        if isinstance(i, dict):
            i = await materialize_walk(i)
            result = union_dicts(result, i)
        else:
            raise Exception("non dict union is not yet supported")
    return result


def union_dicts(x: dict, y: dict) -> dict:
    common_keys = set(x.keys()) & set(y.keys())
    z = {
        k: flatten([x[k], y[k]])
        for k in common_keys
        if isinstance(x[k], collections.Iterable)
        and isinstance(y[k], collections.Iterable)
    }
    # TODO: (1) the union of two dict needs to be well-defined
    #       (2) support recursive union
    return dict(list(x.items()) + list(y.items()) + list(z.items()))


def merge(its):
    return reduce(merge_dicts_iter, its)


# Utilities to deal with the syntactically more convenient
# (3 2 1) instead of  [{':id': 3}, {':id': 2}, {':id': 1}]


def dictify(id_list: Iterable[int]) -> Iterable[Item]:
    for x in id_list:
        yield Item({IDKEY: x})


def ldictify(id_list):
    return list(dictify(id_list))


def undictify(dict_it: Iterable[Item]) -> Iterable[int]:
    for i in dict_it:
        yield i[IDKEY]


def apply_func(x, map_func):
    return {k: map_func(v) for k, v in x.items()}


class FuncStack(List[AsyncGenerator]):
    pass


class AbstractSyntaxTreeVisitor(Visitor):
    def __init__(self, id1s):
        self.iter = None
        self.parent_iter = None
        self.parent_key = None
        if id1s:
            self.root = {id1[IDKEY]: {} for id1 in id1s}
        else:
            self.root = Item({None: self.iter})
        self.id1s = id1s
        self.map_func: AsyncGenerator = self._nop

    @staticmethod
    async def _nop(it):
        raise "Nop map func should never be called"

    @staticmethod
    async def compute(gen, ops: FuncStack) -> Any:
        """Compose the stack of Input generators in FuncStack into a single generator"""
        for f in ops:
            gen = f(gen)
        async for i in gen:
            yield i

    async def finish(self):
        """Setup funcs in reverse order. On entry,
           p[pkey] may be [orig, a, b, c]
           On exit, it changes to a coroutine returning c(b(a(orig)))."""
        if not self.parent_iter:
            return
        pkey = str(self.parent_key)
        async for p in self.parent_iter:
            if isinstance(p[pkey], FuncStack):
                val = p[pkey][0]
                ops = p[pkey][1:]
                p[pkey] = self.compute(val, ops)
        self.parent_key = None
        self.parent_iter = None

    def nested(func):
        """Apply self.map_func to all the values in the map.
           map_func takes a list of Items as the argument
        """

        async def _insert_parent_func(self, func):
            """ Given a recursive dict in self.iter backed by generator
                expressions, insert `func' right above the leaves.

                self.parent_iter and self.parent_key are used to
                quickly locate the parents of leaf nodes.
            """
            if not self.parent_iter or not self.parent_key:
                self.iter = aitertools.map(
                    partial(apply_func, map_func=self.map_func), self.iter
                )
                self.root = self.iter
                return
            # we need to be able to iterate over self.parent_iter multiple
            # times
            self.parent_iter, tmp_it = await aitertools.tee(self.parent_iter)
            async for p in tmp_it:
                pkey = str(self.parent_key)
                if isinstance(p[pkey], FuncStack):
                    p[pkey].append(self.map_func)
                else:
                    p[pkey] = FuncStack([p[pkey], self.map_func])

        async def func_wrapper(self, query):
            await func(self, query)
            await _insert_parent_func(self, self.map_func)

        return func_wrapper

    @nested
    async def visit_take(self, query):
        await self.visit(query.child)

        async def _func(it):
            it = aitertools.iterate(it)
            # islice takes start, stop
            async for i in aitertools.islice(it, 0, query._count):
                yield i

        self.map_func = _func

    @nested
    async def visit_skip(self, query):
        await self.visit(query.child)

        async def _func(it):
            it = aitertools.iterate(it)
            # islice takes start, stop
            async for i in aitertools.islice(it, query.skip, None):
                yield i

        self.map_func = _func

    @nested
    async def visit_project(self, query):
        await self.visit(query.child)

        async def _func(it):
            it = aitertools.iterate(it)
            async for item in it:
                if item:
                    yield {k: item[k] for k in query.projector}

        self.map_func = _func

    @nested
    async def visit_nest(self, query):
        await self.visit(query.child)

        async def _func(it):
            it = aitertools.iterate(it)
            yield {query.key: it}

        self.map_func = _func

    @nested
    async def visit_let(self, query):
        await self.visit(query.child)

        async def _func(it):
            it = aitertools.iterate(it)
            async for item in it:
                if item:
                    yield {
                        (query.new if k == query.old else k): v for k, v in item.items()
                    }
                else:
                    yield item

        self.map_func = _func

    @nested
    async def visit_count(self, query):
        await self.visit(query.child)

        async def _func(it):
            it = aitertools.iterate(it)
            count = 0
            async for _ in it:
                count += 1
            yield {"count": count}

        self.map_func = _func

    @nested
    async def visit_where(self, query):
        await self.visit(query.child)

        async def _func(it):
            it = aitertools.iterate(it)
            async for item in it:
                if query.predicate(item):
                    yield item

        self.map_func = _func

    @nested
    async def visit_order_by(self, query):
        await self.visit(query.child)

        async def _key_func(it):
            it = aitertools.iterate(it)
            heap = [
                (query.key(materialized), materialized) async for materialized in it
            ]
            heapq.heapify(heap)
            while heap:
                _, materialized = heapq.heappop(heap)
                yield materialized

        async def _async_key_func(it):
            it = aitertools.iterate(it)
            materialized = [i async for i in it]  # TODO: eliminate copy
            keys = (query.key(item) for item in materialized)
            keys = await asyncio.gather(*keys)
            heap = [(y, x) for x, y in enumerate(keys)]
            heapq.heapify(heap)
            while heap:
                _, index = heapq.heappop(heap)
                yield materialized[index]

        async def _func(it):
            if asyncio.iscoroutinefunction(query.key):
                async for item in _async_key_func(it):
                    yield item
            else:
                async for item in _key_func(it):
                    yield item

        self.map_func = _func

    async def visit_union(self, query):
        """Merge sort. Expects input to be sorted already"""
        await self.visit_child(query.child)
        iters = [self.iter]
        for q in query.queries:
            await self.visit_child(q)
            iters.append(self.iter)

        self.root = [await union_iters(flatten(iters))]
        self.iter = iter(self.root)

    async def visit_branched_union(self, query):
        """Similar to visit_union, but uses itertools.tee"""
        await self.visit(query.child)
        self.root, saved_root = await aitertools.tee(self.root)
        base_iters = await aitertools.tee(self.iter, len(query.queries))
        for q, it in zip(query.queries, base_iters):
            self.iter = it
            await self.visit_child(q)
        self.root = saved_root

        # Some merges happen naturally, for eg: due to multiple calls
        # to self.visit(q) in the leaf above populating the different
        # branches of the branched union in the same object.
        #
        # For other use cases, we may have to uncomment the code below.
        # Code to construct iters was deleted in D5502968. Please restore
        # from that diff before uncommenting.
        # self.root = merge(iters)
        # self.iter = iter([self.root])

    def _aggregate(self):
        self.iter = leaf_it(self.iter)

    async def visit_aggregate(self, query):
        await self.visit_child(query.child)
        self._aggregate()
        self.root = Item({None: self.iter})
        self.iter = iter([self.root])
        self.parent_iter = None

    async def visit_leaf(self, query):
        if len(query.edges) == 1:
            q = query.edges[0]
            query.edges = []
            await self.visit(query)
            await self.visit_child(q)
            return
        elif len(query.edges) > 1:
            # TODO(asharma): move this if clause to a qrewrite
            from query import UnionQueryable

            unionq = UnionQueryable(query, *query.edges)
            query.edges = []
            await self.visit_branched_union(unionq)
            return
        if query.visited:
            # Prevent infinite recursion
            return
        else:
            query.visited = True
        # Bound query
        if query.parent_edge:
            await self.visit_child(query.parent_edge)
        else:
            self.iter = query.iter()
            self.root = self.iter

    @staticmethod
    async def _visit_edge(it, key, query):
        async for i, res in aitertools.zip(it, query.iter()):
            i[str(key)] = res
            if VISITED_EDGES_KEY in i:
                i[VISITED_EDGES_KEY].append(key)
            yield i

    async def visit_edge(self, query):
        await self.visit_child(query.child)
        self.root, tmp_root = await aitertools.tee(self.root)
        # Tee is necessary because iteration over parent_iter
        # could leave the _visit_edge with an empty (already consumed)
        # iterator
        it1, it2 = await aitertools.tee(leaf_it(tmp_root))
        self.parent_key = query.edge_name
        # Unbound query. Fill in input from the parent
        query._unbound._items = it1
        self.parent_iter = self._visit_edge(it2, self.parent_key, query._unbound)
        self.iter = self.root

    async def visit_callable(self, func):
        self.q = func(self.q)
