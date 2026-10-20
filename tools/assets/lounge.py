# The lounge by the fire on the left wall (docs/BAR_REDESIGN.md): the
# fireplace on the chimney breast (marble surround, mantel shelf, hearth,
# firebox with grate, logs and glowing embers, a painting above, clock,
# candlesticks, photos, books, a vase on the mantel, fire tools, a log
# basket), a Chesterfield sofa, two wingback armchairs, a coffee table with
# a chess game, whiskey glasses and books, a patterned rug, a bookshelf
# full of books, a floor lamp and a side table. The flames themselves are a
# separate model (fire_flames.glb, see-through glowing cards).
#
#   blender -b --factory-startup --python tools/assets/lounge.py -- <texture dir> <out dir>
#
# Model space = world space.
import math
import os
import random
import sys

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


def build_lounge():
    common.reset_scene()
    P = props_lib.Props(tex_dir, seed=77)
    rng = random.Random(5)
    marble = common.pbr_material("Surround marble", tex_dir, "marble_01", tint=(0.42, 0.4, 0.38), roughness_scale=0.4,
                                 size=512)
    mantel_wood = common.pbr_material("Mantel wood", tex_dir, "dark_wood", tint=(0.55, 0.35, 0.28),
                                      roughness_scale=0.5, size=512)
    soot = common.pbr_material("Firebox brick", tex_dir, "brick_wall_006", tint=(0.12, 0.1, 0.1), size=256)
    iron = common.plain_material("Cast iron", (0.04, 0.04, 0.045), 0.7, 0.55)
    bark = common.pbr_material("Log bark", tex_dir, "dark_wood", tint=(0.3, 0.22, 0.17), size=256)
    log_end = common.plain_material("Log end", (0.55, 0.38, 0.22), 0.0, 0.8)
    ember = common.plain_material("Embers", (1.0, 0.35, 0.05), 0.0, 0.6, emission=(1.0, 0.28, 0.04), strength=6.0)
    oxblood = common.pbr_material("Chesterfield leather", tex_dir, "leather_red_02", tint=(0.42, 0.13, 0.08),
                                  roughness_scale=0.55, size=512)
    velvet = common.pbr_material("Wingback velvet", tex_dir, "velour_velvet", tint=(0.1, 0.25, 0.16), size=512)
    shelf_wood = common.pbr_material("Shelf wood", tex_dir, "wood_table_001", tint=(0.5, 0.33, 0.24), size=512)
    brass = P.mat("brass")

    def box(name, lo, hi, mat, bevel=0.006, seg=2, ax="x"):
        o = common.box(name, lo, hi, mat, bevel, seg)
        o["grain_axis"] = ax
        return o

    # ---- Fireplace ----
    z0, z1 = FZ - OPEN_W / 2, FZ + OPEN_W / 2
    box("Firebox back", (FX - 0.42, 0.0, z0 - 0.02), (FX - 0.38, OPEN_H, z1 + 0.02), soot, 0, ax="z")
    for zz in (z0, z1):  # angled cheeks
        box("Firebox side", (FX - 0.4, 0.0, zz - 0.02), (FX, OPEN_H, zz + 0.02), soot, 0, ax="z")
    box("Firebox top", (FX - 0.4, OPEN_H - 0.02, z0), (FX, OPEN_H + 0.02, z1), soot, 0, ax="z")
    box("Firebox floor", (FX - 0.4, 0.0, z0), (FX, 0.02, z1), soot, 0, ax="z")
    # Marble surround and the mantel shelf with corbels.
    SW = 0.24
    box("Surround left", (FX, 0.0, z0 - SW), (FX + 0.05, OPEN_H + SW, z0), marble, 0.006, ax="y")
    box("Surround right", (FX, 0.0, z1), (FX + 0.05, OPEN_H + SW, z1 + SW), marble, 0.006, ax="y")
    box("Surround top", (FX, OPEN_H, z0), (FX + 0.05, OPEN_H + SW, z1), marble, 0.006, ax="z")
    box("Mantel frieze", (FX, OPEN_H + SW, z0 - SW - 0.06), (FX + 0.09, OPEN_H + SW + 0.16, z1 + SW + 0.06),
        mantel_wood, 0.01, 3, ax="z")
    box("Mantel shelf", (FX, OPEN_H + SW + 0.16, z0 - SW - 0.14), (FX + 0.26, OPEN_H + SW + 0.21, z1 + SW + 0.14),
        mantel_wood, 0.015, 3, ax="z")
    for zz in (z0 - SW - 0.03, z1 + SW + 0.03):
        box("Pilaster", (FX, 0.0, zz - 0.05), (FX + 0.09, OPEN_H + SW, zz + 0.05), mantel_wood, 0.01, 3, ax="y")
        box("Corbel", (FX + 0.09, OPEN_H + SW + 0.02, zz - 0.045), (FX + 0.2, OPEN_H + SW + 0.16, zz + 0.045),
            mantel_wood, 0.02, 3)
    box("Hearth", (FX, 0.0, z0 - SW - 0.15), (FX + 0.5, 0.04, z1 + SW + 0.15), marble, 0.01, 3, ax="z")
    # Grate, logs, embers.
    for k in range(6):
        zz = z0 + 0.12 + k * (OPEN_W - 0.24) / 5
        box("Grate bar", (FX - 0.32, 0.08, zz - 0.008), (FX - 0.05, 0.1, zz + 0.008), iron, 0.003)
    for zz in (z0 + 0.12, z1 - 0.12):
        box("Grate leg", (FX - 0.08, 0.0, zz - 0.012), (FX - 0.05, 0.16, zz + 0.012), iron, 0.003)
    box("Grate front", (FX - 0.07, 0.1, z0 + 0.1), (FX - 0.05, 0.16, z1 - 0.1), iron, 0.003, ax="z")
    for (a, y, zz, ln) in ((0.25, 0.14, FZ - 0.08, 0.62), (-0.2, 0.15, FZ + 0.1, 0.58), (0.05, 0.24, FZ, 0.55)):
        lg = common.cylinder("Log", Vector((0, 0, 0)), 0.055, ln, "z", bark, segments=12)
        lg.data.transform(Matrix.Translation((FX - 0.2, y, zz)) @ Matrix.Rotation(a, 4, "Y"))
        for e in (-1, 1):
            cap = common.cylinder("Log end", Vector((0, 0, e * ln / 2)), 0.05, 0.006, "z", log_end, segments=12)
            cap.data.transform(Matrix.Translation((FX - 0.2, y, zz)) @ Matrix.Rotation(a, 4, "Y"))
    for k in range(40):
        p = Vector((FX - 0.32 + rng.uniform(0, 0.26), 0.1 + rng.uniform(0, 0.03), z0 + 0.1 + rng.uniform(0, OPEN_W - 0.2)))
        common.sphere("Ember", p, rng.uniform(0.012, 0.025), ember, 6, 4)
    # Painting above the mantel.
    P.frame(FX, 2.35, FZ, 1.1, 0.75, 0, yaw=math.pi / 2, depth=0.05, border=0.07, mat=brass)
    # Mantel clutter: clock, candlesticks, photos, books, a vase.
    my = OPEN_H + SW + 0.21
    clock_px = np.ones((64, 64, 3), np.float32) * np.array(common.srgb((0.92, 0.88, 0.78)))
    yy, xx = np.mgrid[0:64, 0:64]
    d = np.sqrt((xx - 32) ** 2 + (yy - 32) ** 2)
    clock_px[(d > 29) | ((d > 25) & (d < 27))] = 0.08
    for h in range(12):
        a = h * math.pi / 6
        clock_px[int(32 + 22 * math.sin(a)) - 1:int(32 + 22 * math.sin(a)) + 2,
                 int(32 + 22 * math.cos(a)) - 1:int(32 + 22 * math.cos(a)) + 2] = 0.05
    for (a, ln) in ((math.radians(60), 14), (math.radians(150), 20)):
        for t in range(ln):
            clock_px[int(32 + t * math.sin(a)), int(32 + t * math.cos(a))] = 0.05
    clock_mat = common.image_material("Clock face", clock_px, roughness=0.3)
    cz = FZ
    common.lathe("Clock case", [(0, 0), (0.12, 0), (0.12, 0.02), (0.1, 0.03), (0.1, 0.18), (0.06, 0.26), (0, 0.27)],
                 mantel_wood, 24).data.transform(Matrix.Translation((FX + 0.12, my, cz)) @
                                                 Matrix.Diagonal((0.45, 1, 1, 1)))
    common.panel("Clock face", (FX + 0.17, my + 0.13, cz), (0, 0, -1), (0, 1, 0), 0.14, 0.14, clock_mat)
    for zz in (z0 - 0.1, z1 + 0.1):
        common.lathe("Candlestick", [(0, 0), (0.05, 0), (0.05, 0.015), (0.015, 0.04), (0.012, 0.2), (0.03, 0.22),
                                     (0, 0.22)], brass, 16).data.transform(Matrix.Translation((FX + 0.12, my, zz)))
        P.candle(FX + 0.12, my + 0.22, zz, h=0.16, r=0.012)
    P.frame(FX + 0.06, my + 0.12, z1 + 0.33, 0.14, 0.18, 1, yaw=math.pi / 2 + 0.3, depth=0.02, border=0.02)
    P.frame(FX + 0.06, my + 0.1, z0 - 0.35, 0.18, 0.14, 7, yaw=math.pi / 2 - 0.2, depth=0.02, border=0.02)
    for k in range(3):
        P.book(FX + 0.13, my + k * 0.03, z0 - 0.62, 0.03, 0.22, 0.15, yaw=math.pi / 2, lying=True)
    common.lathe("Vase", [(0, 0), (0.04, 0), (0.07, 0.08), (0.05, 0.2), (0.03, 0.24), (0.04, 0.27), (0.0, 0.27)],
                 common.plain_material("Vase glaze", (0.08, 0.2, 0.3), 0.0, 0.15), 18).data.transform(
        Matrix.Translation((FX + 0.13, my, z1 + 0.65)))
    # Fire tools on a stand, and a log basket.
    tx, tz = FX + 0.35, z1 + SW + 0.35
    common.cylinder("Tool stand", Vector((tx, 0.4, tz)), 0.01, 0.8, "y", iron, segments=8)
    common.cylinder("Tool base", Vector((tx, 0.01, tz)), 0.11, 0.02, "y", iron, segments=16)
    common.torus("Tool ring", Vector((tx, 0.78, tz)), 0.05, 0.006, iron, segments=16, sides=4)
    for k in range(4):
        a = k * math.pi / 2
        common.tube("Fire tool", [(tx + 0.05 * math.cos(a), 0.82, tz + 0.05 * math.sin(a)),
                                  (tx + 0.06 * math.cos(a), 0.1, tz + 0.06 * math.sin(a))], 0.006, iron, sides=5)
        common.sphere("Tool handle", Vector((tx + 0.05 * math.cos(a), 0.84, tz + 0.05 * math.sin(a))), 0.015, brass,
                      8, 6)
    bx, bz = FX + 0.35, z0 - SW - 0.4
    common.lathe("Log basket", [(0, 0), (0.2, 0), (0.24, 0.3), (0.22, 0.3), (0.18, 0.02), (0, 0.02)], iron, 16
                 ).data.transform(Matrix.Translation((bx, 0, bz)))
    for k in range(5):
        lg = common.cylinder("Basket log", Vector((0, 0, 0)), 0.045, 0.42, "x", bark, segments=10)
        lg.data.transform(Matrix.Translation((bx, 0.12 + (k // 3) * 0.09, bz - 0.09 + (k % 3) * 0.09)) @
                          Matrix.Rotation(0.3 * k, 4, "Y"))

    # ---- Rug (a woven pattern drawn here) ----
    RW, RD = 3.0, 3.6
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
    # One slab (not a picture on top of a slab: those flicker against each
    # other), ending in front of the sofa.
    RW, RD = 1.75, 3.3
    RX, RZ = -7.12, FZ
    rug_obj = common.box("Rug", (RX - RW / 2, 0.0, RZ - RD / 2), (RX + RW / 2, 0.012, RZ + RD / 2), rug, 0.004)
    common.set_uvs(rug_obj, lambda co, n: ((co.x - (RX - RW / 2)) / RW, (co.z - (RZ - RD / 2)) / RD))

    # ---- Chesterfield sofa (faces the fire, -x) ----
    SX, SZ, SL = -5.75, FZ, 2.1
    sofa = Matrix.Translation((SX, 0, SZ))
    def sb(name, lo, hi, mat, bev=0.03, seg=3):
        o = common.box(name, lo, hi, mat, bev, seg)
        o.data.transform(sofa)
        o["grain_axis"] = "z"
        return o
    sb("Sofa base", (-0.45, 0.12, -SL / 2), (0.45, 0.42, SL / 2), oxblood)
    sb("Sofa back", (0.25, 0.12, -SL / 2), (0.45, 0.76, SL / 2), oxblood, 0.05, 4)
    for side in (-1, 1):
        sb("Sofa arm", (-0.45, 0.12, side * SL / 2 - 0.12), (0.45, 0.7, side * SL / 2 + 0.12), oxblood, 0.05, 4)
        arm = common.cylinder("Rolled arm", Vector((0, 0, 0)), 0.1, 0.92, "x", oxblood, segments=16)
        arm.data.transform(sofa @ Matrix.Translation((0.0, 0.72, side * (SL / 2 + 0.03))))
    roll = common.cylinder("Back roll", Vector((0, 0, 0)), 0.1, SL + 0.2, "z", oxblood, segments=16)
    roll.data.transform(sofa @ Matrix.Translation((0.36, 0.76, 0)))
    for k in range(3):
        zc = -SL / 2 + 0.2 + (k + 0.5) * (SL - 0.4) / 3
        sb("Seat cushion", (-0.43, 0.42, zc - (SL - 0.4) / 6 + 0.01), (0.25, 0.53, zc + (SL - 0.4) / 6 - 0.01), oxblood,
           0.05, 4)
    for i in range(10):  # deep button tufting on the back
        for j in range(3):
            z = -SL / 2 + 0.25 + i * (SL - 0.5) / 9 + (0.5 * (SL - 0.5) / 9 if j % 2 else 0)
            if z > SL / 2 - 0.2:
                continue
            b = common.sphere("Tuft", Vector((0.245, 0.58 + j * 0.06, z)), 0.012, oxblood, 8, 4)
            b.data.transform(sofa)
    for k in range(36):  # brass nailheads along the front
        z = -SL / 2 - 0.1 + k * (SL + 0.2) / 35
        n = common.sphere("Nailhead", Vector((-0.452, 0.15, z)), 0.007, brass, 6, 4)
        n.data.transform(sofa)
    for (x, z) in ((-0.38, -SL / 2 - 0.05), (-0.38, SL / 2 + 0.05), (0.38, -SL / 2 - 0.05), (0.38, SL / 2 + 0.05)):
        lg = common.lathe("Sofa leg", [(0, 0), (0.03, 0), (0.04, 0.06), (0.03, 0.12), (0, 0.12)], mantel_wood, 12)
        lg.data.transform(sofa @ Matrix.Translation((x, 0, z)))
    # Throw pillows.
    for (z, a) in ((-0.6, 0.2), (0.62, -0.25)):
        p = common.obox("Pillow", (0.0, 0.0, 0.0), (0.12, 0.36, 0.38), a, velvet, 0.06, 4)
        p.data.transform(sofa @ Matrix.Translation((0.15, 0.68, z)) @ Matrix.Rotation(-0.25, 4, "Z"))

    # ---- Wingback armchairs (angled toward the fire) ----
    def wingback(x, z, yaw):
        m = Matrix.Translation((x, 0, z)) @ Matrix.Rotation(yaw, 4, "Y")
        def wb(name, lo, hi, bev=0.03):
            o = common.box(name, lo, hi, velvet, bev, 3)
            o.data.transform(m)
        wb("Chair seat", (-0.38, 0.2, -0.36), (0.38, 0.44, 0.38))
        wb("Chair cushion", (-0.33, 0.44, -0.34), (0.33, 0.52, 0.3), 0.04)
        wb("Chair back", (-0.38, 0.2, 0.3), (0.38, 1.15, 0.42), 0.04)
        for side in (-1, 1):
            wb("Chair arm", (side * 0.38 - 0.06, 0.2, -0.36), (side * 0.38 + 0.06, 0.62, 0.38))
            arm = common.cylinder("Arm roll", Vector((side * 0.4, 0.64, 0.0)), 0.06, 0.72, "z", velvet, segments=12)
            arm.data.transform(m)
            wing = common.box("Wing", (-0.05, 0.6, 0.0), (0.05, 1.1, 0.3), velvet, 0.04, 3)
            wing.data.transform(m @ Matrix.Translation((side * 0.38, 0, 0.12)) @ Matrix.Rotation(side * 0.35, 4, "Y"))
        for (lx, lz) in ((-0.32, -0.3), (0.32, -0.3), (-0.32, 0.36), (0.32, 0.36)):
            leg = common.tube("Chair leg", [(lx, 0.2, lz), (lx * 1.08, 0.1, lz * 1.1), (lx * 1.02, 0.0, lz * 1.02)],
                              0.022, mantel_wood, sides=8)
            leg.data.transform(m)
        for k in range(14):
            n = common.sphere("Nailhead", Vector((-0.32 + k * 0.05, 0.22, -0.365)), 0.006, brass, 6, 4)
            n.data.transform(m)

    wingback(-7.25, FZ - 1.3, math.pi - 0.9)
    wingback(-7.25, FZ + 1.3, 0.9)

    # ---- Coffee table with a chess game, drinks, books ----
    TX, TZ, TY = -6.95, FZ, 0.45
    box("Table top", (TX - 0.38, TY - 0.04, TZ - 0.6), (TX + 0.38, TY, TZ + 0.6), mantel_wood, 0.012, 3, ax="z")
    box("Table apron", (TX - 0.34, TY - 0.12, TZ - 0.56), (TX + 0.34, TY - 0.04, TZ + 0.56), mantel_wood, 0.006, ax="z")
    box("Table shelf", (TX - 0.32, 0.1, TZ - 0.54), (TX + 0.32, 0.13, TZ + 0.54), mantel_wood, 0.006, ax="z")
    for (dx, dz) in ((-0.33, -0.55), (0.33, -0.55), (-0.33, 0.55), (0.33, 0.55)):
        box("Table leg", (TX + dx - 0.03, 0.0, TZ + dz - 0.03), (TX + dx + 0.03, TY - 0.04, TZ + dz + 0.03),
            mantel_wood, 0.008, ax="y")
    # Chess board: 8x8, pieces in an opening position (a few moved).
    cb_px = np.zeros((128, 128, 3), np.float32)
    for i in range(8):
        for j in range(8):
            cb_px[i * 16:(i + 1) * 16, j * 16:(j + 1) * 16] = common.srgb((0.85, 0.75, 0.55)) if (i + j) % 2 else \
                common.srgb((0.3, 0.18, 0.1))
    chess = common.image_material("Chess board", cb_px, roughness=0.3)
    # The board is played on (bar/projects/chess/chess.cpp moves the pieces, models from
    # tools/assets/chess.py): 0.44 m, centred on the table, rank 1 toward +z
    # (the armchair at +z plays white). Keep in step with kBoard* there.
    CB, BZC = 0.44, TZ
    box("Board base", (TX - CB / 2 - 0.025, TY, BZC - CB / 2 - 0.025), (TX + CB / 2 + 0.025, TY + 0.02, BZC + CB / 2 + 0.025),
        mantel_wood, 0.004)
    common.quad("Chess squares", [(TX - CB / 2, TY + 0.0205, BZC + CB / 2), (TX + CB / 2, TY + 0.0205, BZC + CB / 2),
                                  (TX + CB / 2, TY + 0.0205, BZC - CB / 2), (TX - CB / 2, TY + 0.0205, BZC - CB / 2)],
                chess, uvs=((0, 0), (1, 0), (1, 1), (0, 1)))
    P.glass(TX + 0.2, TY, TZ + 0.42, "rocks", "whiskey", ice=True)
    P.glass(TX - 0.2, TY, TZ - 0.42, "snifter", "whiskey")
    P.bottle(TX + 0.02, TY, TZ + 0.48, "square", color=0)
    for k in range(2):
        P.book(TX - 0.15, TY + k * 0.035, TZ + 0.45, 0.035, 0.2, 0.14, yaw=0.2 * k, lying=True)
    P.candle(TX + 0.2, TY, TZ - 0.5, h=0.1, r=0.03)

    # ---- Bookshelf on the left wall between the chimney and the stage ----
    BZ0, BZ1, BH, BD = L.CHIMNEY_Z1 + 0.12, L.STAGE_Z0 - L.STEP_W - 0.12, 2.3, 0.34
    X = L.X0
    box("Shelf side", (X, 0, BZ0), (X + BD, BH, BZ0 + 0.03), shelf_wood, 0.004, ax="y")
    box("Shelf side", (X, 0, BZ1 - 0.03), (X + BD, BH, BZ1), shelf_wood, 0.004, ax="y")
    box("Shelf back", (X, 0, BZ0), (X + 0.012, BH, BZ1), shelf_wood, 0.0, ax="z")
    box("Shelf crown", (X, BH, BZ0 - 0.03), (X + BD + 0.03, BH + 0.06, BZ1 + 0.03), shelf_wood, 0.01, 3, ax="z")
    box("Shelf plinth", (X, 0, BZ0), (X + BD, 0.1, BZ1), shelf_wood, 0.004, ax="z")
    for k, y in enumerate((0.1, 0.52, 0.94, 1.36, 1.78)):
        box("Shelf board", (X, y, BZ0 + 0.03), (X + BD, y + 0.025, BZ1 - 0.03), shelf_wood, 0.004, ax="z")
        # Books along the shelf (spines facing +x), with a bookend or object now and then.
        length = BZ1 - BZ0 - 0.1
        P.book_row(X + BD / 2 + 0.02, y + 0.025, BZ0 + 0.05, length * (0.75 if k % 2 else 0.95), depth=0.2,
                   yaw=-math.pi / 2, max_h=0.36)
        if k % 2:
            P.plant(X + BD / 2, y + 0.025, BZ1 - 0.15, size=0.3, pot_r=0.06, pot_h=0.1, leaves=8, seed=k)
    P.plant(X + BD / 2, BH + 0.06, (BZ0 + BZ1) / 2, size=0.6, leaves=10, seed=9, hanging=True)

    # ---- Floor lamp and side table by the second armchair ----
    LX, LZ = -6.3, FZ + 1.95
    common.cylinder("Lamp base", Vector((LX, 0.015, LZ)), 0.16, 0.03, "y", brass, segments=20)
    common.cylinder("Lamp pole", Vector((LX, 0.8, LZ)), 0.012, 1.6, "y", brass, segments=8)
    shade = common.lathe("Lamp shade", [(0.12, 1.45), (0.22, 1.45), (0.14, 1.72), (0.12, 1.72), (0.2, 1.46)],
                         common.plain_material("Shade fabric", (0.85, 0.7, 0.5), 0.0, 0.9, emission=(1.0, 0.7, 0.4),
                                               strength=0.8), 24)
    shade.data.transform(Matrix.Translation((LX, 0, LZ)))
    common.sphere("Lamp bulb", Vector((LX, 1.55, LZ)), 0.04,
                  common.plain_material("Floor lamp bulb", (1, 0.9, 0.7), 0.0, 0.3, emission=(1, 0.75, 0.45),
                                        strength=8.0), 10, 6)
    common.cylinder("Side table", Vector((LX + 0.45, 0.55, LZ - 0.1)), 0.24, 0.03, "y", mantel_wood, segments=24)
    common.cylinder("Side table post", Vector((LX + 0.45, 0.28, LZ - 0.1)), 0.03, 0.54, "y", mantel_wood, segments=10)
    common.cylinder("Side table foot", Vector((LX + 0.45, 0.015, LZ - 0.1)), 0.18, 0.03, "y", mantel_wood, segments=18)
    P.glass(LX + 0.4, 0.565, LZ - 0.05, "wine", "red_wine")
    P.book(LX + 0.52, 0.565, LZ - 0.15, 0.03, 0.2, 0.14, yaw=0.5, lying=True)

    common.finish_and_export(os.path.join(out_dir, "lounge.glb"), uv_scale={
        "Surround marble": 1.5, "Mantel wood": 1.5, "Firebox brick": 2.0, "Log bark": 4.0,
        "Chesterfield leather": 2.0, "Wingback velvet": 2.0, "Shelf wood": 1.2})


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
