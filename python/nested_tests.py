#!/usr/bin/env python3.6
# Copyright (c) 2016-present, Facebook, Inc. All rights reserved.

import ast
import unittest
import random

from mock import test_execute
from walk import materialize_walk

class Queries(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        self.maxDiff = None
        random.seed(100)  # so tests are deterministic
        super(Queries, self).__init__(*args, **kwargs)

    def assertMapEqual(self, m1, m2):
        m1 = materialize_walk(m1)
        m2 = ''.join([line.lstrip() for line in m2.splitlines()])
        m2 = ast.literal_eval(m2)
        self.assertEqual(str(m1), str(m2))

    def test_obj_query(self):
        expected = """
        [
            {
                "None": [
                    {
                        ":id": 3,
                        "age": 16,
                        "name": "id3"
                    },
                    {
                        ":id": 2,
                        "age": 17,
                        "name": "id2"
                    },
                    {
                        ":id": 1,
                        "age": 17,
                        "name": "id1"
                    }
                ]
            }
        ]
        """
        self.assertMapEqual(test_execute("(->> (3 2 1) (obj))"), expected)

    def test_obj_query_with_ops(self):
        expected = """
        [
            {
                "None": [
                    {
                        ":id": 3,
                        "age": 16,
                        "name": "id3"
                    },
                    {
                        ":id": 2,
                        "age": 18,
                        "name": "id2"
                    }
                ]
            }
        ]
        """
        self.assertMapEqual(test_execute("(->> (3 2 1) (obj) (limit 2))"), expected)


if __name__ == '__main__':
    unittest.main()
