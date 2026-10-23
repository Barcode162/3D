#!/usr/bin/env python3
"""Stages only the changes (hunks) of a piece, when two pieces touched the
same files (docs/AUTOMATION.md: one piece = one commit):

  python3 tools/stage_hunks.py 'regex' [file ...]
      stages every changed hunk whose text matches the regex (in any file),
      plus the given files whole
  python3 tools/stage_hunks.py --except 'regex' file ...
      stages the given files' changes except the hunks matching the regex
      (safer when one piece's edits sit between another's)
  python3 tools/stage_hunks.py --list ...        only shows what it would stage

Then commit as usual (git commit, or tools/piece.mjs with the rest stashed).
"""
import re
import subprocess
import sys

args = sys.argv[1:]
dry = '--list' in args
exclude = '--except' in args
args = [a for a in args if a not in ('--list', '--except')]
pattern = re.compile(args[0]) if args else None
whole = args[1:]

diff = subprocess.run(['git', 'diff', '-U0', '--no-color'], capture_output=True, text=True, check=True).stdout
files = re.split(r'(?m)^(?=diff --git )', diff)
out = []
for f in files:
    if not f.strip():
        continue
    head, *hunks = re.split(r'(?m)^(?=@@ )', f)
    name = re.search(r'^diff --git a/(\S+)', head, re.M).group(1)
    if exclude:
        if name not in whole:
            continue
        keep = [h for h in hunks if not (pattern and pattern.search(h))]
    elif name in whole:
        continue
    else:
        keep = [h for h in hunks if pattern and pattern.search(h)]
    if keep:
        out.append(head + ''.join(keep))
        for h in keep:
            print(f'{name}: {h.splitlines()[0]}')
patch = ''.join(out)
if dry:
    print('(whole: ' + ', '.join(whole) + ')' if whole else '')
    sys.exit(0)
if patch:
    subprocess.run(['git', 'apply', '--cached', '--recount', '--unidiff-zero', '-'], input=patch, text=True, check=True)
if whole and not exclude:
    subprocess.run(['git', 'add', '--'] + whole, check=True)
print(subprocess.run(['git', 'diff', '--cached', '--stat'], capture_output=True, text=True).stdout)
