import unittest
import asyncio
import inspect

from async_test import async_test
from walk import _materialize_walk_sync, materialize_walk


class MaterializeWalkTests(unittest.TestCase):
    def setUp(self):
        self.maxDiff = None

    @async_test
    async def test_generator(self):
        self.assertEqual(
            list(range(1, 5)), await materialize_walk((i for i in range(1, 5)))
        )

    @staticmethod
    async def _gen_range(start, end):
        for i in range(start, end):
            yield i

    @async_test
    async def test_async_generator(self):
        self.assertEqual(
            list(range(1, 5)),
            await materialize_walk((i async for i in self._gen_range(1, 5))),
        )

    @async_test
    async def test_iterable(self):
        self.assertEqual(
            list(range(1, 5)), await materialize_walk(iter(list(range(1, 5))))
        )

    @async_test
    async def test_dict(self):
        self.assertEqual(
            {"nums": list(range(1, 5))},
            await materialize_walk({"nums": iter(list(range(1, 5)))}),
        )

    @staticmethod
    def get_numbers(start, end):
        out = {}
        for i in range(start, end):
            ones, tens = i % 10, int(i / 10)
            try:
                out[str(tens)].append({str(ones): ones})
            except KeyError:
                out[str(tens)] = [{str(ones): ones}]
        return out

    @staticmethod
    def gen_numbers(start, end):
        for i in range(start, end, 10):
            yield str(int(i / 10)), [{str(i): i} for i in range(0, 10)]

    @async_test
    async def test_nested_dict(self):
        expected = self.get_numbers(0, 20)
        lazy = dict(self.gen_numbers(0, 20))
        self.assertEqual(expected, await materialize_walk(lazy))

    def test_nested_dict_sync(self):
        expected = self.get_numbers(0, 20)
        lazy = dict(self.gen_numbers(0, 20))
        self.assertEqual(expected, _materialize_walk_sync(lazy))


if __name__ == "__main__":
    unittest.main()
