# Copyright (c) 2016-present, Facebook, Inc. All rights reserved.

import ast

op_table = {
 "=" : ast.Eq(),
 "<" : ast.Lt(),
 ">" : ast.Gt(),
 ">=" : ast.GtE(),
 "<=" : ast.LtE(),
 "!=" : ast.NotEq(),
 "inset" : None,
 "contains" :  None,
 "prefix" : None,
 "range" : None,
}
 
def expr(query):
  """ Translate a query such as (= foo 3) to python ast"""
  op = op_table[query[0]]
  name = query[1]
  val = query[2]
  # TODO derive type from name, op
  if op:
      if (query[0] == '='):
          query[0] = "=="

      # The following hack should be replaced by type inference
      # of type(name) based on a schema
      is_num = True
      try:
          float(val) # covers ints, floats, but not bool
      except ValueError:
          is_num = False
          val = '"{0}"'.format(str(val))
      # The overhead of parsing can be avoided using the commented method
      # below, but it seems to create problems on python3.5
      # https://bugs.python.org/issue25555
      node = ast.parse("lambda x: x['%s'] %s %s" % (name, query[0], str(val)))
      node = ast.Expression(node.body[0].value)
      # node = ast.Expression(
      #            ast.Lambda(
      #                args=ast.arguments(args=[ast.Name(id=':id', ctx=ast.Param())], defaults=[]),
      #                body=ast.Compare(
      #                    ast.Name(id=name, ctx=ast.Load()),
      #                    [op],
      #                    [ast.Num(val)])))
  elif (query[0] == "range"):
      upper, lower = map(str, val)
      node = ast.parse("lambda x: %s <= x['%s'] < %s" % (lower, name, upper))
      node = ast.Expression(node.body[0].value)
  elif (query[0] == "inset"):
      node = ast.parse("lambda x: x['%s'] in set(%s)" % (name, str(val)))
      node = ast.Expression(node.body[0].value)
  elif (query[0] == "contains"):
      node = ast.parse("lambda x: x['%s'].find('%s') != -1" % (name, str(val)))
      node = ast.Expression(node.body[0].value)
  elif (query[0] == "prefix"):
      node = ast.parse("lambda x: x['%s'].startswith('%s')" % (name, str(val)))
      node = ast.Expression(node.body[0].value)

  node = ast.fix_missing_locations(node)
  return node
