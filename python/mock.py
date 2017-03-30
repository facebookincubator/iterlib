#!/usr/bin/env python3.5
# Copyright (c) 2016-present, Facebook, Inc. All rights reserved.


import argparse
import execute
import random
import validate

from item import pprint_json


class MockExecutor(execute.AbstractSyntaxTreeVisitor):
    """Driver for test purposes which uses a formula to traverse
       the graph. A real driver will use a key value storage or
       a database to implement similar functionality.
    """

    def driver_obj(self, id_list):
        return [int(x) + 1 for x in id_list]

    def driver_assoc(self, assoc, id):
        l = []
        for x in range(id * 10, id * 10 + 3):
            l.append({':id': x, 'name': 'id%d' % x,
                      'age': random.choice([16, 17, 18])})
        return l


def test_execute(expr):
    e = validate.validate(expr)
    visitor = MockExecutor(None)
    visitor.visit(e['query'])
    return visitor.iter


def test_execute_hier(expr):
    """This one produces an ordered dict instead
       of a flat list of Items.
    """
    e = validate.validate(expr)
    visitor = MockExecutor(None)
    visitor.visit(e['query'])
    return visitor.root

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("--verbose", help="increase output verbosity",
                        action="store_true")
    parser.add_argument("-t", "--tree", help="hierarchical tree output",
                        action="store_true")
    args, query = parser.parse_known_args()
    if args.tree:
        tree = test_execute_hier(query[0])
        pprint_json(execute.materialize_walk(tree))
    else:
        tree = test_execute(query[0])
        for i in tree:
            pprint_json(execute.materialize_walk(execute.leaf_it(i)))
