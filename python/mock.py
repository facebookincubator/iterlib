#!/usr/bin/env python3.5
# Copyright (c) 2016-present, Facebook, Inc. All rights reserved.


import argparse
import asyncio
import execute
import random
import validate

from async_test import wait_for
from item import pprint_json, IDKEY
from mock_user import UserQuery
from query import CountQueryable, ProjectQueryable, WhereQueryable, NestQueryable, LetQueryable, OrderbyQueryable, TakeQueryable


class MockExecutor(execute.AbstractSyntaxTreeVisitor):
    """Driver for test purposes which uses a formula to traverse
       the graph. A real driver will use a key value storage or
       a database to implement similar functionality.
    """

    def driver_obj(self, key, id):
        return {IDKEY: id, 'name': 'id%d' % id,
                 'age': random.choice([16, 17, 18])}

    def driver_assoc(self, assoc, id):
        l = []
        for x in range(id * 10, id * 10 + 3):
            l.append({IDKEY: x, 'name': 'id%d' % x,
                      'age': random.choice([16, 17, 18])})
        return l

QUERYABLES = {
    'count': CountQueryable,
    'project': ProjectQueryable,
    'where': WhereQueryable,
    'nest': NestQueryable,
    'let': LetQueryable,
    'order_by': OrderbyQueryable,
    'take': TakeQueryable,
}


def convert(query):
    """Convert a s-expression based query into a Queryable"""
    op = query[0]
    rest = query[1:]
    queryable = None
    if type(op) == str:
        queryable = QUERYABLES.get(op, None)
    if queryable is None:
        return UserQuery(query[0])
    else:
        return queryable(convert(rest))


def test_execute(expr):
    e = validate.validate(expr)
    visitor = MockExecutor(None)
    wait_for(visitor.visit(convert(e['query'])))
    return wait_for(execute.materialize_walk(visitor.iter))


def test_execute_hier(expr):
    """This one produces an ordered dict instead
       of a flat list of Items.
    """
    e = validate.validate(expr)
    visitor = MockExecutor(None)
    wait_for(visitor.visit(convert(e['query'])))
    return wait_for(execute.materialize_walk(visitor.root))

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--verbose", help="increase output verbosity",
                        action="store_true")
    parser.add_argument("-t", "--tree", help="hierarchical tree output",
                        action="store_true")
    args, query = parser.parse_known_args()
    random.seed(100)  # so tests are deterministic
    if args.tree:
        tree = test_execute_hier(query[0])
        pprint_json(execute.materialize_walk(tree))
    else:
        tree = test_execute(query[0])
        for i in tree:
            pprint_json(execute.materialize_walk(execute.leaf_it(i)))
