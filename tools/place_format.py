#!/usr/bin/env python3
"""Pretty JSON for the world's files (assets/world/): short lists and small
objects stay on one line, the rest one entry per line, so a place or kind
file reads like a list of things.

  python3 tools/place_format.py assets/world/places/bar.json ...   # reformat in place
"""
import json
import sys

WIDTH = 120


def fmt(v, indent=0):
    one = json.dumps(v, ensure_ascii=False)
    if len(one) + indent <= WIDTH or not isinstance(v, (list, dict)) or not v:
        return one
    pad = ' ' * (indent + 2)
    if isinstance(v, list):
        items = [pad + fmt(x, indent + 2) for x in v]
        return '[\n' + ',\n'.join(items) + '\n' + ' ' * indent + ']'
    items = [pad + json.dumps(k) + ': ' + fmt(x, indent + 2) for k, x in v.items()]
    return '{\n' + ',\n'.join(items) + '\n' + ' ' * indent + '}'


def write(path, data):
    with open(path, 'w') as f:
        f.write(fmt(data) + '\n')


def main():
    for path in sys.argv[1:]:
        write(path, json.load(open(path)))


main()
