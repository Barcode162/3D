#!/usr/bin/env python3
"""The lounge bookshelf's books (J99): public-domain works from Project
Gutenberg, cut into chapters and paragraphs for the reader (bar/web/reader.js).
Gutenberg's header, footer and licence are left out (the texts are public
domain; the Gutenberg name is not used with them).

  python3 tools/books.py <download dir>     → bar/web/books/<id>.json and index.json

Each book: its Gutenberg number, how its chapter (and part) lines look, and
the facts shown in the reader.
"""
import json
import os
import re
import sys
import urllib.request

BOOKS = [
    {"id": "crime-and-punishment", "pg": 2554, "title": "Crime and Punishment", "author": "Fyodor Dostoyevsky",
     "translator": "Constance Garnett", "year": 1866, "part": r"^PART [IVX]+$", "chapter": r"^CHAPTER [IVXL]+$",
     "extra": r"^EPILOGUE$"},
    {"id": "brothers-karamazov", "pg": 28054, "title": "The Brothers Karamazov", "author": "Fyodor Dostoyevsky",
     "translator": "Constance Garnett", "year": 1880, "part": r"^(PART [IVX]+|Book [IVXL]+\..*)$",
     "chapter": r"^Chapter [IVXL]+\..*$", "extra": r"^EPILOGUE$"},
    {"id": "notes-from-underground", "pg": 600, "title": "Notes from Underground", "author": "Fyodor Dostoyevsky",
     "translator": "Constance Garnett", "year": 1864, "part": r"^ ?PART [IVX]+.*$", "chapter": r"^ ?[IVXL]+$",
     "begin": r"^NOTES FROM THE UNDERGROUND\[\*\]"},
    {"id": "relativity", "pg": 30155, "title": "Relativity: The Special and General Theory", "author": "Albert Einstein",
     "translator": "Robert W. Lawson", "year": 1916, "part": r"^PART [IVX]+.*$",
     "chapter": r"^(PREFACE|[IVXL]+\.( .+)?|APPENDIX.*)$", "name_below": True},
    {"id": "moby-dick", "pg": 2701, "title": "Moby-Dick; or, The Whale", "author": "Herman Melville", "year": 1851,
     "chapter": r"^(CHAPTER \d+\..*|Epilogue)$"},
    {"id": "great-gatsby", "pg": 64317, "title": "The Great Gatsby", "author": "F. Scott Fitzgerald", "year": 1925,
     "chapter": r"^ *[IVX]+ *$"},
]


def body(text):
    start = re.search(r"\*\*\* ?START OF (THE|THIS) PROJECT GUTENBERG.*?\*\*\*", text)
    end = re.search(r"\*\*\* ?END OF (THE|THIS) PROJECT GUTENBERG", text)
    return text[start.end() if start else 0:end.start() if end else len(text)]


def paragraphs(lines):
    out, cur = [], []
    for ln in lines:
        if ln.strip():
            cur.append(ln.strip())
        elif cur:
            out.append(" ".join(cur))
            cur = []
    if cur:
        out.append(" ".join(cur))
    # Typewriter dashes become real ones; _italics_ stay marked for the reader.
    return [p.replace("--", "\u2014") for p in out if "gutenberg" not in p.lower()]


ROMAN = re.compile(r"^[IVXLC]+[.:]?$")


def nice(s):
    """Title case, numerals kept as numerals (Part II, not Part Ii)."""
    s = s.strip().rstrip(".")
    if not s.isupper():
        return s
    return " ".join(w if ROMAN.match(w) else w.capitalize() for w in s.split())


def chapters(book, text):
    part_re = re.compile(book["part"]) if book.get("part") else None
    chap_re = re.compile(book["chapter"])
    extra_re = re.compile(book["extra"]) if book.get("extra") else None
    lines = body(text).replace("\r\n", "\n").split("\n")
    if book.get("begin"):
        first = next(i for i, ln in enumerate(lines) if re.match(book["begin"], ln))
        lines = lines[first:]
    out, part, title, buf = [], "", None, []
    name_next = False

    def flush():
        if title is not None:
            paras = paragraphs(buf)
            if sum(len(p) for p in paras) > 600:  # a real chapter (not a line of the contents)
                out.append({"title": title, "paras": paras})

    for ln in lines:
        s = ln.rstrip()
        if name_next:  # the section's name, on the lines below its number (capitals, until a blank line)
            if s.strip() and s.strip().isupper():
                title += " " + nice(s)
                continue
            if s.strip() or title.split(" · ")[-1].count(" "):
                name_next = False
        if part_re and part_re.match(s):
            flush()
            part, title, buf = nice(s), None, []
            continue
        if extra_re and extra_re.match(s):  # an epilogue: a part of its own (or one chapter)
            flush()
            part = nice(s)
            title, buf = part, []
            continue
        if chap_re.match(s):
            flush()
            name = nice(s)
            title, buf = (part + " · " + name) if part and part != name else name, []
            name_next = bool(book.get("name_below")) and bool(ROMAN.match(s.strip()))
            continue
        if title is not None:
            buf.append(ln)
    flush()
    return out


def main():
    dl = sys.argv[1]
    os.makedirs(dl, exist_ok=True)
    out_dir = os.path.join(os.path.dirname(__file__), "..", "bar", "web", "books")
    os.makedirs(out_dir, exist_ok=True)
    index = []
    for b in BOOKS:
        path = os.path.join(dl, f"pg{b['pg']}.txt")
        if not os.path.exists(path):
            urllib.request.urlretrieve(f"https://www.gutenberg.org/cache/epub/{b['pg']}/pg{b['pg']}.txt", path)
        chs = chapters(b, open(path, encoding="utf-8").read())
        facts = {k: b[k] for k in ("id", "title", "author", "year") if k in b}
        if "translator" in b:
            facts["translator"] = b["translator"]
        with open(os.path.join(out_dir, b["id"] + ".json"), "w", encoding="utf-8") as f:
            json.dump({**facts, "chapters": chs}, f, ensure_ascii=False, separators=(",", ":"))
        words = sum(len(p.split()) for c in chs for p in c["paras"])
        index.append({**facts, "chapters": len(chs), "words": words})
        print(f"BOOK {b['id']}: {len(chs)} chapters, {words} words")
    with open(os.path.join(out_dir, "index.json"), "w", encoding="utf-8") as f:
        json.dump({"about": "The lounge bookshelf's books (tools/books.py): public-domain works from Project Gutenberg.",
                   "books": index}, f, ensure_ascii=False, indent=1)


main()
