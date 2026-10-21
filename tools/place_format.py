#!/usr/bin/env python3
"""Pretty JSON for the world's files (assets/world/): short lists and small
objects stay on one line, the rest one entry per line, so a place or kind
file reads like a list of things.

  python3 tools/place_format.py <file.json>          # reformat in place
  python3 tools/place_format.py --dump <dump.json>   # the converter's output
      (tools/native/place_dump.cpp) → places/bar.json + parts into kinds/bar.json
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
    if sys.argv[1] == '--dump':
        dump = json.load(open(sys.argv[2]))
        write('assets/world/places/bar.json', dump['place'])
        kinds = json.load(open('assets/world/kinds/bar.json'))
        have = {k['id']: k for k in kinds['kinds']}
        for kid, parts in dump['kindParts'].items():
            k = have.get(kid)
            if k is None:
                print('no kind', kid)
                continue
            k.setdefault('parts', {}).update(parts)
        for k in kinds['kinds']:
            ordered = {x: k[x] for x in ['id', 'is', 'about', 'still', 'pivot', 'parts'] if x in k}
            ordered.update({x: v for x, v in k.items() if x not in ordered})
            k.clear()
            k.update(ordered)
        write('assets/world/kinds/bar.json', kinds)
    else:
        write(sys.argv[1], json.load(open(sys.argv[1])))


main()
