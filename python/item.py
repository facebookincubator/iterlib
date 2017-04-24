# Copyright (c) 2016-present, Facebook, Inc. All rights reserved.

from collections import OrderedDict

import json


def pprint_json(d):
    """Output valid json and pretty print"""
    s = json.dumps(d, indent=4, sort_keys=True)
    print(s)

IDKEY = ":id"

class Item(OrderedDict):
    """Like an OrderedDict, but treats :id as special for
       equality purposes and hashable (so you can create sets).
    """

    def __lt__(self, other):
        return self[IDKEY] < other[IDKEY]

    def __eq__(self, other):
        if (IDKEY in self):
            return self[IDKEY] == other[IDKEY]
        else:
            return dict.__eq__(self, other)

    def __repr__(self):
        return dict.__repr__(self)

    def __hash__(self):
        if (IDKEY in self):
            return hash(self[IDKEY])
        else:
            return 0
