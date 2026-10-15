# Builds the back bar: a paneled lower cabinet, a smoked-mirror back with
# three shelves on posts, warm LED strips under each shelf, a crown on top,
# and the bottles on the shelves (six shapes, many glass colors, labels).
#
#   blender -b --factory-startup --python tools/assets/back_bar.py -- <texture dir> <out.glb>
#
# Model space: x along the wall (centered), y up from the floor, +z faces
# the room. The back is at z = 0 (the scene puts it against the back wall).
import math
import os
import random
import sys

import bmesh
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402

tex_dir, out_path = sys.argv[sys.argv.index("--") + 1:][:2]
common.reset_scene()
rng = random.Random(7)

W = 6.4
HW = W / 2
DEPTH = 0.55       # lower cabinet
SHELF_D = 0.32     # upper shelves
SHELVES = (1.30, 1.75, 2.20)
POSTS = (-HW, -1.1, 1.1, HW - 0.08)
POST_W = 0.08
TOP_Y = 2.48

wood = common.pbr_material("Back bar oak", tex_dir, "dark_wood", tint=(0.55, 0.45, 0.4), roughness_scale=0.55)
top_wood = common.pbr_material("Back bar top", tex_dir, "wood_table_001", tint=(1.0, 0.92, 0.85),
                               roughness_scale=0.45, roughness_bias=0.04, grain="v", size=512)
mirror = common.plain_material("Smoked mirror", (0.30, 0.24, 0.19), 1.0, 0.06)
led = common.plain_material("LED strip", (1.0, 0.75, 0.45), 0.0, 0.3, emission=(1.0, 0.55, 0.22), strength=2.5)
brass = common.plain_material("Brass", (0.91, 0.70, 0.38), 1.0, 0.3)
kick = common.plain_material("Black kick plate", (0.02, 0.02, 0.02), 0.0, 0.6)


def box(name, lo, hi, mat, bevel=0.005, segments=2, grain_axis="x"):
    obj = common.box(name, lo, hi, mat, bevel, segments)
    obj["grain_axis"] = grain_axis
    return obj


# ---- Lower cabinet: doors with raised panels, brass knobs, a thick top. ----
box("Cabinet body", (-HW, 0.1, 0.0), (HW, 0.95, DEPTH - 0.02), wood, 0.003)
box("Toe kick", (-HW + 0.02, 0.0, 0.0), (HW - 0.02, 0.1, DEPTH - 0.06), kick, 0.002)
doors = 8
door_w = W / doors
for i in range(doors):
    x0 = -HW + i * door_w + 0.008
    x1 = x0 + door_w - 0.016
    box("Door", (x0, 0.12, DEPTH - 0.02), (x1, 0.93, DEPTH), wood, 0.006, 2, "y")
    box("Door field", (x0 + 0.07, 0.2, DEPTH - 0.005), (x1 - 0.07, 0.85, DEPTH + 0.012), wood, 0.016, 3, "y")
    kx = x1 - 0.05 if i % 2 == 0 else x0 + 0.05
    common.sphere("Knob", Vector((kx, 0.78, DEPTH + 0.02)), 0.014, brass, 12, 8)
box("Cabinet top", (-HW - 0.03, 0.95, 0.0), (HW + 0.03, 0.995, DEPTH + 0.04), top_wood, 0.012, 4)

# ---- Upper unit: mirror back, posts, shelves, LED strips, crown. ----
box("Mirror", (-HW, 0.995, 0.0), (HW, TOP_Y, 0.012), mirror, 0.0)
for x in POSTS:
    box("Post", (x, 0.995, 0.0), (x + POST_W, TOP_Y, SHELF_D + 0.02), wood, 0.008, 2, "y")
for y in SHELVES:
    box("Shelf", (-HW, y - 0.0175, 0.012), (HW, y + 0.0175, SHELF_D), wood, 0.006)
    box("Shelf lip", (-HW, y - 0.03, SHELF_D - 0.012), (HW, y + 0.025, SHELF_D + 0.005), wood, 0.006, 2)
    box("LED", (-HW + 0.05, y - 0.04, SHELF_D - 0.03), (HW - 0.05, y - 0.03, SHELF_D - 0.016), led, 0.0)
box("Crown", (-HW - 0.04, TOP_Y, 0.0), (HW + 0.04, TOP_Y + 0.06, SHELF_D + 0.05), wood, 0.015, 3)
box("Crown cap", (-HW - 0.07, TOP_Y + 0.06, 0.0), (HW + 0.07, TOP_Y + 0.1, SHELF_D + 0.08), wood, 0.012, 3)
box("Crown frieze", (-HW, TOP_Y - 0.12, SHELF_D - 0.01), (HW, TOP_Y, SHELF_D + 0.02), wood, 0.006)

# ---- Bottles ----
# Glass colors live in a small atlas: each bottle's glass points its UVs at
# one swatch, so all the glass is a single material (one draw call).
GLASS = [(0.42, 0.17, 0.03), (0.05, 0.20, 0.06), (0.55, 0.60, 0.58), (0.30, 0.03, 0.05),
         (0.62, 0.36, 0.06), (0.04, 0.09, 0.30), (0.07, 0.05, 0.04), (0.02, 0.28, 0.18)]
sw = np.zeros((8, 8, 3), dtype=np.float32)
for i, c in enumerate(GLASS):
    sw[:, i] = c
glass_mat = common.image_material("Bottle glass", np.repeat(np.repeat(sw, 8, 0), 8, 1), roughness=0.05)


def glass_uv(obj, color):
    common.swatch_uvs(obj, (color + 0.5) / 8, 0.5)


# Labels: a 4x4 atlas of designs drawn here (paper colors, borders, bands,
# "text" lines, a crest). Row 0 of the image is the bottom.
LS = 128
labels = np.zeros((LS * 4, LS * 4, 3), dtype=np.float32)
PAPER = [(0.85, 0.80, 0.65), (0.08, 0.08, 0.08), (0.75, 0.10, 0.08), (0.92, 0.92, 0.88),
         (0.12, 0.20, 0.35), (0.70, 0.55, 0.20), (0.15, 0.30, 0.15), (0.55, 0.15, 0.25)]
INK = [(0.10, 0.08, 0.05), (0.85, 0.70, 0.35), (0.95, 0.90, 0.80), (0.60, 0.10, 0.08),
       (0.85, 0.80, 0.60), (0.10, 0.08, 0.05), (0.90, 0.85, 0.60), (0.95, 0.85, 0.75)]
lr = np.random.default_rng(3)
yy, xx = np.mgrid[0:LS, 0:LS]
for k in range(16):
    paper, ink = PAPER[k % 8], INK[(k * 3) % 8]
    cell = np.empty((LS, LS, 3), dtype=np.float32)
    cell[:] = paper
    # Paper grain.
    cell *= (0.94 + 0.06 * lr.random((LS, LS, 1))).astype(np.float32)
    # Border.
    bw = 4 + k % 3 * 2
    edge = (xx < bw) | (xx >= LS - bw) | (yy < bw) | (yy >= LS - bw)
    inner = (xx >= bw + 3) & (xx < LS - bw - 3) & (yy >= bw + 3) & (yy < LS - bw - 3)
    cell[edge] = ink
    cell[~edge & ~inner] = paper
    # A band across the middle with a big "brand name" bar.
    band = (yy > LS * 0.42) & (yy < LS * 0.62) & inner
    if k % 2:
        cell[band] = ink
        name_c = paper
    else:
        name_c = ink
    words = lr.integers(2, 4)
    x = LS * 0.18
    for wdx in range(words):
        wl = lr.integers(12, 28)
        m = (xx > x) & (xx < x + wl) & (yy > LS * 0.47) & (yy < LS * 0.57)
        cell[m] = name_c
        x += wl + 6
    # Small text lines below, and a crest (circle) above.
    for line in range(3):
        y0 = LS * (0.18 + 0.06 * line)
        lw = lr.integers(40, 80)
        m = (xx > (LS - lw) / 2) & (xx < (LS + lw) / 2) & (yy > y0) & (yy < y0 + 3)
        cell[m] = ink
    cy, cx, cr = LS * 0.78, LS * 0.5, LS * (0.08 + 0.02 * (k % 3))
    d = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2)
    cell[(d < cr) & (d > cr - 3)] = ink
    if k % 3 == 0:
        cell[d < cr * 0.5] = ink
    r, c = divmod(k, 4)
    labels[r * LS:(r + 1) * LS, c * LS:(c + 1) * LS] = cell
label_mat = common.image_material("Bottle labels", labels, roughness=0.55)
cap_gold = common.plain_material("Cap foil", (0.85, 0.65, 0.3), 1.0, 0.35)
cap_dark = common.plain_material("Cap", (0.05, 0.04, 0.04), 0.3, 0.4)
cork = common.plain_material("Cork", (0.45, 0.30, 0.17), 0.0, 0.8)


def label_band(name, radius, y0, y1, k, x, z, wide=1.0, segments=10):
    """A label wrapped part way round a round bottle, facing +z."""
    half = math.radians(55) * wide
    bm = bmesh.new()
    cols = []
    for s in range(segments + 1):
        a = -half + 2 * half * s / segments
        p = (x + radius * math.sin(a), z + radius * math.cos(a))
        cols.append((bm.verts.new((p[0], y0, p[1])), bm.verts.new((p[0], y1, p[1]))))
    for s in range(segments):
        bm.faces.new([cols[s][0], cols[s + 1][0], cols[s + 1][1], cols[s][1]])
    obj = common.mesh_object(name, bm, label_mat, smooth=True)
    r, c = divmod(k, 4)
    u0, v0 = c / 4 + 0.01, r / 4 + 0.01
    common.set_uvs(obj, lambda co, n: (u0 + 0.23 * (0.5 + math.atan2(co.x - x, co.z - z) / (2 * half)),
                                       v0 + 0.23 * (co.y - y0) / (y1 - y0)))
    return obj


def flat_label(name, x0, x1, y0, y1, z, k):
    bm = bmesh.new()
    v = [bm.verts.new(p) for p in ((x0, y0, z), (x1, y0, z), (x1, y1, z), (x0, y1, z))]
    bm.faces.new(v)
    obj = common.mesh_object(name, bm, label_mat)
    r, c = divmod(k, 4)
    u0, v0 = c / 4 + 0.01, r / 4 + 0.01
    common.set_uvs(obj, lambda co, n: (u0 + 0.23 * (co.x - x0) / (x1 - x0), v0 + 0.23 * (co.y - y0) / (y1 - y0)))
    return obj


def round_bottle(x, y, z, kind, color, k):
    """Lathed bottles. Profiles are (radius, height) from the base up."""
    shapes = {
        # whiskey: tall shoulders, short neck
        "whiskey": [(0.0, 0), (0.036, 0), (0.038, 0.006), (0.038, 0.19), (0.032, 0.215), (0.016, 0.235),
                    (0.014, 0.27), (0.0, 0.27)],
        # gin/vodka: tall and slim with a long neck
        "slim": [(0.0, 0), (0.031, 0), (0.033, 0.006), (0.033, 0.21), (0.024, 0.24), (0.012, 0.26),
                 (0.011, 0.31), (0.0, 0.31)],
        # rum: wide and stubby
        "stubby": [(0.0, 0), (0.042, 0), (0.044, 0.006), (0.044, 0.15), (0.034, 0.18), (0.016, 0.195),
                   (0.015, 0.225), (0.0, 0.225)],
        # liqueur: round body, long thin neck
        "liqueur": [(0.0, 0), (0.03, 0), (0.042, 0.03), (0.044, 0.08), (0.036, 0.13), (0.014, 0.16),
                    (0.010, 0.25), (0.0, 0.25)],
        # wine: classic Bordeaux
        "wine": [(0.0, 0), (0.035, 0), (0.037, 0.006), (0.037, 0.2), (0.03, 0.225), (0.0135, 0.25),
                 (0.0125, 0.30), (0.0, 0.30)],
    }
    prof = shapes[kind]
    top = prof[-1][1]
    body = common.lathe("Bottle", [(r, h) for (r, h) in prof], glass_mat, 12)
    common.transform(body, Matrix.Translation((x, y, z)))
    glass_uv(body, color)
    r_body = prof[3][0]
    # Label on the body, and a neck label on some.
    y_lo, y_hi = {"whiskey": (0.05, 0.15), "slim": (0.06, 0.17), "stubby": (0.04, 0.12),
                  "liqueur": (0.045, 0.11), "wine": (0.05, 0.14)}[kind]
    label_band("Label", r_body + 0.0015, y + y_lo, y + y_hi, k, x, z, 1.1 if kind == "stubby" else 1.0)
    neck_r = prof[-2][0]
    cap_mat = [cap_gold, cap_dark, cork][k % 3]
    cap_h = 0.035 if cap_mat is not cork else 0.02
    common.cylinder("Cap", Vector((x, y + top - cap_h / 2 + 0.008, z)), neck_r + 0.002, cap_h, "y", cap_mat,
                    segments=10, bevel=0.001)


def square_bottle(x, y, z, color, k):
    """A square-shouldered bottle (Tennessee whiskey style)."""
    s = 0.034
    body = common.box("Square bottle", (x - s, y, z - s), (x + s, y + 0.17, z + s), glass_mat, 0.008, 2)
    glass_uv(body, color)
    shoulder = common.lathe("Shoulder", [(0.0, 0.17), (0.034, 0.17), (0.026, 0.19), (0.014, 0.205),
                                         (0.0125, 0.25), (0.0, 0.25)], glass_mat, 12)
    common.transform(shoulder, Matrix.Translation((x, y, z)))
    glass_uv(shoulder, color)
    flat_label("Label", x - s + 0.006, x + s - 0.006, y + 0.04, y + 0.13, z + s + 0.0015, k)
    common.cylinder("Cap", Vector((x, y + 0.245, z)), 0.0145, 0.03, "y", cap_dark, segments=12, bevel=0.001)


KINDS = ["whiskey", "slim", "stubby", "liqueur", "wine", "square"]
count = 0
for si, sy in enumerate(SHELVES):
    y = sy + 0.0175
    for bay in range(3):
        x0 = POSTS[bay] + POST_W + 0.06
        x1 = POSTS[bay + 1] - 0.06
        x = x0
        while x < x1 - 0.04:
            kind = rng.choice(KINDS)
            zj = SHELF_D * 0.55 + rng.uniform(-0.03, 0.03)
            color = rng.randrange(8)
            k = rng.randrange(16)
            if kind == "square":
                square_bottle(x + 0.035, y, zj, color, k)
            else:
                round_bottle(x + 0.04, y, zj, kind, color, k)
            count += 1
            x += 0.14 + rng.uniform(0.0, 0.06)
# A few bottles standing on the cabinet top too (the "speed rail" favorites).
for x in (-2.6, -2.45, -0.15, 0.0, 2.3, 2.45, 2.6):
    round_bottle(x, 0.995, 0.38, rng.choice(KINDS[:5]), rng.randrange(8), rng.randrange(16))
    count += 1
print("BOTTLES", count)

common.finish_and_export(out_path, uv_scale={"Back bar oak": 0.9, "Back bar top": 0.9})
