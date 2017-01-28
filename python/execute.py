#!/usr/bin/env python3.5
#
# Copyright (c) 2016-present, Facebook, Inc. All rights reserved.

import collections
import filterexpr
import heapq
import itertools
import operator
import json
import random
import sys
import validate
import visitor

from contextlib import ExitStack
from functools import reduce
from item import Item
from walk import *

def reservoir_sample(it, k):
    '''https://en.wikipedia.org/wiki/Reservoir_sampling#Algorithm_R'''
    it = iter(it)
    result = []
    for i, datum in enumerate(it):
        if i < k:
            result.append(datum)
        else:
            j = random.randint(0, i-1)
            if j < k:
                result[j] = datum
    return result

def projection(keys):
   """Return a lambda that filters only the selected keys"""
   return lambda xlist: [{k : item[k] for k in keys} for item in xlist]

def project_item(keys):
   """Like projection, but operates on a single item instead of a list"""
   return lambda x: {k : x[k] for k in keys}

def rename(env):
   """Return a lambda that adds some aliases specified by env"""
   return lambda x: x.update({k : x[v] for k,v in env}) or x

def projection_cmp(keys):
   return lambda x: [x[k] for k in keys]

def flatten(iterable):
    "Flatten one level of nesting"
    return itertools.chain.from_iterable(iterable)

def count_it(iter):
    return sum(1 for _ in iter)

def intersect(its):
    source = heapq.merge(*its, reverse=True)
    return iter([k for k, g in itertools.groupby(source, project_item([":id"])) if count_it(g) == len(its)])

def union(its):
    source = heapq.merge(*its, reverse=True)
    return iter([k for k, g in itertools.groupby(source, project_item([":id"]))])

def reduce(function, iterable, initializer=None):
    it = iter(iterable)
    if initializer is None:
        value = next(it)
    else:
        value = initializer
    for element in it:
        value = function(value, element)
    return value

def merge_dicts(x, y):
    common_keys = set(x.keys()) & set(y.keys())
    z = { k : operator.add(x[k], y[k]) for k in common_keys }
    z = dict(list(x.items()) + list(y.items()) + list(z.items()))
    return z

def merge_dicts_iter(x, y):
    x = next(x)
    y = next(y)
    z = merge_dicts(x, y)
    yield z

def merge(its):
    return reduce(merge_dicts_iter, its)

# Utilities to deal with the syntactically more convenient
# (3 2 1) instead of  [{':id': 3}, {':id': 2}, {':id': 1}]
def dictify(id_list):
    for x in id_list:
        yield Item({ ":id" : x })

def ldictify(id_list):
    return list(dictify(id_list))

def undictify(dict_it):
    for i in dict_it:
        yield i[":id"]

class AbstractSyntaxTreeVisitor(visitor.Visitor):

  def __init__(self, id1s):
      self.iter = None
      self.parent_iter = None
      self.parent_key = None
      if id1s:
          self.root = { id1[":id"] : {} for id1 in id1s }
      self.id1s = id1s

  def batch(func):
      """Apply self.mapFunc to all the values in the map.
         mapFunc takes a list of Items as the argument
      """

      def _insert_parent_func(self, func):
          """ Given a recursive dict in self.iter backed by generator
              expressions, insert `func' right above the leaves.

              self.parent_iter and self.parent_key are used to
              quickly locate the parents of leaf nodes.
          """
          if not self.parent_iter:
              self.iter = map(lambda x : { k : self.mapFunc(v) for k,v in x.items() },
                              self.iter)
              self.root = self.iter
              return
          # we need to be able to iterate over self.parent_iter multiple times
          self.parent_iter, tmp_it = itertools.tee(self.parent_iter)
          for p in tmp_it:
              pkey = str(self.parent_key)
              if pkey in p:
                  old = p[pkey]
                  p[pkey] = self.mapFunc(old)

      def func_wrapper(self, query):
          func(self, query)
          _insert_parent_func(self, self.mapFunc)
      return func_wrapper

  @batch
  def visit_limit(self, query):
      self.visit(query[-1])
      limit = query[1]
      if len(query) == 4:
          offset = query[2]
      else:
          offset = 0
      # islice takes start, stop
      self.mapFunc = lambda x: itertools.islice(x, offset, limit + offset)

  @batch
  def visit_random(self, query):
      self.visit(query[2])
      self.mapFunc = lambda x: reservoir_sample(x, query[1])

  @batch
  def visit_reverse(self, query):
      self.visit(query[1])
      self.mapFunc = lambda x: reversed(list(x))

  @batch
  def visit_count(self, query):
      self.visit(query[1])
      self.mapFunc = lambda x: {"count": count_it(x) }

  def visit_literal(self, query):
      self.root = { str(self.id1s) : ldictify(query) }
      self.iter = iter([self.root])

  def visit_json_literal(self, query):
      self.root = json.loads(query[1])
      self.root = dict_to_item(self.root)
      self.iter = iter(self.root)

  @batch
  def visit_filter(self, query):
      self.visit(query[2])
      node = filterexpr.expr(query[1])
      #import ast
      #print(ast.dump(node))
      pred_expr = compile(node, '<string>', 'eval')
      pred = eval(pred_expr)
      self.mapFunc = lambda x : filter(pred, x)

  @batch
  def visit_project(self, query):
      self.visit(query[2])
      self.mapFunc = projection(query[1])

  def _aggregate(self):
      self.iter = leaf_it(self.iter)

  def visit_aggregate(self, query):
      self.visit(query[1])
      self._aggregate()
      self.root = Item({ None :  self.iter})
      self.iter = iter([self.root])
      self.parent_iter = None

  @batch
  def visit_orderby(self, query):
      self.visit(query[2])
      self.mapFunc = lambda x: sorted(x, key=projection_cmp(query[1]), reverse=True)

  @batch
  def visit_groupby(self, query):
      self.visit(query[2])
      self.mapFunc = lambda x: itertools.groupby(x, project_item(query[1]))

  def visit_and(self, query):
      iters = []
      for q in query[1:]:
          self.visit(q)
          self._aggregate()
          iters.append(self.iter)

      self.iter = intersect(iters)

  def visit_or(self, query):
      """ Like index_or, but works on only the :id column.
          Expects input to be sorted by id descending.
      """
      iters = []
      for q in query[1:]:
          self.visit(q)
          self._aggregate()
          iters.append(self.iter)

      self.iter = union(iters)

  def visit_index_or(self, query):
      """Merge sort. Expects input to be sorted already"""
      iters = []
      for q in query[1:]:
          self.visit(q)
          iters.append(self.iter)

      self.iter = union(iters)

  def visit_difference(self, query):
      self.visit(query[1])
      self._aggregate()
      it1 = self.iter
      self.visit(query[2])
      self._aggregate()
      it2 = self.iter
      self.iter = iter(set(list(it1)) - set(list(it2)))

  def visit_nest(self, query):
      self.visit(query[2])
      self.iter = [{ query[1] : x } for x in self.iter]

  def visit_merge(self, query):
      iters = []
      for q in query[1:]:
          self.visit(q)
          iters.append(self.iter)

      self.iter = merge(iters)

  def visit_apply(self, query):
      self.visit(query[2])
      id1s = list(leaf_it(self.root))
      visitor = self.__class__(id1s)
      visitor.root = self.root
      visitor.visit(query[1])
      self.iter = visitor.iter

  def visit_exists(self, query):
      self.visit(query[1])
      try:
          next(self.iter)
          self.iter = iter([self.id1s])
      except StopIteration:
          self.iter = []

  def visit_let(self, query):
      self.visit(query[2])
      self.iter = list(map(rename(query[1]), self.iter))

  def visit_target(self, query):
      self.visit(query[2])
      self.target = query[1]  # unused

  def visit_join(self, query):
      raise "Not implemented"

  def visit_obj(self, query):
      self.iter = self.driver_obj(query[1])

  def visit_assoc(self, query):
      if (len(query) > 2):
          self.visit(query[2])

      # TODO: investigate if eager computation here is necessary
      self.parent_iter = iter(list(leaf_it(self.root)))
      self.iter = walk(self.root)
      assoc = query[1]
      self.parent_key = assoc

      # invariant: self.iter has the list of keys we are computing assocs for
      leaves = []
      with ExitStack() as stack:
          for root, parent, k, i in self.iter:
              id_i = i[":id"]
              res = self.driver_assoc(assoc, id_i)
              stack.callback(i.__setitem__, str(assoc), res)
              leaves.append(list(undictify(res)))
      self.iter = iter([self.root])

  def driver_obj(self, id_list):
      raise "Not implemented"

  def driver_assoc(self, id):
      raise "Not implemented"

def execute(expr):
    e = validate.validate(expr)
    visitor = AbstractSyntaxTreeVisitor(None)
    visitor.visit(e['query'])
    return visitor.iter

if __name__ == '__main__':
    iter = execute(sys.argv[1])
    for i in iter:
        print(i)
