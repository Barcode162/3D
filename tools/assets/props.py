# Builds the small things on the bar and tables: the beer tap tower with
# its drip tray, coasters, a chrome napkin holder and a menu card in a
# wooden stand.
#
#   blender -b --factory-startup --python tools/assets/props.py -- <texture dir> <out dir>
#
# Each prop stands on its origin (bottom center), y up, front +z.
import math
import os
import sys

import bmesh
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402

tex_dir, out_dir = sys.argv[sys.argv.index("--") + 1:][:2]


def beer_taps():
    """A T-shaped chrome tower with four faucets; the faucets face +z
    (the bartender's side, so the scene turns it around)."""
    common.reset_scene()
    chrome = common.plain_material("Chrome", (0.92, 0.92, 0.94), 1.0, 0.08)
    black = common.plain_material("Black rubber", (0.015, 0.015, 0.015), 0.0, 0.7)
    handle_mats = [common.plain_material("Handle black", (0.02, 0.02, 0.02), 0.2, 0.3),
                   common.pbr_material("Handle wood", tex_dir, "wood_table_001", tint=(1.2, 1.0, 0.85),
                                       roughness_scale=0.5, grain="v", size=256),
                   common.plain_material("Handle red", (0.5, 0.04, 0.03), 0.1, 0.3),
                   common.plain_material("Handle cream", (0.8, 0.75, 0.6), 0.1, 0.3)]
    gold = common.plain_material("Brass", (0.91, 0.70, 0.38), 1.0, 0.3)
    # Base plate and column.
    common.cylinder("Base", Vector((0, 0.006, 0)), 0.07, 0.012, "y", chrome, segments=28)
    common.cylinder("Column", Vector((0, 0.19, 0)), 0.04, 0.36, "y", chrome, segments=24)
    # Cross bar with rounded ends.
    common.cylinder("Cross bar", Vector((0, 0.37, 0)), 0.035, 0.5, "x", chrome, segments=24)
    for x in (-0.25, 0.25):
        common.sphere("End", Vector((x, 0.37, 0)), 0.035, chrome, 20, 10)
    for i, x in enumerate((-0.18, -0.06, 0.06, 0.18)):
        # Shank, faucet body, spout, lever and a tall handle.
        common.cylinder("Shank", Vector((x, 0.37, 0.05)), 0.012, 0.04, "z", chrome, segments=12)
        common.cylinder("Faucet", Vector((x, 0.37, 0.09)), 0.017, 0.05, "z", chrome, segments=16)
        common.tube("Spout", [Vector((x, 0.36, 0.11)), Vector((x, 0.34, 0.115)), Vector((x, 0.31, 0.115))], 0.008,
                    chrome, sides=10)
        common.cylinder("Lever", Vector((x, 0.40, 0.1)), 0.006, 0.04, "y", chrome, segments=8)
        hm = handle_mats[i % 4]
        handle = common.lathe("Handle", [(0.0, 0.42), (0.012, 0.42), (0.016, 0.45), (0.02, 0.56), (0.017, 0.6),
                                         (0.0, 0.605)], hm, 16)
        common.transform(handle, Matrix.Translation((x, 0, 0.1)))
        common.cylinder("Ferrule", Vector((x, 0.425, 0.1)), 0.013, 0.012, "y", gold, segments=12)
    # Drip tray under the spouts (on the bar top).
    common.box("Drip tray", (-0.27, 0.0, 0.05), (0.27, 0.025, 0.2), chrome, 0.006)
    for k in range(12):
        x = -0.24 + k * 0.0436
        common.box("Tray slot", (x, 0.024, 0.065), (x + 0.025, 0.027, 0.185), black, 0.0)
    common.finish_and_export(os.path.join(out_dir, "beer_taps.glb"), uv_scale={"Handle wood": 6.0})


def coaster():
    """A round cardboard coaster with a printed ring logo."""
    common.reset_scene()
    S = 128
    yy, xx = np.mgrid[0:S, 0:S]
    d = np.sqrt((xx - S / 2 + 0.5) ** 2 + (yy - S / 2 + 0.5) ** 2) / (S / 2)
    px = np.ones((S, S, 3), dtype=np.float32) * np.array((0.8, 0.72, 0.55), dtype=np.float32)
    px[(d > 0.86) & (d < 0.94)] = (0.45, 0.06, 0.05)
    px[(d > 0.55) & (d < 0.6)] = (0.45, 0.06, 0.05)
    px[d < 0.42] = (0.45, 0.06, 0.05)
    common.draw_text(px, "BAR", S // 2, S // 2 + 9, 3, (0.85, 0.78, 0.6), center=True)
    printed = common.image_material("Coaster print", px, roughness=0.85)
    edge = common.plain_material("Coaster edge", (0.7, 0.62, 0.48), 0.0, 0.9)
    common.cylinder("Coaster", Vector((0, 0.002, 0)), 0.05, 0.004, "y", edge, segments=32, bevel=0.0)
    top = common.cylinder("Print", Vector((0, 0.00405, 0)), 0.0498, 0.0001, "y", printed, segments=32, bevel=0.0)
    common.set_uvs(top, lambda co, n: (co.x / 0.1 + 0.5, -co.z / 0.1 + 0.5))
    common.finish_and_export(os.path.join(out_dir, "coaster.glb"), uv_scale={})


def napkin_holder():
    common.reset_scene()
    chrome = common.plain_material("Chrome", (0.92, 0.92, 0.94), 1.0, 0.1)
    paper = common.plain_material("Napkins", (0.92, 0.9, 0.86), 0.0, 0.95)
    common.box("Base", (-0.075, 0.0, -0.045), (0.075, 0.012, 0.045), chrome, 0.004)
    for z in (-0.035, 0.035):
        common.box("Side", (-0.07, 0.012, z - 0.002), (0.07, 0.11, z + 0.002), chrome, 0.0015)
    # Stack of folded napkins, slightly fanned at the top.
    for k in range(12):
        z = -0.03 + k * 0.0055
        common.box("Napkin", (-0.065, 0.012, z), (0.065, 0.125 + 0.004 * math.sin(k), z + 0.005), paper, 0.0015)
    common.finish_and_export(os.path.join(out_dir, "napkin_holder.glb"), uv_scale={})


def menu_stand():
    """A small card with the drinks list in a wooden block."""
    common.reset_scene()
    wood = common.pbr_material("Stand wood", tex_dir, "wood_table_001", tint=(1.1, 1.0, 0.9), roughness_scale=0.6,
                               size=256)
    W, H = 160, 224
    px = np.ones((H, W, 3), dtype=np.float32) * np.array((0.9, 0.86, 0.76), dtype=np.float32)
    px[4:8, 6:W - 6] = px[H - 8:H - 4, 6:W - 6] = (0.35, 0.05, 0.04)
    common.draw_text(px, "DRINKS", W // 2, H - 16, 3, (0.35, 0.05, 0.04), center=True)
    items = ["OLD FASHIONED", "NEGRONI", "MARTINI", "MANHATTAN", "MOJITO", "DAIQUIRI", "BEER", "WINE"]
    for i, item in enumerate(items):
        y = H - 52 - i * 20
        common.draw_text(px, item, 12, y, 1, (0.12, 0.1, 0.08))
        common.draw_text(px, str(8 + (i * 3) % 7), W - 22, y, 1, (0.12, 0.1, 0.08))
    card = common.image_material("Menu card", px, roughness=0.8)
    common.box("Block", (-0.045, 0.0, -0.02), (0.045, 0.025, 0.02), wood, 0.004)
    bm = bmesh.new()
    w, h = 0.08, 0.112
    for zz, flip in ((0.0008, False), (-0.0008, True)):
        v = [bm.verts.new(p) for p in ((-w / 2, 0.015, zz), (w / 2, 0.015, zz), (w / 2, 0.015 + h, zz),
                                       (-w / 2, 0.015 + h, zz))]
        bm.faces.new(list(reversed(v)) if flip else v)
    obj = common.mesh_object("Card", bm, card)
    common.set_uvs(obj, lambda co, n: ((co.x + w / 2) / w if n.z > 0 else 1 - (co.x + w / 2) / w, (co.y - 0.015) / h))
    common.finish_and_export(os.path.join(out_dir, "menu_stand.glb"), uv_scale={"Stand wood": 6.0})


beer_taps()
coaster()
napkin_holder()
menu_stand()
