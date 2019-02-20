#!/usr/bin/env python3.5
# Copyright (c) 2016-present, Facebook, Inc. All rights reserved.

import ast
import execute
import sexp
import unittest

from contextlib import ExitStack
from execute import dictify
from execute import ldictify
from mock import test_execute
from walk import path_it, leaf_it


class Queries(unittest.TestCase):

    @staticmethod
    def stringify_json(data):
        data = str(data)
        data = data.replace("'", '\\"')
        data = "\"%s\"" % data
        return data

    def assertMapEqual(self, m1, m2):
        self.assertEqual(list(path_it(m1)), list(path_it(m2)))

    def test_limit(self):
        self.assertMapEqual(test_execute("(limit 3 (6 5 4 3 2 1))"),
                            [{'None': dictify([6, 5, 4])}])
        self.assertMapEqual(test_execute("(limit 3 2 (6 5 4 3 2 1))"),
                            [{'None': dictify([4, 3, 2])}])

    def test_reverse(self):
        self.assertMapEqual(test_execute("(reverse (6 5 4 3 2 1))"),
                            [{'None': dictify([1, 2, 3, 4, 5, 6])}])

    def test_count(self):
        self.assertEqual(list(test_execute("(count (6 5 4 3 2 1))")),
                         [{'None': {"count": 6}}])

    def test_and(self):
        self.assertEqual(list(test_execute("(and (6 5 4 3 2 1) (6 3 1))")),
                         ldictify([6, 3, 1]))

    def test_or(self):
        self.assertEqual(list(test_execute("(or (6 5 4 3 2 1) (10 6 3 1))")),
                         ldictify([10, 6, 5, 4, 3, 2, 1]))

    def test_difference(self):
        self.assertEqual(list(test_execute("(difference (6 5 4 3 2 1) (6 3 1))")),
                         ldictify([2, 4, 5]))

    def test_filter(self):
        self.assertMapEqual(test_execute("(filter (< :id 3) (6 5 4 3 2 1))"),
                            [{'None': ldictify([2, 1])}])
        self.assertMapEqual(test_execute("(filter (inset :id (3, 1)) (6 5 4 3 2 1))"),
                            [{'None': ldictify([3, 1])}])
        data = self.stringify_json(
            [{'None': [{"a": "foo1"}, {"a": "2foo"}, {"a": "bar"}]}])
        self.assertMapEqual(test_execute("(filter (contains a foo) (json_literal %s))" % data),
                            [{'None': [{"a": "foo1"}, {"a": "2foo"}]}])
        self.assertMapEqual(test_execute("(filter (prefix a foo) (json_literal %s))" % data),
                            [{'None': [{"a": "foo1"}]}])
        self.assertMapEqual(test_execute("(filter (range :id (5 2)) (6 5 4 3 2 1))"),
                            [{'None': ldictify([4, 3, 2])}])

    def test_project(self):
        data = self.stringify_json([{'None': [{"a": 1, "b": 2, "c": 3}]}])
        self.assertEqual(list(test_execute("(project (a b) (json_literal %s))" % data)),
                         [{'None': [{"a": 1, "b": 2}]}])

    def test_aggregate(self):
        data = self.stringify_json([{"a": ldictify([1, 2, 3]),
                                     "b": ldictify([4, 5, 6])}])
        m = test_execute("(aggregate (json_literal %s))" % data)
        m = execute.materialize_walk(m)
        m = tuple(m[0].values())[0]
        self.assertEqual(set(m), set(ldictify([1, 2, 3, 4, 5, 6])))

    def test_orderby(self):
        data = [{'None': [{"a": 1, "b": 2, "c": 3},
                          {"a": 4, "b": 2, "c": 2},
                          {"a": 4, "b": 1, "c": 6},
                          {"a": 7, "b": 2, "c": 5}]}]
        expected = [{'None': [{"a": 7, "b": 2, "c": 5},
                              {"a": 4, "b": 1, "c": 6},
                              {"a": 4, "b": 2, "c": 2},
                              {"a": 1, "b": 2, "c": 3}]}]
        data = self.stringify_json(data)
        self.assertEqual(list(test_execute("(orderby (a c) (json_literal %s))" % data)),
                         expected)

    def test_groupby(self):
        data = [{'None': [{"a": 1, "b": 2, "c": 3},
                          {"a": 4, "b": 2, "c": 2},
                          {"a": 4, "b": 1, "c": 6},
                          {"a": 7, "b": 2, "c": 5}]}]
        expected = [{'None': [[{"a": 1}, [{"a": 1, "b": 2, "c": 3}]],
                              [{"a": 4}, [{"a": 4, "b": 2, "c": 2},
                                          {"a": 4, "b": 1, "c": 6}]],
                              [{"a": 7}, [{"a": 7, "b": 2, "c": 5}]]]}]
        data = self.stringify_json(data)
        it = test_execute("(groupby (a) (json_literal %s))" % data)
        actual = execute.materialize_walk(it)
        self.assertEqual(actual, expected)

    def test_nest(self):
        data = self.stringify_json([{"a": 1, "b": 2, "c": 3}])
        self.assertEqual(list(test_execute("(nest foo (json_literal %s))" % data)),
                         [{"foo": {"a": 1, "b": 2, "c": 3}}])

    def test_merge(self):
        data1 = self.stringify_json([{"a": [1], "b": [2]}])
        data2 = self.stringify_json([{"a": [3, 4], "b": [5, 6]}])
        self.assertEqual(list(test_execute("(merge (json_literal %s) (json_literal %s))" % (data1, data2))),
                         [{"a": [1, 3, 4], "b": [2, 5, 6]}])

    def test_exists(self):
        self.assertEqual(list(test_execute("(exists (and (3 2 1) (6 5 4)))")),
                         [])
        self.assertEqual(list(test_execute("(exists (and (3 2 1) (5 4 3)))")),
                         [None])

    def test_let(self):
        data = self.stringify_json([{"a": 1, "b": 2, "c": 3}])
        self.assertEqual(list(test_execute("(let ((d a) (e c)) (json_literal %s))" % data)),
                         [{"a": 1, "b": 2, "c": 3, "d": 1, "e": 3}])

    def test_apply(self):
        self.maxDiff = None
        with open("test_data_1.txt") as f:
            expected = ast.literal_eval(f.read())
        self.assertEqual(list(test_execute("(apply (assoc 100) (3 2 1))")),
                         expected)
        with open("test_data_2.txt") as f:
            expected_2hop = ast.literal_eval(f.read())
        self.assertEqual(list(test_execute("(apply (assoc 200) (apply (assoc 100) (3 2 1)))")),
                         expected_2hop)


class ThreadLast(unittest.TestCase):

    def test_threadLast(self):
        self.assertEqual(sexp.parse("(->> a b )"), ['b', 'a'])
        self.assertEqual(sexp.parse("(->> a b c )"), ['c', ['b', 'a']])
        self.assertEqual(sexp.parse("(->> (a) (b) (c ))"), ['c', ['b', ['a']]])
        self.assertEqual(sexp.parse("(->> (a) (b c) (d ))"),
                         ['d', ['b', 'c', ['a']]])
        self.assertEqual(sexp.parse("(->> a (b) (c) (->> d e f ))"),
                         ['f', ['e', 'd'], ['c', ['b', 'a']]])

if __name__ == '__main__':
    unittest.main()
