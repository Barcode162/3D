# Reports, per model in assets/models/: file size, gzipped size (what the
# browser downloads), triangles, and GPU texture memory (with mipmaps), so
# the room stays inside the phone's budget (docs/archive/BAR_REDESIGN.md).
#   python3 tools/assets/glb_report.py [glb files...]
#   python3 tools/assets/glb_report.py --budget   every model with a budget in
#       assets/models/budget.json (the remake's kit, J35): fails when one is
#       over its triangles or texture memory
import glob
import gzip
import json
import struct
import sys


def image_size(data):
    if data[:8] == b"\x89PNG\r\n\x1a\n":
        return struct.unpack(">II", data[16:24])
    i = 2
    while i < len(data) - 9:
        if data[i] != 0xFF:
            i += 1
            continue
        marker = data[i + 1]
        if marker in (0xC0, 0xC1, 0xC2):
            h, w = struct.unpack(">HH", data[i + 5:i + 9])
            return w, h
        i += 2 + struct.unpack(">H", data[i + 2:i + 4])[0]
    return 0, 0


def report(path):
    b = open(path, "rb").read()
    n = struct.unpack("<I", b[12:16])[0]
    j = json.loads(b[20:20 + n])
    bin0 = 20 + n + 8
    tex = 0
    for im in j.get("images", []):
        bv = j["bufferViews"][im["bufferView"]]
        o = bin0 + bv.get("byteOffset", 0)
        w, h = image_size(b[o:o + bv["byteLength"]])
        tex += w * h * 4 * 4 / 3
    tris = sum(j["accessors"][p["indices"]]["count"] // 3 for m in j["meshes"] for p in m["primitives"])
    return len(b), len(gzip.compress(b, 6)), tris, tex


if "--budget" in sys.argv:
    budget = json.load(open("assets/models/budget.json"))["models"]
    over = []
    for name, b in sorted(budget.items()):
        r = report("assets/models/" + name)
        if r[2] > b["tris"] or r[3] / 1e6 > b["texMB"]:
            over.append(f"{name}: {r[2]} triangles (budget {b['tris']}), {r[3] / 1e6:.1f} MB of textures (budget {b['texMB']})")
    for line in over:
        print("OVER  " + line)
    print(f"{'FAIL' if over else 'ok  '}  budget: {len(budget) - len(over)} of {len(budget)} models within their budget")
    sys.exit(1 if over else 0)

files = sys.argv[1:] or sorted(glob.glob("assets/models/*.glb"))
tot = [0, 0, 0, 0]
print(f"{'model':28s} {'file MB':>8s} {'gz MB':>7s} {'tris':>8s} {'tex MB':>7s}")
for f in files:
    r = report(f)
    tot = [a + c for a, c in zip(tot, r)]
    print(f"{f.split('/')[-1]:28s} {r[0] / 1e6:8.2f} {r[1] / 1e6:7.2f} {r[2]:8d} {r[3] / 1e6:7.1f}")
print(f"{'TOTAL':28s} {tot[0] / 1e6:8.2f} {tot[1] / 1e6:7.2f} {tot[2]:8d} {tot[3] / 1e6:7.1f}")
