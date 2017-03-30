#!/usr/bin/env python3.5
# Copyright (c) 2016-present, Facebook, Inc. All rights reserved.


import argparse
import execute
import json
import leveldb
import validate

from item import pprint_json


class KVExecutor(execute.AbstractSyntaxTreeVisitor):
    """Driver for test purposes which uses a formula to traverse
       the graph. A real driver will use a key value storage or
       a database to implement similar functionality.
    """

    def __init__(self):
        # execute.AbstractSyntaxTreeVisitor.__init__(self)
        self.id1s = None
        self.obj = leveldb.LevelDB('./db')
        self.assoc = leveldb.LevelDB('./assoc')

    def _lookup_obj(self, id):
        """ Key format: <id> => json-blob """
        key = bytes(str(id), 'utf-8')
        val = self.obj.Get(key)
        return val.decode('utf-8')

    def _lookup_assoc(self, id, assoc):
        """ Key format: <id1, type, time, id2> => data"""
        key = str(id) + ':' + str(assoc) + ':'
        key = bytes(key, 'utf-8')
        it = self.assoc.RangeIter(key)
        return it

    def driver_obj(self, id_list):
        return [json.loads(self._lookup_obj(x)) for x in id_list]

    def driver_assoc(self, assoc, id):
        l = []
        for k, v in self._lookup_assoc(id, assoc):
            k = k.decode('utf-8')
            v = v.decode('utf-8')
            id1, atype, atime, id2 = map(int, k.split(':'))
            # TODO: handle this in RangeIter using a prefix scan
            if id1 != id or atype != assoc:
                break
            l.append({':id': id2, ':time': atime, 'data': v})
        return l


def test_execute(expr):
    e = validate.validate(expr)
    visitor = KVExecutor()
    visitor.visit(e['query'])
    return visitor.iter


def test_execute_hier(expr):
    """This one produces an ordered dict instead
       of a flat list of Items.
    """
    e = validate.validate(expr)
    visitor = KVExecutor()
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
