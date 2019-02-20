import itertools
from typing import Dict, List, Optional, Tuple, Type, Union

from async_test import wait_for
from execute import AbstractSyntaxTreeVisitor
from walk import (
    EdgeContext,
    ViewModel,
    PrintASTVisitor,
    Tree,
    materialize_walk,
    materialize_walk_obj,
    print_walk,
)
from enum import IntEnum


class QueryableOp(IntEnum):
    INVALID = 0
    PROJECT = 1
    WHERE = 2
    TAKE = 3
    COUNT = 4
    LEAF = 5
    COND = 6
    EDGE = 7
    UNION = 8
    BRANCHED_UNION = 9
    NEST = 10
    LET = 11
    ORDER_BY = 12


SwitchType = Union[Tuple, Tuple[int, "Query"]]


class Query:
    """
    A Query doesn't have an iterator attached to it
    """

    OP: QueryableOp = QueryableOp.INVALID
    EDGE_NAME_TO_QUERY_TYPE: Dict[str, Type["Query"]] = {}

    def __init__(
        self,
        child: "Query" = None,
        ids: Optional[List[int]] = None,
        items: Optional[List[ViewModel]] = None,
    ) -> None:
        # Boiler plate code that bloats subclasses
        self.parent_edge: Optional[EdgeQueryable] = None
        self._unbound_class = self.__class__
        # At least one of ids or items should be true, but not both.
        # One exception is unbound queries, where the ids/items come
        # from a child query.
        assert (bool(items) ^ bool(ids)) or child
        self._items = items
        if self._items:
            # pyre-fixme[16]: `ViewModel` has no attribute `id`.
            self.ids = [item.id for item in self._items]
        else:
            self.ids = ids or []

        if child:
            self.child: "Query" = child
            self._unbound_class: Type[Query] = child._unbound_class
        self.edges: List["Query"] = []
        self.visited = False
        # Only one of the two below can be true
        self._as_dict = False
        self._as_list = False
        self._to_json = False

    def __str__(self) -> str:
        query_name = str(self.OP)[len("QueryableOp.") :]
        if self.OP == QueryableOp.LEAF:
            return f"{query_name} ({self.__class__.__name__})"
        else:
            return query_name

    # The meaning of child vs parent depends on your perspective
    # Query Author writes: a.b().c() where a is the parent of b etc
    # AST construction: a.b().c() where b is the child of a etc
    #
    # Since this API is exposed to Query Author, we use their perspective
    def parent(self) -> "Query":
        if len(self.edges) == 0:
            self.child.edges.append(self)
        else:
            # TODO: Handle the case where len(self.edges) > 1
            self.child.edges.append(self.edges[0])
        return self.child

    def project(self, projector) -> "ProjectQueryable":
        return ProjectQueryable(self, projector)

    def where(self, predicate) -> "WhereQueryable":
        return WhereQueryable(self, predicate)

    def take(self, count: int = 1) -> "TakeQueryable":
        return TakeQueryable(self, count)

    def count(self) -> "CountQueryable":
        return CountQueryable(self)

    def nest(self, key: str) -> "NestQueryable":
        return NestQueryable(self, key)

    def let(self, old: str, new: str) -> "LetQueryable":
        return LetQueryable(self, old, new)

    def order_by(self, key) -> "OrderbyQueryable":
        return OrderbyQueryable(self, key)

    def cond(self, key=":type", switch: SwitchType = ()) -> "CondQueryable":
        return CondQueryable(self, key, switch)

    def edge(self, edge_name: str, edge_ctx: EdgeContext = None) -> "EdgeQueryable":
        if edge_name in self.EDGE_NAME_TO_QUERY_TYPE:
            self._unbound_class = self.EDGE_NAME_TO_QUERY_TYPE[edge_name]
        return EdgeQueryable(self, edge_name, self._unbound_class(self), edge_ctx)

    def union(self, *queries) -> "UnionQueryable":
        return UnionQueryable(self, *queries)

    def as_dict(self) -> "Query":
        self._as_dict = True
        return self

    def as_list(self) -> "Query":
        self._as_list = True
        return self

    def to_json(self) -> "Query":
        self._to_json = True
        return self

    def get_ids(self) -> List[int]:
        if self.ids:
            return self.ids
        if self.child:
            return self.child.get_ids()
        return []

    def get_keys(self) -> List[str]:
        return [str(x) for x in self.get_ids()]

    def debug(self, indent=1):
        visitor = AbstractSyntaxTreeVisitor([])
        wait_for(visitor.visit_child(self))
        print_walk(visitor.root, indent)

    def send(self) -> Tree:
        visitor = AbstractSyntaxTreeVisitor([])
        wait_for(visitor.visit_child(self))
        if self._to_json:
            r = wait_for(materialize_walk(visitor.root))
        else:
            r = wait_for(materialize_walk_obj(visitor.root))
        if not self._as_list and not self._as_dict:
            return r
        # r is a list of dicts, but static checkers don't know that
        r = next(itertools.islice(r[0].values(), 0, 1))
        if self._as_dict:
            return {k: v for k, v in zip(self.get_keys(), r)}
        else:
            return r

    async def async_debug(self, indent=1):
        visitor = AbstractSyntaxTreeVisitor([])
        await visitor.visit_child(self)
        print_walk(visitor.root, indent)

    async def async_send(self) -> Tree:
        visitor = AbstractSyntaxTreeVisitor([])
        await visitor.visit_child(self)
        if self._to_json:
            r = await materialize_walk(visitor.root)
        else:
            r = await materialize_walk_obj(visitor.root)
        if not self._as_list and not self._as_dict:
            return r
        # r is a list of dicts, but static checkers don't know that
        # pyre-fixme[16]: Optional type has no attribute `__getitem__`.
        r = next(itertools.islice(r[0].values(), 0, 1))
        if self._as_dict:
            return {k: v for k, v in zip(self.get_keys(), r)}
        else:
            return r

    def __await__(self) -> Tree:
        return self.async_send().__await__()

    def dump(self) -> str:
        visitor = PrintASTVisitor([])
        wait_for(visitor.visit(self))
        return visitor.tree

    def batch_resolve_objs(self) -> List[Dict[str, List[ViewModel]]]:
        return [{str(None): [o for o in (self.resolve_obj(i) for i in self.ids) if o]}]

    @staticmethod
    def resolve_obj(_id: int, edge: str = "") -> Optional[ViewModel]:
        return None

    async def iter(self) -> Union[List[ViewModel], Dict[str, List[ViewModel]]]:
        if not self.parent_edge:
            yield self.batch_resolve_objs()[0]
        else:
            resolved = await asyncio.gather(
                *[
                    self.resolve(
                        item, self.parent_edge.edge_name, self.parent_edge._ctx
                    )
                    async for item in self._items
                ]
            )
            for view_models in resolved:
                yield view_models

    async def resolve(self, item, key, edge_ctx):
        if isinstance(item, ViewModel):
            return await item.resolve_edge(key, edge_ctx)
        else:
            return item


class ProjectQueryable(Query):
    OP = QueryableOp.PROJECT

    def __init__(self, child: Query, projector) -> None:
        super(ProjectQueryable, self).__init__(child)
        self.projector = projector

    def __str__(self) -> str:
        return Query.__str__(self) + " " + str(self.projector)


class WhereQueryable(Query):
    OP = QueryableOp.WHERE

    def __init__(self, child: Query, predicate) -> None:
        super(WhereQueryable, self).__init__(child)
        self.predicate = predicate

    def __str__(self) -> str:
        return Query.__str__(self) + " " + str(self.predicate)


class TakeQueryable(Query):
    OP = QueryableOp.TAKE

    def __init__(self, child: Query, count: int) -> None:
        super(TakeQueryable, self).__init__(child)
        self._count: int = count

    def __str__(self) -> str:
        return Query.__str__(self) + " " + str(self._count)


class CountQueryable(Query):
    OP = QueryableOp.COUNT

    def __init__(self, child: Query) -> None:
        super().__init__(child)


class CondQueryable(Query):
    OP = QueryableOp.COND

    def __init__(self, child: Query, key: str, switch: SwitchType) -> None:
        super(CondQueryable, self).__init__(child)
        self.key = key
        # pair of <type, Query>
        self.switch = switch

    def __str__(self) -> str:
        return Query.__str__(self) + " " + self.key + " " + str(self.switch)


class UnionQueryable(Query):
    OP = QueryableOp.UNION

    def __init__(self, child, *queries):
        super(UnionQueryable, self).__init__(child)
        self.queries = queries


class BranchedUnionQueryable(Query):
    OP = QueryableOp.BRANCHED_UNION

    def __init__(self, child, *queries):
        super(BranchedUnionQueryable, self).__init__(child)
        self.queries = queries


class EdgeQueryable(Query):
    OP = QueryableOp.EDGE

    # q1.edge('foo').q2
    #
    # constructs the following AST
    # q2 -- (parent_edge) --> EdgeQueryable('foo') -- (child) --> q1
    def __init__(
        self, child: Query, edge_name: str, unbound: Query, edge_ctx: EdgeContext
    ) -> None:
        super(EdgeQueryable, self).__init__(child)
        self._unbound = unbound
        unbound.parent_edge = self
        self.edge_name = edge_name
        self._ctx = edge_ctx

    def __str__(self) -> str:
        return Query.__str__(self) + " " + self.edge_name + " " + str(self._unbound)


class NestQueryable(Query):
    OP = QueryableOp.NEST

    def __init__(self, child: Query, key: str) -> None:
        super().__init__(child)
        self.key = key


class LetQueryable(Query):
    OP = QueryableOp.LET

    def __init__(self, child: Query, old: str, new: str) -> None:
        super().__init__(child)
        self.old = old
        self.new = new


class OrderbyQueryable(Query):
    OP = QueryableOp.ORDER_BY

    def __init__(self, child: Query, key) -> None:
        super(OrderbyQueryable, self).__init__(child)
        self.key = key

    def __str__(self) -> str:
        return Query.__str__(self) + " " + str(self.key)
