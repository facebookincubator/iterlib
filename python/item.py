# Copyright (c) 2016-present, Facebook, Inc. All rights reserved.

from collections import OrderedDict

import json


def pprint_json(d):
    """Output valid json and pretty print"""
    s = json.dumps(d, indent=4, sort_keys=True)
    print(s)


class Item(OrderedDict):
    """Like an OrderedDict, but treats :id as special for
       equality purposes and hashable (so you can create sets).
    """

    def __lt__(self, other):
        return self[":id"] < other[":id"]

    def __eq__(self, other):
        if (":id" in self):
            return self[":id"] == other[":id"]
        else:
            return dict.__eq__(self, other)

    def __repr__(self):
        return dict.__repr__(self)

    def __hash__(self):
        if (":id" in self):
            return hash(self[":id"])
        else:
            return 0
