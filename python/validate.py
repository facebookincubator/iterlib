#!/usr/bin/env python3.5
# Copyright (c) 2016-present, Facebook, Inc. All rights reserved.
import sys
sys.path.append("/usr/lib/python2.6/site-packages")
import jsonschema

import json
import sexp

schema = json.loads(open("queries.json").read())

def validate(exp):
    """ validate if a s-expression is a valid query"""
    s = sexp.parse(exp)
    j = {"query": s}
    jsonschema.validate(j, schema)
    return j

# todo: wrap these in a test suite
def tests():
    validate("(orderby (a  b) (1 2 3))")
    validate("(orderby (a  (b desc)) (1 2 3))")
    validate("(groupby (a  b) (1 2 3))")
    validate("(assoc 100 (1 2 3))")
    validate("(and (1 2 3) (1 2 4))")
    validate("(and (1 2 3) (1 2 4) (1 2 5))")
    validate("(apply (1 2 3) (1 2 4))")
    validate("(filter (= name foo) (1 2 3))")
    validate("(filter (>= (name1 name2) (foo 3)) (1 2 3))")
    validate("(targets (foo bar baz) (filter (= name foo) (1 2 3)))")
    validate("(let ((foo bar)) (1 2 3))")
    validate("(let ((foo1 bar1) (foo2 bar2)) (1 2 3))")

    # Negative examples
    try:
        # too many args
        validate("(assoc 100 (1 2 3) (4 5 6))")
        # wrong keyword descending
        validate("(orderby (a  (b descending)) (1 2 3))")
        # apply takes two args
        validate("(apply (1 2 3) (1 2 4) (1 2 5))")
        # let takes at least one arg
        validate("(let () (1 2 3))")
        validate("(let (1 2 3))")
        # attr pair takes exactly two args
        validate("(let ((a b c)) (1 2 3))")
    except jsonschema.exceptions.ValidationError:
        pass

#tests()
if __name__ == '__main__':
    validate(sys.argv[1])
