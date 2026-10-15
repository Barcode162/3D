# Builds the wall decor: a dartboard, a chalkboard for scores and specials,
# two framed posters, and the frames for the two mirrors.
#
#   blender -b --factory-startup --python tools/assets/decor.py -- <texture dir> <out dir>
#
# Wall pieces: origin at the center of their back (against the wall), the
# front faces +z.
import math
import os
import sys

import bmesh
import numpy as np
from mathutils import Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402

tex_dir, out_dir = sys.argv[sys.argv.index("--") + 1:][:2]


def disc_face(name, r, z, mat, segments=64):
    """A flat disc facing +z with planar UVs over its square."""
    bm = bmesh.new()
    v = [bm.verts.new((r * math.cos(2 * math.pi * k / segments), r * math.sin(2 * math.pi * k / segments), z))
         for k in range(segments)]
    bm.faces.new(v)
    obj = common.mesh_object(name, bm, mat)
    common.set_uvs(obj, lambda co, n: (co.x / (2 * r) + 0.5, co.y / (2 * r) + 0.5))
    return obj


def quad(name, x0, x1, y0, y1, z, mat):
    bm = bmesh.new()
    v = [bm.verts.new(p) for p in ((x0, y0, z), (x1, y0, z), (x1, y1, z), (x0, y1, z))]
    bm.faces.new(v)
    obj = common.mesh_object(name, bm, mat)
    common.set_uvs(obj, lambda co, n: ((co.x - x0) / (x1 - x0), (co.y - y0) / (y1 - y0)))
    return obj


def frame(name, w, h, depth, width, mat, bevel=0.008):
    """A picture-frame molding around a w x h opening (centered at x, y = 0)."""
    hw, hh = w / 2, h / 2
    common.box(name, (-hw - width, -hh - width, 0), (hw + width, -hh, depth), mat, bevel, 3)
    common.box(name, (-hw - width, hh, 0), (hw + width, hh + width, depth), mat, bevel, 3)
    common.box(name, (-hw - width, -hh, 0), (-hw, hh, depth), mat, bevel, 3)["grain_axis"] = "y"
    common.box(name, (hw, -hh, 0), (hw + width, hh, depth), mat, bevel, 3)["grain_axis"] = "y"


def dartboard():
    common.reset_scene()
    S = 512
    R = 0.225  # playing area radius, m
    yy, xx = np.mgrid[0:S, 0:S]
    x = (xx - S / 2 + 0.5) / (S / 2) * 0.2  # meters on the board (texture covers the 0.2 m face)
    y = (yy - S / 2 + 0.5) / (S / 2) * 0.2
    d = np.sqrt(x * x + y * y)
    ang = (np.degrees(np.arctan2(y, x)) + 360 + 9) % 360  # 20 sectors, 20 at the top
    sector = (ang // 18).astype(int)
    black, cream = (0.02, 0.02, 0.02), (0.82, 0.74, 0.55)
    red, green = (0.6, 0.04, 0.03), (0.02, 0.32, 0.12)
    px = np.zeros((S, S, 3), dtype=np.float32)
    even = (sector % 2) == 0
    px[even] = black
    px[~even] = cream
    ring = ((d > 0.099) & (d < 0.107)) | ((d > 0.162) & (d < 0.170))
    px[ring & even] = red
    px[ring & ~even] = green
    px[d < 0.016] = green
    px[d < 0.0064] = red
    px[d > 0.170] = (0.02, 0.02, 0.02)
    # Sisal fibres: speckle.
    rng = np.random.default_rng(5)
    px *= (0.85 + 0.15 * rng.random((S, S, 1))).astype(np.float32)
    # Numbers ring.
    nums = [20, 1, 18, 4, 13, 6, 10, 15, 2, 17, 3, 19, 7, 16, 8, 11, 14, 9, 12, 5]
    for i, n in enumerate(nums):
        a = math.radians(90 - i * 18)
        cx = S / 2 + math.cos(a) * 0.186 / 0.2 * S / 2
        cy = S / 2 + math.sin(a) * 0.186 / 0.2 * S / 2
        common.draw_text(px, str(n), int(cx), int(cy + 7), 2, (0.85, 0.85, 0.85), center=True)
    face = common.image_material("Dartboard face", px, roughness=0.95)
    wire = common.plain_material("Wire", (0.8, 0.8, 0.82), 1.0, 0.25)
    rim = common.plain_material("Board rim", (0.02, 0.02, 0.02), 0.0, 0.7)
    cabinet = common.pbr_material("Cabinet wood", tex_dir, "dark_wood", tint=(0.6, 0.48, 0.42),
                                  roughness_scale=0.6, grain="v", size=512)
    # Board body and printed face.
    common.cylinder("Board", Vector((0, 0, 0.02)), R, 0.04, "z", rim, segments=64, bevel=0.005)
    disc_face("Face", 0.2, 0.0402, face)
    # Wire spider: rings and radial wires.
    for r in (0.0064, 0.016, 0.099, 0.107, 0.162, 0.170):
        common.torus("Ring wire", Vector((0, 0, 0.041)), r, 0.0009, wire, "z", 48, 4)
    for i in range(20):
        a = math.radians(90 - 9 - i * 18)
        common.tube("Spoke wire", [Vector((math.cos(a) * 0.016, math.sin(a) * 0.016, 0.041)),
                                   Vector((math.cos(a) * 0.17, math.sin(a) * 0.17, 0.041))], 0.0009, wire, sides=4,
                    caps=False)
    # Open cabinet behind: back board and side doors.
    common.box("Cabinet back", (-0.32, -0.32, -0.025), (0.32, 0.32, 0.0), cabinet, 0.004)
    frame("Cabinet frame", 0.6, 0.6, 0.08, 0.04, cabinet, 0.008)
    # The two doors stand fully open, flat against the wall either side.
    for side in (-1, 1):
        x0, x1 = (0.36, 0.66) if side > 0 else (-0.66, -0.36)
        common.box("Door", (x0, -0.32, 0.0), (x1, 0.32, 0.025), cabinet, 0.004)["grain_axis"] = "y"
        common.box("Door trim", (x0 + 0.03, -0.29, 0.025), (x1 - 0.03, 0.29, 0.03), cabinet, 0.003)["grain_axis"] = "y"
    common.finish_and_export(os.path.join(out_dir, "dartboard.glb"), uv_scale={"Cabinet wood": 1.5})


def chalkboard():
    common.reset_scene()
    W, H = 384, 512
    rng = np.random.default_rng(9)
    px = np.zeros((H, W, 3), dtype=np.float32)
    px[:] = (0.035, 0.05, 0.045)
    px *= (0.8 + 0.4 * rng.random((H, W, 1))).astype(np.float32)
    # Chalk smudges.
    yy, xx = np.mgrid[0:H, 0:W]
    for _ in range(6):
        cx, cy, r = rng.uniform(0, W), rng.uniform(0, H), rng.uniform(40, 120)
        px += (0.03 * np.exp(-((xx - cx) ** 2 + (yy - cy) ** 2) / (r * r)))[..., None].astype(np.float32)
    chalk, pink, yellow = (0.85, 0.85, 0.8), (0.9, 0.55, 0.6), (0.9, 0.85, 0.5)
    common.draw_text(px, "TONIGHT", W // 2, H - 24, 4, yellow, center=True)
    lines = [("JAZZ ON THE", chalk), ("JUKEBOX", chalk), ("", chalk), ("HAPPY HOUR", pink), ("5 - 7 PM", pink),
             ("", chalk), ("DARTS", yellow), ("JO    301", chalk), ("SAM   287", chalk), ("ALEX  240", chalk)]
    for i, (t, c) in enumerate(lines):
        common.draw_text(px, t, 28, H - 80 - i * 40, 3, c)
    board = common.image_material("Chalkboard", px, roughness=0.95)
    wood = common.pbr_material("Frame wood", tex_dir, "wood_table_001", tint=(1.0, 0.9, 0.8), roughness_scale=0.6,
                               size=512)
    w, h = 0.6, 0.8
    common.box("Back", (-w / 2, -h / 2, 0.0), (w / 2, h / 2, 0.012), board, 0.0)
    quad("Board", -w / 2, w / 2, -h / 2, h / 2, 0.0125, board)
    frame("Frame", w, h, 0.03, 0.045, wood, 0.01)
    common.box("Chalk ledge", (-w / 2, -h / 2 - 0.045, 0.0), (w / 2, -h / 2 - 0.03, 0.06), wood, 0.004)
    chalk_m = common.plain_material("Chalk", (0.9, 0.9, 0.86), 0.0, 0.95)
    for (x, l) in ((-0.15, 0.05), (0.1, 0.035)):
        common.cylinder("Chalk", Vector((x, -h / 2 - 0.025, 0.035)), 0.005, l, "x", chalk_m, segments=8)
    common.finish_and_export(os.path.join(out_dir, "chalkboard.glb"), uv_scale={"Frame wood": 1.5})


def poster(name, art):
    common.reset_scene()
    W, H = 320, 448
    px = art(W, H)
    print_mat = common.image_material(f"{name} print", px, roughness=0.6)
    gold = common.plain_material("Gilt frame", (0.75, 0.55, 0.28), 1.0, 0.35)
    mat_board = common.plain_material("Mat board", (0.85, 0.82, 0.74), 0.0, 0.9)
    w, h = 0.5, 0.7
    common.box("Backing", (-w / 2 - 0.06, -h / 2 - 0.06, 0.0), (w / 2 + 0.06, h / 2 + 0.06, 0.01), mat_board, 0.0)
    quad("Print", -w / 2, w / 2, -h / 2, h / 2, 0.0105, print_mat)
    frame("Frame", w + 0.12, h + 0.12, 0.035, 0.04, gold, 0.01)
    common.finish_and_export(os.path.join(out_dir, f"{name}.glb"), uv_scale={})


def jazz_art(W, H):
    yy, xx = np.mgrid[0:H, 0:W]
    t = (yy / H)[..., None]
    px = (np.array((0.55, 0.12, 0.05)) * (1 - t) + np.array((0.95, 0.6, 0.2)) * t).astype(np.float32)
    # Big sun and a saxophone-ish silhouette made of circles and a bent bar.
    d = np.sqrt((xx - W * 0.5) ** 2 + (yy - H * 0.55) ** 2)
    px[d < W * 0.32] = px[d < W * 0.32] * 0.6 + np.array((1.0, 0.85, 0.5)) * 0.4
    body = ((np.abs(xx - W * 0.52 - (yy - H * 0.4) * 0.25) < 14) & (yy > H * 0.3) & (yy < H * 0.75))
    bell = np.sqrt((xx - W * 0.42) ** 2 + (yy - H * 0.3) ** 2) < 30
    px[body | bell] = (0.08, 0.04, 0.03)
    for k in range(6):
        px[np.sqrt((xx - W * 0.555 - (H * 0.42 + k * 22 - H * 0.4) * 0.25) ** 2 + (yy - H * 0.42 - k * 22) ** 2) < 5] = \
            (0.95, 0.75, 0.35)
    px[:int(H * 0.2)] = (0.08, 0.04, 0.03)
    common.draw_text(px, "JAZZ", W // 2, int(H * 0.17), 7, (0.95, 0.7, 0.3), center=True)
    common.draw_text(px, "LIVE TONIGHT", W // 2, int(H * 0.17) - 56, 2, (0.9, 0.85, 0.7), center=True)
    common.draw_text(px, "CLUB", W // 2, H - 20, 4, (0.25, 0.05, 0.03), center=True)
    return px


def cocktail_art(W, H):
    yy, xx = np.mgrid[0:H, 0:W]
    px = np.zeros((H, W, 3), dtype=np.float32)
    px[:] = (0.05, 0.15, 0.17)
    stripes = ((xx + yy) // 24) % 2 == 0
    px[stripes] = (0.06, 0.18, 0.2)
    # A coupe glass: bowl (half disc), stem, foot.
    cx, top = W * 0.5, H * 0.66
    bowl = (np.sqrt((xx - cx) ** 2 + (yy - top) ** 2) < 90) & (yy < top)
    px[bowl] = (0.85, 0.85, 0.8)
    drink = (np.sqrt((xx - cx) ** 2 + (yy - top) ** 2) < 82) & (yy < top - 4)
    px[drink] = (0.75, 0.15, 0.2)
    px[(np.abs(xx - cx) < 4) & (yy > top - 210) & (yy < top - 88)] = (0.85, 0.85, 0.8)
    px[(np.abs(xx - cx) < 50) & (yy > top - 216) & (yy < top - 206)] = (0.85, 0.85, 0.8)
    cherry = np.sqrt((xx - cx - 40) ** 2 + (yy - top - 10) ** 2) < 14
    px[cherry] = (0.8, 0.05, 0.08)
    px[(np.abs(xx - cx - 40 - (yy - top - 10) * 0.3) < 2) & (yy > top + 10) & (yy < top + 60)] = (0.2, 0.4, 0.1)
    common.draw_text(px, "COCKTAILS", W // 2, H - 22, 3, (0.95, 0.8, 0.5), center=True)
    common.draw_text(px, "SINCE 1931", W // 2, 42, 2, (0.95, 0.8, 0.5), center=True)
    return px


def mirror_frame(name, mat_fn):
    """The frame around a mirror's 1.35 x 1.95 m glass (the glass itself is
    drawn by the engine, just in front of the backing)."""
    common.reset_scene()
    mat = mat_fn()
    backing = common.plain_material("Mirror backing", (0.02, 0.02, 0.02), 0.0, 0.8)
    w, h = 1.35, 1.95
    common.box("Backing", (-w / 2, -h / 2, 0.0), (w / 2, h / 2, 0.02), backing, 0.0)
    frame("Frame", w, h, 0.06, 0.075, mat, 0.012)
    frame("Inner bead", w, h, 0.075, 0.018, mat, 0.008)
    common.finish_and_export(os.path.join(out_dir, f"{name}.glb"), uv_scale={})


dartboard()
chalkboard()
poster("poster_jazz", jazz_art)
poster("poster_cocktails", cocktail_art)
mirror_frame("mirror_frame_brass", lambda: common.plain_material("Brass", (0.91, 0.70, 0.38), 1.0, 0.28))
mirror_frame("mirror_frame_black", lambda: common.plain_material("Black lacquer", (0.02, 0.02, 0.02), 0.0, 0.2))
