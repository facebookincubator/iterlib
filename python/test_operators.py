import ast
import os
import random
import unittest

from mock_user import UserQuery
from async_test import async_test


TEST_DATA = os.path.join(os.path.dirname(__file__), "test_data")


class QueryTests(unittest.TestCase):
    def setUp(self):
        random.seed(100)
        self.maxDiff = None

    @async_test
    async def test_project(self):
        resp = await UserQuery(range(1, 10)).project(["name", ":id"]).take(3).to_json()
        expected = [
            {
                "None": [
                    {"name": "id1", ":id": 1},
                    {"name": "id2", ":id": 2},
                    {"name": "id3", ":id": 3},
                ]
            }
        ]
        self.assertEqual(expected, resp)

    def test_sync_project(self):
        resp = UserQuery(range(1, 10)).project(["name", ":id"]).take(3).to_json().send()
        expected = [
            {
                "None": [
                    {"name": "id1", ":id": 1},
                    {"name": "id2", ":id": 2},
                    {"name": "id3", ":id": 3},
                ]
            }
        ]
        self.assertEqual(expected, resp)

    @async_test
    async def test_edge_project(self):
        resp = (
            await UserQuery(range(1, 5))
            .edge("friends")
            .project(["name", ":id"])
            .take(3)
            .to_json()
        )
        with open(os.path.join(TEST_DATA, "test_data_edge_project.txt")) as f:
            expected = ast.literal_eval(f.read())
        self.assertEqual(expected, resp)

    def test_sync_edge_project(self):
        resp = (
            UserQuery(range(1, 5))
            .edge("friends")
            .project(["name", ":id"])
            .take(3)
            .to_json()
            .send()
        )
        with open(os.path.join(TEST_DATA, "test_data_edge_project.txt")) as f:
            expected = ast.literal_eval(f.read())
        self.assertEqual(expected, resp)

    def test_sync_two_hop_project(self):
        resp = (
            UserQuery([1])
            .edge("friends")
            .edge("friends")
            .project(["name", ":id"])
            .take(3)
            .to_json()
            .send()
        )
        with open(os.path.join(TEST_DATA, "test_data_two_hop_project.txt")) as f:
            expected = ast.literal_eval(f.read())
        self.assertEqual(expected, resp)

    def test_sync_two_hop_multiple_project(self):
        resp = (
            UserQuery([1])
            .edge("friends")
            .project(["age", ":id"])
            .edge("friends")
            .project(["name", ":id"])
            .take(3)
            .to_json()
            .send()
        )
        with open(
            os.path.join(TEST_DATA, "test_data_two_hop_multiple_project.txt")
        ) as f:
            expected = ast.literal_eval(f.read())
        self.assertEqual(expected, resp)

    def test_sync_edge_union(self):
        resp = (
            UserQuery([1])
            .edge("friends")
            .project(["name"])
            .union(UserQuery([2]).edge("friends").project(["name"]))
            .to_json()
            .send()
        )
        with open(os.path.join(TEST_DATA, "test_data_edge_union.txt")) as f:
            expected = ast.literal_eval(f.read())
        self.assertEqual(expected, resp)

    @async_test
    async def test_nest(self):
        resp = (
            await UserQuery(range(1, 4))
            .project(["name", ":id"])
            .nest("items")
            .to_json()
        )
        expected = [
            {
                "None": [
                    {
                        "items": [
                            {"name": "id1", ":id": 1},
                            {"name": "id2", ":id": 2},
                            {"name": "id3", ":id": 3},
                        ]
                    }
                ]
            }
        ]
        self.assertEqual(expected, resp)

    @async_test
    async def test_let(self):
        resp = (
            await UserQuery(range(1, 5))
            .edge("friends")
            .count()
            .let("count", "friend_count")
            .to_json()
        )
        with open(os.path.join(TEST_DATA, "test_data_edge_let.txt")) as f:
            expected = ast.literal_eval(f.read())
        self.assertEqual(expected, resp)

    @async_test
    async def test_edge_count(self):
        resp = await UserQuery(range(1, 5)).edge("friends").count().to_json()
        with open(os.path.join(TEST_DATA, "test_data_edge_count.txt")) as f:
            expected = ast.literal_eval(f.read())
        self.assertEqual(expected, resp)

    @async_test
    async def test_where(self):
        resp = await UserQuery(range(1, 4)).where(lambda x: x["age"] == 16).to_json()
        expected = [{"None": [{":id": 2, "name": "id2", ":type": 2, "age": 16}]}]
        self.assertEqual(expected, resp)

    @async_test
    async def test_order_by(self):
        resp = await UserQuery(range(1, 4)).order_by(lambda x: x["age"]).to_json()
        expected = [
            {
                "None": [
                    {":id": 2, "name": "id2", ":type": 2, "age": 16},
                    {":id": 1, "name": "id1", ":type": 1, "age": 17},
                    {":id": 3, "name": "id3", ":type": 2, "age": 18},
                ]
            }
        ]
        self.assertEqual(expected, resp)

    @async_test
    async def test_as_dict(self):
        resp = (
            await UserQuery(range(1, 10))
            .project(["name", ":id"])
            .take(3)
            .to_json()
            .as_dict()
        )
        expected = {
            "1": {"name": "id1", ":id": 1},
            "2": {"name": "id2", ":id": 2},
            "3": {"name": "id3", ":id": 3},
        }
        self.assertEqual(expected, resp)

    @async_test
    async def test_as_list(self):
        resp = (
            await UserQuery(range(1, 10))
            .project(["name", ":id"])
            .take(3)
            .to_json()
            .as_list()
        )
        expected = [
            {"name": "id1", ":id": 1},
            {"name": "id2", ":id": 2},
            {"name": "id3", ":id": 3},
        ]
        self.assertEqual(expected, resp)


if __name__ == "__main__":
    unittest.main()
