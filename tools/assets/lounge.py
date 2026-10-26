# The lounge by the fire on the left wall (docs/archive/BAR_REDESIGN.md),
# taken apart (the world remake J70): every object is its own model, made
# here in the room's place and saved standing on its own origin (its
# footprint's middle on the floor, its front toward +z), so the place file
# puts each where it stood and the Studio can move it. Our own: the
# fireplace (firebox, marble surround, mantel, hearth, grate, logs, embers),
# candlesticks, photos, books, fire tools, a log basket, the rug, the coffee
# table with the chessboard, the bookshelf with its books and plants, the
# floor lamp and the side table. From Poly Haven (build_all.sh): the
# Chesterfield sofa, the armchairs, the mantel clock, the painting, the vase.
# The flames are their own model too (fire_flames.glb, see-through glowing
# cards).
#
#   blender -b --factory-startup --python tools/assets/lounge.py -- <texture dir> <out dir>
#
# Each model's place in the room: LOUNGE_PLACES below (the place file uses
# the same numbers).
import math
import os
import random
import sys

import bpy
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402
import layout as L  # noqa: E402
import props_lib  # noqa: E402

tex_dir, out_dir = sys.argv[sys.argv.index("--") + 1:][:2]

FX = L.X0 + L.CHIMNEY_D            # face of the chimney breast
FZ = (L.CHIMNEY_Z0 + L.CHIMNEY_Z1) / 2
OPEN_W, OPEN_H = 0.9, 0.78         # firebox opening


def mats(P):
    """The lounge's materials (made again for each model: each is its own
    Blender scene)."""
    m = {}
    m["marble"] = common.pbr_material("Surround marble", tex_dir, "marble_01", tint=(0.42, 0.4, 0.38),
                                      roughness_scale=0.4, size=512)
    m["wood"] = common.pbr_material("Mantel wood", tex_dir, "dark_wood", tint=(0.55, 0.35, 0.28), roughness_scale=0.5,
                                    size=512)
    m["soot"] = common.pbr_material("Firebox brick", tex_dir, "brick_wall_006", tint=(0.12, 0.1, 0.1), size=256)
    m["iron"] = common.plain_material("Cast iron", (0.04, 0.04, 0.045), 0.7, 0.55)
    m["bark"] = common.pbr_material("Log bark", tex_dir, "dark_wood", tint=(0.3, 0.22, 0.17), size=256)
    m["log_end"] = common.plain_material("Log end", (0.55, 0.38, 0.22), 0.0, 0.8)
    m["ember"] = common.plain_material("Embers", (1.0, 0.35, 0.05), 0.0, 0.6, emission=(1.0, 0.28, 0.04), strength=6.0)
    m["shelf"] = common.pbr_material("Shelf wood", tex_dir, "wood_table_001", tint=(0.5, 0.33, 0.24), size=512)
    m["brass"] = P.mat("brass")
    return m


UV = {"Surround marble": 1.5, "Mantel wood": 1.5, "Firebox brick": 2.0, "Log bark": 4.0, "Shelf wood": 1.2}
z0, z1 = FZ - OPEN_W / 2, FZ + OPEN_W / 2
SW = 0.24
MY = OPEN_H + SW + 0.21  # the mantel shelf's top

# Where each model stands in the room: (x, y, z, turn in degrees). Its front
# (+z in the model) faces where `turn` points it; the place file uses these.
LOUNGE_PLACES = {
    "fireplace": (FX, 0.0, FZ, 90),
    "candlestick-1": (FX + 0.12, MY, z0 - 0.1, 90),
    "candlestick-2": (FX + 0.12, MY, z1 + 0.1, 90),
    "mantel_photo_a": (FX + 0.06, MY, z1 + 0.33, 90 + math.degrees(0.3)),
    "mantel_photo_b": (FX + 0.06, MY, z0 - 0.35, 90 - math.degrees(0.2)),
    "mantel_books": (FX + 0.13, MY, z0 - 0.62, 90),
    "fire_tools": (FX + 0.35, 0.0, z1 + SW + 0.35, 90),
    "log_basket": (FX + 0.35, 0.0, z0 - SW - 0.4, 90),
    "rug": (-7.12, 0.0, FZ, 0),
    "coffee_table": (-6.95, 0.0, FZ, 0),
    "bookshelf": (L.X0, 0.0, None, 90),  # z: the middle between the chimney and the stage (below)
    "floor_lamp": (-6.3, 0.0, FZ + 1.95, 0),
    "side_table": (-5.85, 0.0, FZ + 1.85, 0),
}
BZ0, BZ1, BH, BD = L.CHIMNEY_Z1 + 0.12, L.STAGE_Z0 - L.STEP_W - 0.12, 2.3, 0.34
LOUNGE_PLACES["bookshelf"] = (L.X0, 0.0, (BZ0 + BZ1) / 2, 90)


def export_here(name, uv=None, image_format="JPEG"):
    """Moves everything built (in the room's place) onto the model's own
    origin and facing, and saves <out dir>/<name>.glb."""
    x, y, z, turn = LOUNGE_PLACES[name.split("-")[0] if name not in LOUNGE_PLACES else name]
    m = Matrix.Rotation(-math.radians(turn), 4, "Y") @ Matrix.Translation((-x, -y, -z))
    for o in bpy.context.scene.objects:
        if o.type == "MESH":
            o.data.transform(m)
    common.finish_and_export(os.path.join(out_dir, name.split("-")[0] + ".glb"), uv_scale=uv or UV,
                             image_format=image_format)


def box(name, lo, hi, mat, bevel=0.006, seg=2, ax="x"):
    o = common.box(name, lo, hi, mat, bevel, seg)
    o["grain_axis"] = ax
    return o


def build_fireplace():
    common.reset_scene()
    P = props_lib.Props(tex_dir, seed=77)
    M = mats(P)
    rng = random.Random(5)
    soot, marble, wood, iron = M["soot"], M["marble"], M["wood"], M["iron"]
    box("Firebox back", (FX - 0.42, 0.0, z0 - 0.02), (FX - 0.38, OPEN_H, z1 + 0.02), soot, 0, ax="z")
    for zz in (z0, z1):
        box("Firebox side", (FX - 0.4, 0.0, zz - 0.02), (FX, OPEN_H, zz + 0.02), soot, 0, ax="z")
    box("Firebox top", (FX - 0.4, OPEN_H - 0.02, z0), (FX, OPEN_H + 0.02, z1), soot, 0, ax="z")
    box("Firebox floor", (FX - 0.4, 0.0, z0), (FX, 0.02, z1), soot, 0, ax="z")
    box("Surround left", (FX, 0.0, z0 - SW), (FX + 0.05, OPEN_H + SW, z0), marble, 0.006, ax="y")
    box("Surround right", (FX, 0.0, z1), (FX + 0.05, OPEN_H + SW, z1 + SW), marble, 0.006, ax="y")
    box("Surround top", (FX, OPEN_H, z0), (FX + 0.05, OPEN_H + SW, z1), marble, 0.006, ax="z")
    box("Mantel frieze", (FX, OPEN_H + SW, z0 - SW - 0.06), (FX + 0.09, OPEN_H + SW + 0.16, z1 + SW + 0.06),
        wood, 0.01, 3, ax="z")
    box("Mantel shelf", (FX, OPEN_H + SW + 0.16, z0 - SW - 0.14), (FX + 0.26, MY, z1 + SW + 0.14), wood, 0.015, 3,
        ax="z")
    for zz in (z0 - SW - 0.03, z1 + SW + 0.03):
        box("Pilaster", (FX, 0.0, zz - 0.05), (FX + 0.09, OPEN_H + SW, zz + 0.05), wood, 0.01, 3, ax="y")
        box("Corbel", (FX + 0.09, OPEN_H + SW + 0.02, zz - 0.045), (FX + 0.2, OPEN_H + SW + 0.16, zz + 0.045), wood,
            0.02, 3)
    box("Hearth", (FX, 0.0, z0 - SW - 0.15), (FX + 0.5, 0.04, z1 + SW + 0.15), marble, 0.01, 3, ax="z")
    for k in range(6):
        zz = z0 + 0.12 + k * (OPEN_W - 0.24) / 5
        box("Grate bar", (FX - 0.32, 0.08, zz - 0.008), (FX - 0.05, 0.1, zz + 0.008), iron, 0.003)
    for zz in (z0 + 0.12, z1 - 0.12):
        box("Grate leg", (FX - 0.08, 0.0, zz - 0.012), (FX - 0.05, 0.16, zz + 0.012), iron, 0.003)
    box("Grate front", (FX - 0.07, 0.1, z0 + 0.1), (FX - 0.05, 0.16, z1 - 0.1), iron, 0.003, ax="z")
    for (a, y, zz, ln) in ((0.25, 0.14, FZ - 0.08, 0.62), (-0.2, 0.15, FZ + 0.1, 0.58), (0.05, 0.24, FZ, 0.55)):
        lg = common.cylinder("Log", Vector((0, 0, 0)), 0.055, ln, "z", M["bark"], segments=12)
        lg.data.transform(Matrix.Translation((FX - 0.2, y, zz)) @ Matrix.Rotation(a, 4, "Y"))
        for e in (-1, 1):
            cap = common.cylinder("Log end", Vector((0, 0, e * ln / 2)), 0.05, 0.006, "z", M["log_end"], segments=12)
            cap.data.transform(Matrix.Translation((FX - 0.2, y, zz)) @ Matrix.Rotation(a, 4, "Y"))
    for k in range(40):
        p = Vector((FX - 0.32 + rng.uniform(0, 0.26), 0.1 + rng.uniform(0, 0.03), z0 + 0.1 + rng.uniform(0, OPEN_W - 0.2)))
        common.sphere("Ember", p, rng.uniform(0.012, 0.025), M["ember"], 6, 4)
    export_here("fireplace")


def build_mantel_things():
    for name, zz in (("candlestick-1", z0 - 0.1),):
        common.reset_scene()
        P = props_lib.Props(tex_dir, seed=77)
        brass = P.mat("brass")
        common.lathe("Candlestick", [(0, 0), (0.05, 0), (0.05, 0.015), (0.015, 0.04), (0.012, 0.2), (0.03, 0.22),
                                     (0, 0.22)], brass, 16).data.transform(Matrix.Translation((FX + 0.12, MY, zz)))
        P.candle(FX + 0.12, MY + 0.22, zz, h=0.16, r=0.012)
        export_here(name)
    # Standing on the mantel (they floated 3 cm above it).
    for name, photo, size, lift in (("mantel_photo_a", 1, (0.14, 0.18), 0.09), ("mantel_photo_b", 7, (0.18, 0.14), 0.07)):
        common.reset_scene()
        P = props_lib.Props(tex_dir, seed=77)
        x, y, z, turn = LOUNGE_PLACES[name]
        P.frame(x, y + lift, z, size[0], size[1], photo, yaw=math.radians(turn), depth=0.02, border=0.02)
        export_here(name)
    common.reset_scene()
    P = props_lib.Props(tex_dir, seed=77)
    for k in range(3):
        P.book(FX + 0.13, MY + k * 0.03, z0 - 0.62, 0.03, 0.22, 0.15, yaw=math.pi / 2, lying=True)
    export_here("mantel_books")


def build_hearth_things():
    common.reset_scene()
    P = props_lib.Props(tex_dir, seed=77)
    M = mats(P)
    iron, brass = M["iron"], M["brass"]
    tx, _, tz, _ = LOUNGE_PLACES["fire_tools"]
    common.cylinder("Tool stand", Vector((tx, 0.4, tz)), 0.01, 0.8, "y", iron, segments=8)
    common.cylinder("Tool base", Vector((tx, 0.01, tz)), 0.11, 0.02, "y", iron, segments=16)
    common.torus("Tool ring", Vector((tx, 0.78, tz)), 0.05, 0.006, iron, segments=16, sides=4)
    for k in range(4):
        a = k * math.pi / 2
        common.tube("Fire tool", [(tx + 0.05 * math.cos(a), 0.82, tz + 0.05 * math.sin(a)),
                                  (tx + 0.06 * math.cos(a), 0.1, tz + 0.06 * math.sin(a))], 0.006, iron, sides=5)
        common.sphere("Tool handle", Vector((tx + 0.05 * math.cos(a), 0.84, tz + 0.05 * math.sin(a))), 0.015, brass,
                      8, 6)
    export_here("fire_tools")
    common.reset_scene()
    P = props_lib.Props(tex_dir, seed=77)
    M = mats(P)
    bx, _, bz, _ = LOUNGE_PLACES["log_basket"]
    common.lathe("Log basket", [(0, 0), (0.2, 0), (0.24, 0.3), (0.22, 0.3), (0.18, 0.02), (0, 0.02)], M["iron"], 16
                 ).data.transform(Matrix.Translation((bx, 0, bz)))
    for k in range(5):
        lg = common.cylinder("Basket log", Vector((0, 0, 0)), 0.045, 0.42, "x", M["bark"], segments=10)
        lg.data.transform(Matrix.Translation((bx, 0.12 + (k // 3) * 0.09, bz - 0.09 + (k % 3) * 0.09)) @
                          Matrix.Rotation(0.3 * k, 4, "Y"))
    export_here("log_basket")


def build_rug():
    common.reset_scene()
    rug_px = np.zeros((360, 300, 3), np.float32)
    yy, xx = np.mgrid[0:360, 0:300] / np.array([360.0, 300.0])[:, None, None]
    base = np.array(common.srgb((0.42, 0.06, 0.06)))
    rug_px[:] = base
    border = (xx < 0.08) | (xx > 0.92) | (yy < 0.07) | (yy > 0.93)
    rug_px[border] = common.srgb((0.08, 0.1, 0.22))
    inner_border = ((xx > 0.1) & (xx < 0.9) & (yy > 0.085) & (yy < 0.915)) & ~((xx > 0.12) & (xx < 0.88) & (yy > 0.1) & (yy < 0.9))
    rug_px[inner_border] = common.srgb((0.85, 0.7, 0.4))
    cx, cy = (xx - 0.5) * 2, (yy - 0.5) * 2
    medallion = (np.abs(cx) + np.abs(cy) * 0.8) < 0.5
    rug_px[medallion] = common.srgb((0.08, 0.1, 0.22))
    rug_px[(np.abs(cx) + np.abs(cy) * 0.8) < 0.3] = common.srgb((0.85, 0.7, 0.4))
    rug_px[(np.abs(cx) + np.abs(cy) * 0.8) < 0.15] = common.srgb((0.42, 0.06, 0.06))
    pattern = (np.sin(xx * 60) * np.sin(yy * 60) > 0.6) & ~border & ~medallion
    rug_px[pattern] = rug_px[pattern] * 0.6 + np.array(common.srgb((0.85, 0.7, 0.4))) * 0.4
    rug_px *= (0.85 + 0.15 * common.noise2d(360, 300, 40, 3))[..., None]
    rug = common.image_material("Rug", rug_px, roughness=0.95)
    # One slab (not a picture on top of a slab: those flicker), ending in
    # front of the sofa.
    RW, RD = 1.75, 3.3
    RX, _, RZ, _ = LOUNGE_PLACES["rug"]
    rug_obj = common.box("Rug", (RX - RW / 2, 0.0, RZ - RD / 2), (RX + RW / 2, 0.012, RZ + RD / 2), rug, 0.004)
    common.set_uvs(rug_obj, lambda co, n: ((co.x - (RX - RW / 2)) / RW, (co.z - (RZ - RD / 2)) / RD))
    export_here("rug")


def build_coffee_table():
    common.reset_scene()
    P = props_lib.Props(tex_dir, seed=77)
    M = mats(P)
    wood = M["wood"]
    TX, _, TZ, _ = LOUNGE_PLACES["coffee_table"]
    TY = 0.45
    box("Table top", (TX - 0.38, TY - 0.04, TZ - 0.6), (TX + 0.38, TY, TZ + 0.6), wood, 0.012, 3, ax="z")
    box("Table apron", (TX - 0.34, TY - 0.12, TZ - 0.56), (TX + 0.34, TY - 0.04, TZ + 0.56), wood, 0.006, ax="z")
    box("Table shelf", (TX - 0.32, 0.1, TZ - 0.54), (TX + 0.32, 0.13, TZ + 0.54), wood, 0.006, ax="z")
    for (dx, dz) in ((-0.33, -0.55), (0.33, -0.55), (-0.33, 0.55), (0.33, 0.55)):
        box("Table leg", (TX + dx - 0.03, 0.0, TZ + dz - 0.03), (TX + dx + 0.03, TY - 0.04, TZ + dz + 0.03), wood,
            0.008, ax="y")
    # The chessboard is played on (bar/projects/chess: its centre and square
    # size are the place file's "chess" spot): 0.44 m, centred on the table,
    # its squares at 0.4705 m.
    cb_px = np.zeros((128, 128, 3), np.float32)
    for i in range(8):
        for j in range(8):
            cb_px[i * 16:(i + 1) * 16, j * 16:(j + 1) * 16] = common.srgb((0.85, 0.75, 0.55)) if (i + j) % 2 else \
                common.srgb((0.3, 0.18, 0.1))
    chess = common.image_material("Chess board", cb_px, roughness=0.3)
    CB = 0.44
    box("Board base", (TX - CB / 2 - 0.025, TY, TZ - CB / 2 - 0.025), (TX + CB / 2 + 0.025, TY + 0.02, TZ + CB / 2 + 0.025),
        wood, 0.004)
    common.quad("Chess squares", [(TX - CB / 2, TY + 0.0205, TZ + CB / 2), (TX + CB / 2, TY + 0.0205, TZ + CB / 2),
                                  (TX + CB / 2, TY + 0.0205, TZ - CB / 2), (TX - CB / 2, TY + 0.0205, TZ - CB / 2)],
                chess, uvs=((0, 0), (1, 0), (1, 1), (0, 1)))
    P.glass(TX + 0.2, TY, TZ + 0.42, "rocks", "whiskey", ice=True)
    P.glass(TX - 0.2, TY, TZ - 0.42, "snifter", "whiskey")
    P.bottle(TX + 0.02, TY, TZ + 0.48, "square", color=0)
    for k in range(2):
        P.book(TX - 0.15, TY + k * 0.035, TZ + 0.45, 0.035, 0.2, 0.14, yaw=0.2 * k, lying=True)
    P.candle(TX + 0.2, TY, TZ - 0.5, h=0.1, r=0.03)
    export_here("coffee_table")


def build_bookshelf():
    common.reset_scene()
    P = props_lib.Props(tex_dir, seed=77)
    M = mats(P)
    shelf_wood = M["shelf"]
    X = L.X0
    box("Shelf side", (X, 0, BZ0), (X + BD, BH, BZ0 + 0.03), shelf_wood, 0.004, ax="y")
    box("Shelf side", (X, 0, BZ1 - 0.03), (X + BD, BH, BZ1), shelf_wood, 0.004, ax="y")
    box("Shelf back", (X, 0, BZ0), (X + 0.012, BH, BZ1), shelf_wood, 0.0, ax="z")
    box("Shelf crown", (X, BH, BZ0 - 0.03), (X + BD + 0.03, BH + 0.06, BZ1 + 0.03), shelf_wood, 0.01, 3, ax="z")
    box("Shelf plinth", (X, 0, BZ0), (X + BD, 0.1, BZ1), shelf_wood, 0.004, ax="z")
    for k, y in enumerate((0.1, 0.52, 0.94, 1.36, 1.78)):
        box("Shelf board", (X, y, BZ0 + 0.03), (X + BD, y + 0.025, BZ1 - 0.03), shelf_wood, 0.004, ax="z")
        length = BZ1 - BZ0 - 0.1
        P.book_row(X + BD / 2 + 0.02, y + 0.025, BZ0 + 0.05, length * (0.75 if k % 2 else 0.95), depth=0.2,
                   yaw=-math.pi / 2, max_h=0.36)
        if k % 2:
            P.plant(X + BD / 2, y + 0.025, BZ1 - 0.15, size=0.3, pot_r=0.06, pot_h=0.1, leaves=8, seed=k)
    P.plant(X + BD / 2, BH + 0.06, (BZ0 + BZ1) / 2, size=0.6, leaves=10, seed=9, hanging=True)
    export_here("bookshelf")


def build_lamp_and_table():
    common.reset_scene()
    P = props_lib.Props(tex_dir, seed=77)
    brass = P.mat("brass")
    LX, _, LZ, _ = LOUNGE_PLACES["floor_lamp"]
    common.cylinder("Lamp base", Vector((LX, 0.015, LZ)), 0.16, 0.03, "y", brass, segments=20)
    common.cylinder("Lamp pole", Vector((LX, 0.8, LZ)), 0.012, 1.6, "y", brass, segments=8)
    shade = common.lathe("Lamp shade", [(0.12, 1.45), (0.22, 1.45), (0.14, 1.72), (0.12, 1.72), (0.2, 1.46)],
                         common.plain_material("Shade fabric", (0.85, 0.7, 0.5), 0.0, 0.9, emission=(1.0, 0.7, 0.4),
                                               strength=0.8), 24)
    shade.data.transform(Matrix.Translation((LX, 0, LZ)))
    # The harp: three thin arms from the pole's top to the shade's rim, so the
    # shade is held (it hung in the air, 11 cm from the pole).
    for k in range(3):
        a = k * 2 * math.pi / 3
        common.tube("Lamp harp", [(LX, 1.6, LZ), (LX + 0.13 * math.cos(a), 1.7, LZ + 0.13 * math.sin(a))], 0.004, brass,
                    sides=5)
    common.sphere("Lamp bulb", Vector((LX, 1.55, LZ)), 0.04,
                  common.plain_material("Floor lamp bulb", (1, 0.9, 0.7), 0.0, 0.3, emission=(1, 0.75, 0.45),
                                        strength=8.0), 10, 6)
    export_here("floor_lamp")
    common.reset_scene()
    P = props_lib.Props(tex_dir, seed=77)
    M = mats(P)
    SX, _, SZ, _ = LOUNGE_PLACES["side_table"]
    common.cylinder("Side table", Vector((SX, 0.55, SZ)), 0.24, 0.03, "y", M["wood"], segments=24)
    common.cylinder("Side table post", Vector((SX, 0.28, SZ)), 0.03, 0.54, "y", M["wood"], segments=10)
    common.cylinder("Side table foot", Vector((SX, 0.015, SZ)), 0.18, 0.03, "y", M["wood"], segments=18)
    P.glass(SX - 0.05, 0.565, SZ + 0.05, "wine", "red_wine")
    P.book(SX + 0.07, 0.565, SZ - 0.05, 0.03, 0.2, 0.14, yaw=0.5, lying=True)
    export_here("side_table")


def build_lounge():
    build_fireplace()
    build_mantel_things()
    build_hearth_things()
    build_rug()
    build_coffee_table()
    build_bookshelf()
    build_lamp_and_table()


def build_flames():
    """Crossed glowing flame cards over the logs (see-through)."""
    common.reset_scene()
    w, h = 64, 128
    yy, xx = np.mgrid[0:h, 0:w] / np.array([h, w])[:, None, None]
    rgba = np.zeros((h, w, 4), np.float32)
    for k, (cx, height, width) in enumerate(((0.3, 0.75, 0.16), (0.55, 0.95, 0.2), (0.78, 0.6, 0.14))):
        shape = np.clip(1 - (np.abs(xx - cx) / (width * (1 - yy / height) + 1e-3)), 0, 1) * (yy < height)
        flick = 0.85 + 0.15 * np.sin(yy * 30 + k)
        a = np.clip(shape * 1.6 * flick, 0, 1)
        rgba[..., 3] = np.maximum(rgba[..., 3], a)
    heat = np.clip(1 - yy * 1.1, 0, 1)
    rgba[..., 0] = 1.0
    rgba[..., 1] = 0.25 + 0.6 * heat
    rgba[..., 2] = 0.05 + 0.25 * heat ** 3
    rgba[..., 3] *= np.clip(yy * 8, 0, 1)
    mat = common.glow_cutout_material("Flames", rgba, strength=3.0)
    for k, a in enumerate((0.0, math.pi / 3, -math.pi / 3, math.pi / 2)):
        q = common.quad("Flame card", [(-0.3, 0, 0), (0.3, 0, 0), (0.3, 0.5, 0), (-0.3, 0.5, 0)], mat)
        back = common.quad("Flame card", [(0.3, 0, 0), (-0.3, 0, 0), (-0.3, 0.5, 0), (0.3, 0.5, 0)], mat,
                           uvs=((1, 0), (0, 0), (0, 1), (1, 1)))
        for o in (q, back):
            o.data.transform(Matrix.Translation((FX - 0.2 + 0.03 * (k - 1.5), 0.12, FZ)) @ Matrix.Rotation(a + math.pi / 2, 4, "Y"))
    common.finish_and_export(os.path.join(out_dir, "fire_flames.glb"), uv_scale={}, image_format="AUTO")


build_lounge()
build_flames()
