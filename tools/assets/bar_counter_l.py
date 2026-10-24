# Builds the new bar counter (docs/archive/BAR_REDESIGN.md): it bows into the room
# along a big curve and turns back to the back wall. Customer side: plinth,
# paneled front with pilasters, apron molding, a thick mahogany top, a
# padded leather arm rest and a brass foot rail on cast brackets. Bartender
# side: the stainless underbar (fridges with glass doors, ice wells with
# speed rails, a three-basin sink, drainboard and glass rack, a glass
# washer, a kegerator, a trash bin). On top: an 8-tap beer tower with drip
# tray, the POS screen and receipt printer, a tip jar and bell, a cocktail
# station (shakers, jiggers, strainers, garnish tray, bitters, straws) and
# customers' clutter (drinks, peanut bowls, a phone, keys, candles).
#
#   blender -b --factory-startup --python tools/assets/bar_counter_l.py -- <texture dir> <out.glb>
#
# Model space = world space (placed at the origin).
import math
import os
import random
import sys

import bmesh
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402
import layout as L  # noqa: E402
import props_lib  # noqa: E402

tex_dir, out_path = sys.argv[sys.argv.index("--") + 1:][:2]
common.reset_scene()
P = props_lib.Props(tex_dir, seed=21)
rng = random.Random(4)

TOP_Y = 1.08         # bar top surface (C++ kBarTopY)
BODY_S = 0.27        # half depth of the body
UNDER_Y = 0.86       # underbar work surface
UNDER_S = (-0.29, -0.80)

mahogany = common.pbr_material("Bar top mahogany", tex_dir, "wood_table_001", tint=(0.62, 0.30, 0.20),
                               roughness_scale=0.35, roughness_bias=0.02, grain="v")
body_wood = common.pbr_material("Counter mahogany", tex_dir, "dark_wood", tint=(0.55, 0.33, 0.26),
                                roughness_scale=0.6, size=512)
leather = common.pbr_material("Arm rest leather", tex_dir, "leather_red_02", tint=(0.32, 0.12, 0.08),
                              roughness_scale=0.7, size=512)
brass = P.mat("brass")
steel = P.mat("steel")
plinth = common.plain_material("Plinth", (0.02, 0.018, 0.016), 0.0, 0.5)
dark_steel = common.plain_material("Dark steel", (0.12, 0.12, 0.13), 1.0, 0.35)
fridge_light = common.plain_material("Fridge light", (0.9, 0.95, 1.0), 0.0, 0.4, emission=(0.75, 0.85, 1.0),
                                     strength=1.6)
ice_bed = common.plain_material("Ice bed", (0.82, 0.88, 0.92), 0.0, 0.18)
basin = common.plain_material("Basin", (0.25, 0.26, 0.27), 1.0, 0.3)
rack_plastic = common.plain_material("Glass rack", (0.05, 0.05, 0.06), 0.0, 0.55)
sink_water = common.glass_material("Sink water", color=(0.6, 0.7, 0.75), alpha=0.35)

# ---- The path ----
path = L.bar_centerline(0.04)
frames = common.path_frames(path)
lengths = [0.0]
for a, b in zip(path, path[1:]):
    lengths.append(lengths[-1] + math.dist(a, b))
TOTAL = lengths[-1]


def at(u):
    """(position, tangent, normal) at arc length u along the centerline."""
    u = min(max(u, 0.0), TOTAL)
    i = max(0, min(len(path) - 2, next((k for k in range(len(lengths)) if lengths[k] >= u), len(lengths) - 1) - 1))
    seg = lengths[i + 1] - lengths[i]
    f = (u - lengths[i]) / seg if seg > 0 else 0.0
    p = (path[i][0] + (path[i + 1][0] - path[i][0]) * f, path[i][1] + (path[i + 1][1] - path[i][1]) * f)
    t = (path[i + 1][0] - path[i][0], path[i + 1][1] - path[i][1])
    ln = math.hypot(*t)
    t = (t[0] / ln, t[1] / ln)
    return p, t, (-t[1], t[0])


def world(u, s, y=0.0):
    p, t, n = at(u)
    return (p[0] + n[0] * s, y, p[1] + n[1] * s)


def yaw_at(u):
    """obox yaw so that local x runs along the counter and local z toward
    the customers."""
    _, t, _ = at(u)
    return -math.atan2(t[1], t[0])


def facing(u, toward_customers):
    """props_lib yaw so a prop's front (+z) faces the customers or the
    bartender."""
    _, t, n = at(u)
    d = n if toward_customers else (-n[0], -n[1])
    return math.atan2(d[0], d[1])


def lbox(name, u, s, y, size, mat, bevel=0.004, segments=2, yaw_extra=0.0, grain="x"):
    """A box in the counter's local frame: centered at arc length u, offset
    s toward the customers, height y; size = (along, up, across)."""
    o = common.obox(name, world(u, s, y), size, yaw_at(u) + yaw_extra, mat, bevel, segments)
    o["grain_axis"] = grain
    return o


# Where the curve ends and the turn begins (the turn is too tight for flat
# panels).
U_CURVE_END = next(lengths[k] for k in range(1, len(path) - 1)
                   if abs(frames[k][1][0] - frames[k - 1][1][0]) + abs(frames[k][1][1] - frames[k - 1][1][1]) > 0.02)
U_RETURN = U_CURVE_END + L.BAR_TURN_R * 1.3

# ---- Swept body ----
common.sweep("Counter body", path, [(-BODY_S, 0.12), (0.2, 0.12), (BODY_S, 0.16), (BODY_S, 1.02),
                                    (-BODY_S, 1.02)], body_wood)["grain_axis"] = "y"
common.sweep("Plinth", path, [(-BODY_S, 0.0), (0.255, 0.0), (0.255, 0.12), (-BODY_S, 0.12)], plinth)
common.sweep("Base molding", path, [(0.2, 0.12), (0.29, 0.12), (0.295, 0.14), (0.28, 0.17), (0.2, 0.17)], body_wood)
common.sweep("Apron", path, [(0.25, 0.9), (0.285, 0.91), (0.31, 0.95), (0.325, 0.99), (0.325, 1.02), (0.25, 1.02)],
             body_wood)
common.sweep("Bar top", path, [(-0.42, 1.02), (0.34, 1.02), (0.34, TOP_Y), (-0.40, TOP_Y), (-0.42, TOP_Y - 0.015)],
             mahogany)["grain_axis"] = "x"
roll = [(0.37 + 0.058 * math.cos(a), TOP_Y + 0.032 + 0.042 * math.sin(a))
        for a in [i * 2 * math.pi / 16 for i in range(16)]]
common.sweep("Arm rest", path, roll, leather, smooth=True)
common.sweep("Arm rest welt", path, [(0.31, TOP_Y - 0.003), (0.43, TOP_Y - 0.003), (0.43, TOP_Y + 0.004),
                                     (0.31, TOP_Y + 0.004)], dark_steel)
# Brass foot rail on brackets, with end caps.
rail = [(p[0] + n[0] * 0.46, 0.2, p[1] + n[1] * 0.46) for (p, t, n) in common.path_frames(common.resample(path, 0.1))]
rail = [r for r in rail if r[2] > L.Z0 + 0.25]
common.tube("Foot rail", rail, 0.026, brass, sides=14)
for end in (rail[0], rail[-1]):
    common.sphere("Rail cap", Vector(end), 0.03, brass, 14, 8)
u = 0.4
while u < TOTAL - 0.4:
    if world(u, 0.46)[2] > L.Z0 + 0.3:
        lbox("Rail bracket", u, 0.36, 0.2, (0.03, 0.035, 0.2), brass, 0.008)
        lbox("Rail bracket foot", u, 0.28, 0.2, (0.08, 0.09, 0.02), brass, 0.006)
        p = world(u, 0.46, 0.2)
        ring = common.torus("Rail ring", Vector((0, 0, 0)), 0.03, 0.007, brass, axis="x", segments=16, sides=6)
        ring.data.transform(Matrix.Translation(p) @ Matrix.Rotation(yaw_at(u), 4, "Y"))
    u += 0.95

# Pilasters and raised panels on the customer side (along the curve and the
# straight return).
spans = [(0.15, U_CURVE_END - 0.1), (U_RETURN, TOTAL - 0.35)]
for (u0, u1) in spans:
    n = max(1, round((u1 - u0) / 0.9))
    step = (u1 - u0) / n
    for k in range(n + 1):
        uu = u0 + k * step
        lbox("Pilaster", uu, BODY_S + 0.012, 0.56, (0.09, 0.74, 0.03), body_wood, 0.008, 3, grain="y")
        lbox("Pilaster cap", uu, BODY_S + 0.022, 0.89, (0.12, 0.05, 0.04), body_wood, 0.01, 3)
        lbox("Pilaster base", uu, BODY_S + 0.022, 0.21, (0.12, 0.06, 0.04), body_wood, 0.01, 3)
    for k in range(n):
        uu = u0 + (k + 0.5) * step
        w = step - 0.16
        lbox("Panel frame", uu, BODY_S + 0.006, 0.56, (w, 0.6, 0.014), body_wood, 0.004, grain="y")
        lbox("Panel field", uu, BODY_S + 0.012, 0.56, (w - 0.12, 0.48, 0.02), body_wood, 0.018, 3, grain="y")
        lbox("Panel bead", uu, BODY_S + 0.01, 0.56, (w - 0.07, 0.53, 0.012), plinth, 0.003)
# A rounded end post at the left end.
p0, t0, n0 = at(0.0)
common.cylinder("End post", Vector((p0[0] - t0[0] * 0.02, 0.56, p0[1] - t0[1] * 0.02)), 0.29, 0.92, "y", body_wood,
                segments=28)
common.cylinder("End top", Vector((p0[0] - t0[0] * 0.02, TOP_Y - 0.03, p0[1] - t0[1] * 0.02)), 0.37, 0.06, "y",
                mahogany, segments=32)

# ---- Underbar (bartender side) ----
S_MID = (UNDER_S[0] + UNDER_S[1]) / 2
DEPTH = UNDER_S[0] - UNDER_S[1]


def unit_shell(u, w, h=UNDER_Y):
    lbox("Underbar", u, S_MID, h / 2 + 0.05, (w - 0.01, h - 0.1, DEPTH), steel, 0.004)
    lbox("Underbar toe", u, S_MID + 0.02, 0.05, (w - 0.01, 0.1, DEPTH - 0.06), plinth, 0.002)
    lbox("Underbar top", u, S_MID - 0.005, h - 0.01, (w, 0.02, DEPTH + 0.02), steel, 0.003)


def fridge(u, w, kind="beer"):
    """Under-counter fridge with two glass doors, lit, full of bottles."""
    unit_shell(u, w)
    lbox("Fridge light", u, UNDER_S[0] - 0.04, 0.48, (w - 0.08, 0.6, 0.01), fridge_light, 0.0)
    for sy in (0.2, 0.48):
        lbox("Fridge shelf", u, S_MID, sy, (w - 0.08, 0.01, DEPTH - 0.1), dark_steel, 0.0)
        k = -w / 2 + 0.08
        while k < w / 2 - 0.06:
            uu = u + k
            pos = world(uu, S_MID - 0.08, sy + 0.005)
            P.bottle(pos[0], pos[1], pos[2], "beer" if kind == "beer" else "wine" if sy < 0.3 else "champagne",
                     color=rng.choice([0, 1, 9, 2]) if kind == "beer" else rng.choice([3, 1, 8]),
                     yaw=facing(uu, False), segments=8)
            k += 0.075 if kind == "beer" else 0.09
    for side in (-1, 1):
        lbox("Fridge door frame", u + side * w / 4, UNDER_S[1] - 0.012, 0.5, (w / 2 - 0.02, 0.66, 0.02), dark_steel,
             0.004)
        p, t, nn = at(u + side * w / 4)
        c = world(u + side * w / 4, UNDER_S[1] - 0.025, 0.5)
        common.panel("Fridge glass", c, (-t[0], 0, -t[1]), (0, 1, 0), w / 2 - 0.08, 0.56, P.mat("glass"))
        lbox("Fridge handle", u + side * (w / 2 - 0.08), UNDER_S[1] - 0.05, 0.6, (0.02, 0.3, 0.02), steel, 0.006)


def ice_well(u, w):
    unit_shell(u, w, UNDER_Y)
    lbox("Ice well rim", u, S_MID, UNDER_Y + 0.005, (w - 0.04, 0.012, DEPTH - 0.04), steel, 0.002)
    # Lumpy ice surface.
    bm = bmesh.new()
    nu, nv = 14, 8
    grid = []
    for i in range(nu + 1):
        row = []
        for j in range(nv + 1):
            uu = u - (w - 0.1) / 2 + (w - 0.1) * i / nu
            ss = UNDER_S[0] - 0.05 - (DEPTH - 0.1) * j / nv
            p = world(uu, ss, UNDER_Y - 0.03 + rng.uniform(0, 0.025))
            row.append(bm.verts.new(p))
        grid.append(row)
    for i in range(nu):
        for j in range(nv):
            bm.faces.new([grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    common.mesh_object("Ice", bm, ice_bed, smooth=True)
    # Speed rail on the front, full of bottles facing the bartender.
    lbox("Speed rail", u, UNDER_S[1] - 0.06, 0.48, (w - 0.04, 0.02, 0.1), steel, 0.004)
    lbox("Speed rail lip", u, UNDER_S[1] - 0.11, 0.53, (w - 0.04, 0.08, 0.01), steel, 0.003)
    k = -w / 2 + 0.07
    while k < w / 2 - 0.05:
        pos = world(u + k, UNDER_S[1] - 0.06, 0.49)
        P.bottle(pos[0], pos[1], pos[2], rng.choice(["slim", "whiskey", "tequila", "stubby"]), yaw=facing(u + k, False))
        k += 0.095
    # An ice scoop.
    pos = world(u + 0.15, S_MID, UNDER_Y + 0.01)
    common.obox("Ice scoop", pos, (0.18, 0.05, 0.08), yaw_at(u) + 0.4, steel, 0.02)


def sink(u, w):
    unit_shell(u, w)
    for k in range(3):
        uu = u - w / 3 + k * w / 3
        lbox("Basin", uu, S_MID, UNDER_Y - 0.07, (w / 3 - 0.06, 0.14, DEPTH - 0.1), basin, 0.01)
        if k == 1:
            lbox("Water", uu, S_MID, UNDER_Y - 0.06, (w / 3 - 0.08, 0.005, DEPTH - 0.12), sink_water, 0.0)
    # Gooseneck faucet at the back.
    base = world(u, UNDER_S[0] - 0.03, UNDER_Y)
    _, t, n = at(u)
    pts = [base, (base[0], base[1] + 0.25, base[2])]
    for a in range(1, 7):
        ang = math.pi * a / 6
        pts.append((base[0] - n[0] * 0.09 * (1 - math.cos(ang)), base[1] + 0.25 + 0.09 * math.sin(ang),
                    base[2] - n[1] * 0.09 * (1 - math.cos(ang))))
    pts.append((pts[-1][0], pts[-1][1] - 0.05, pts[-1][2]))
    common.tube("Faucet", pts, 0.012, P.mat("chrome"), sides=10)
    for side in (-1, 1):
        hp = world(u + side * 0.09, UNDER_S[0] - 0.03, UNDER_Y + 0.03)
        common.cylinder("Faucet handle", Vector(hp), 0.012, 0.06, "y", P.mat("chrome"), segments=8)


def drainboard(u, w):
    unit_shell(u, w)
    for k in range(int(w / 0.03)):
        lbox("Drain rib", u - w / 2 + 0.03 + k * 0.03, S_MID, UNDER_Y + 0.004, (0.006, 0.006, DEPTH - 0.06), steel, 0.0)
    # A glass rack with upside-down glasses.
    lbox("Rack", u, S_MID, UNDER_Y + 0.05, (0.48, 0.09, 0.48), rack_plastic, 0.01)
    for i in range(3):
        for j in range(3):
            pos = world(u - 0.15 + i * 0.15, S_MID - 0.15 + j * 0.15, UNDER_Y + 0.1)
            g = common.lathe("Rack glass", [(0, 0), (0.042, 0), (0.041, 0.078), (0.04, 0.09), (0.0, 0.09)],
                             P.mat("glass"), 14)
            g.data.transform(Matrix.Translation(pos))


def glass_washer(u, w):
    unit_shell(u, w)
    lbox("Washer door", u, UNDER_S[1] - 0.012, 0.45, (w - 0.06, 0.6, 0.02), dark_steel, 0.006)
    lbox("Washer handle", u, UNDER_S[1] - 0.04, 0.72, (w - 0.2, 0.025, 0.025), steel, 0.008)
    lbox("Washer panel", u, UNDER_S[1] - 0.025, 0.8, (0.2, 0.05, 0.01),
         common.plain_material("Washer LED", (0.1, 0.8, 0.3), 0.0, 0.4, emission=(0.1, 1.0, 0.3), strength=2.0), 0.0)


def kegerator(u, w):
    unit_shell(u, w)
    lbox("Keg door", u, UNDER_S[1] - 0.012, 0.45, (w - 0.05, 0.7, 0.02), steel, 0.006)
    lbox("Keg handle", u + w / 2 - 0.08, UNDER_S[1] - 0.04, 0.5, (0.025, 0.4, 0.025), dark_steel, 0.008)
    lbox("Keg vent", u, UNDER_S[1] - 0.012, 0.08, (w - 0.1, 0.06, 0.022), plinth, 0.002)


def trash(u, w):
    unit_shell(u, w)
    lbox("Trash opening", u, S_MID, UNDER_Y + 0.001, (0.2, 0.004, 0.14), plinth, 0.0)


stations = [(fridge, 1.15, "beer"), (ice_well, 0.85), (sink, 1.1), (drainboard, 0.6), (ice_well, 0.85),
            (glass_washer, 0.6), (fridge, 1.15, "wine"), (trash, 0.5), (kegerator, 0.9)]
u = 0.55
for st in stations:
    fn, w = st[0], st[1]
    if u + w > U_CURVE_END - 0.1:
        break
    if len(st) > 2:
        fn(u + w / 2, w, st[2])
    else:
        fn(u + w / 2, w)
    u += w + 0.01

# ---- On the bar top ----
# Beer tower: chrome T with eight faucets facing the bartender, colorful tap
# handles, a ribbed drip tray.
U_TAPS = (U_CURVE_END) * 0.52
tap_colors = [common.plain_material(f"Tap handle {i}", c, 0.0, 0.4) for i, c in enumerate(
    [(0.6, 0.08, 0.05), (0.05, 0.1, 0.35), (0.85, 0.65, 0.2), (0.08, 0.3, 0.12), (0.1, 0.08, 0.06),
     (0.85, 0.85, 0.8), (0.55, 0.3, 0.12), (0.4, 0.05, 0.25)])]
chrome = P.mat("chrome")
for side in (-0.42, 0.42):
    c = world(U_TAPS + side, -0.12, TOP_Y + 0.2)
    common.cylinder("Tower column", Vector(c), 0.035, 0.4, "y", chrome, segments=16)
    common.cylinder("Tower foot", Vector(world(U_TAPS + side, -0.12, TOP_Y + 0.008)), 0.06, 0.016, "y", chrome,
                    segments=20)
_, tt, nn = at(U_TAPS)
bar = common.cylinder("Tower bar", Vector((0, 0, 0)), 0.04, 1.0, "x", chrome, segments=18)
bar.data.transform(Matrix.Translation(world(U_TAPS, -0.12, TOP_Y + 0.42)) @ Matrix.Rotation(yaw_at(U_TAPS), 4, "Y"))
for side in (-0.5, 0.5):
    cap = common.sphere("Tower cap", Vector(world(U_TAPS + side, -0.12, TOP_Y + 0.42)), 0.042, chrome, 16, 8)
for k in range(8):
    uu = U_TAPS - 0.385 + k * 0.11
    fp = world(uu, -0.2, TOP_Y + 0.38)
    shank = common.cylinder("Faucet", Vector((0, 0, 0)), 0.013, 0.1, "z", chrome, segments=10)
    shank.data.transform(Matrix.Translation(world(uu, -0.17, TOP_Y + 0.42)) @ Matrix.Rotation(yaw_at(uu), 4, "Y"))
    common.cylinder("Spout", Vector(world(uu, -0.22, TOP_Y + 0.4)), 0.011, 0.06, "y", chrome, segments=10)
    hh = [0.2, 0.26, 0.16, 0.24, 0.22, 0.18, 0.28, 0.2][k]
    hp = world(uu, -0.2, TOP_Y + 0.44 + hh / 2)
    handle = common.lathe("Tap handle", [(0, 0), (0.012, 0), (0.016, hh * 0.3), (0.021, hh * 0.85), (0.019, hh),
                                         (0, hh)], tap_colors[k], 12)
    handle.data.transform(Matrix.Translation(world(uu, -0.2, TOP_Y + 0.445)))
    common.sphere("Handle top", Vector(world(uu, -0.2, TOP_Y + 0.445 + hh)), 0.02, chrome, 10, 6)
lbox("Drip tray", U_TAPS, -0.2, TOP_Y + 0.012, (0.98, 0.024, 0.13), steel, 0.006)
for k in range(30):
    lbox("Tray grate", U_TAPS - 0.46 + k * 0.032, -0.2, TOP_Y + 0.026, (0.008, 0.004, 0.11), steel, 0.0)

# POS terminal near the right end, facing the bartender.
U_POS = U_CURVE_END - 0.6
pos_px = np.zeros((128, 192, 3), np.float32)
pos_px[:] = common.srgb((0.08, 0.09, 0.12))
pos_px[110:128] = common.srgb((0.15, 0.35, 0.65))
common.draw_text(pos_px, "BAR3D POS", 8, 124, 2, common.srgb((1, 1, 1)))
items = ["BEER", "WINE", "SHOT", "GIN", "RUM", "SODA", "TAB", "PAY", "VOID"]
for i, word in enumerate(items):
    r, c = divmod(i, 3)
    x0, y1 = 10 + c * 60, 100 - r * 30
    col = common.srgb([(0.2, 0.6, 0.3), (0.6, 0.2, 0.3), (0.3, 0.3, 0.7)][(i + r) % 3])
    pos_px[y1 - 24:y1, x0:x0 + 52] = col
    common.draw_text(pos_px, word, x0 + 26, y1 - 8, 1, common.srgb((1, 1, 1)), center=True)
pos_screen = common.glow_image_material("POS screen", pos_px, strength=1.2)
base_c = world(U_POS, -0.25, TOP_Y)
lbox("POS base", U_POS, -0.25, TOP_Y + 0.03, (0.18, 0.06, 0.14), P.mat("black"), 0.01)
lbox("POS stand", U_POS, -0.25, TOP_Y + 0.12, (0.04, 0.16, 0.04), P.mat("black"), 0.006)
tilt = Matrix.Rotation(0.35, 4, "X")
scr = common.box("POS body", (-0.19, -0.13, -0.02), (0.19, 0.13, 0.0), P.mat("black"), 0.01)
img = common.quad("POS screen", [(0.17, -0.11, -0.021), (-0.17, -0.11, -0.021), (-0.17, 0.11, -0.021),
                                 (0.17, 0.11, -0.021)], pos_screen)
for o in (scr, img):
    o.data.transform(Matrix.Translation(world(U_POS, -0.25, TOP_Y + 0.3)) @ Matrix.Rotation(yaw_at(U_POS), 4, "Y") @
                     tilt)
# Receipt printer with a curl of paper, card reader.
lbox("Printer", U_POS + 0.32, -0.28, TOP_Y + 0.065, (0.15, 0.13, 0.19), P.mat("white"), 0.015)
lbox("Receipt", U_POS + 0.32, -0.36, TOP_Y + 0.16, (0.07, 0.12, 0.002), P.mat("paper"), 0.0)
lbox("Card reader", U_POS - 0.3, -0.22, TOP_Y + 0.02, (0.08, 0.035, 0.15), P.mat("black"), 0.01)
# Tip jar with bills, and a brass bell, on the customer side.
tj = world(U_POS + 0.15, 0.12, TOP_Y)
common.lathe("Tip jar", [(0, 0), (0.06, 0), (0.062, 0.16), (0.05, 0.18), (0.05, 0.2), (0.0, 0.2)], P.mat("glass"), 18
             ).data.transform(Matrix.Translation(tj))
bills = common.plain_material("Bills", (0.35, 0.45, 0.3), 0.0, 0.8)
for k in range(5):
    b = common.obox("Bill", (0, 0.03 + k * 0.022, 0), (0.08, 0.004, 0.035), rng.uniform(0, 3), bills, 0.0)
    b.data.transform(Matrix.Translation(tj) @ Matrix.Rotation(rng.uniform(-0.6, 0.6), 4, "X"))
bell = world(U_POS + 0.35, 0.1, TOP_Y)
common.cylinder("Bell base", Vector((bell[0], TOP_Y + 0.008, bell[2])), 0.04, 0.016, "y", P.mat("dark_wood"), 16)
common.lathe("Bell", [(0, 0.016), (0.035, 0.016), (0.03, 0.04), (0.012, 0.06), (0.0, 0.062)], brass, 16
             ).data.transform(Matrix.Translation((bell[0], TOP_Y, bell[2])))
common.cylinder("Bell knob", Vector((bell[0], TOP_Y + 0.07, bell[2])), 0.006, 0.02, "y", brass, 8)

# Cocktail station by the turn: service mat, shakers, tins, jiggers,
# strainers, a utensil cup, garnish tray, bitters, straws, napkins.
U_ST = U_CURVE_END - 1.55
P.bar_mat(*world(U_ST, -0.2, TOP_Y), 0.6, yaw=yaw_at(U_ST), width=0.16)
fy = facing(U_ST, False)
P.shaker(*world(U_ST - 0.22, -0.2, TOP_Y + 0.011), yaw=fy)
P.tin(*world(U_ST - 0.12, -0.25, TOP_Y + 0.011), yaw=fy)
P.tin(*world(U_ST - 0.04, -0.17, TOP_Y + 0.011), yaw=fy, tall=False)
P.jigger(*world(U_ST + 0.06, -0.24, TOP_Y + 0.011))
P.jigger(*world(U_ST + 0.11, -0.16, TOP_Y + 0.011))
P.strainer(*world(U_ST + 0.2, -0.2, TOP_Y + 0.011), yaw=yaw_at(U_ST) + 0.5)
P.utensil_cup(*world(U_ST + 0.42, -0.26, TOP_Y))
P.garnish_tray(*world(U_ST - 0.6, -0.27, TOP_Y), yaw=yaw_at(U_ST))
for k, kind in enumerate(["bitters", "bitters", "bitters", "slim"]):
    P.bottle(*world(U_ST + 0.6 + k * 0.07, -0.28, TOP_Y), kind, yaw=fy)
straw_cols = [common.plain_material(f"Straw {i}", c, 0.0, 0.4) for i, c in enumerate(
    [(0.9, 0.1, 0.1), (0.1, 0.3, 0.9), (0.95, 0.95, 0.95), (0.1, 0.1, 0.1)])]
cup = world(U_ST + 0.95, -0.25, TOP_Y)
common.lathe("Straw cup", [(0, 0), (0.035, 0), (0.035, 0.12), (0.032, 0.12), (0.032, 0.005), (0, 0.005)], steel, 14
             ).data.transform(Matrix.Translation(cup))
for k in range(14):
    a = k * 2.4
    r = 0.022 * (k % 3) / 2
    s0 = (cup[0] + r * math.cos(a), TOP_Y + 0.01, cup[2] + r * math.sin(a))
    lean = 0.08
    common.tube("Straw", [s0, (s0[0] + lean * math.cos(a) * 0.4, TOP_Y + 0.21, s0[2] + lean * math.sin(a) * 0.4)],
                0.003, straw_cols[k % 4], sides=5)
for k in range(10):  # napkin stack
    lbox("Napkin", U_ST + 1.2, -0.22, TOP_Y + 0.002 + k * 0.003, (0.12, 0.003, 0.12), P.mat("paper"), 0.0,
         yaw_extra=rng.uniform(-0.08, 0.08))

# Customers' clutter: drinks, peanut bowls, a phone, keys, candles. (The
# pick-up glasses are separate models placed by the engine.)
drinks = [(0.9, "coupe", "pink"), (1.5, "pint", "beer"), (2.6, "wine", "red_wine"), (3.3, "martini", "blue"),
          (4.75, "highball", "cola"), (5.6, "rocks", "whiskey"), (6.6, "pint", "beer"), (7.2, "coupe", "orange")]
for (uu, kind, drink) in drinks:
    if uu > U_CURVE_END - 0.2:
        continue
    pos = world(uu, 0.14, TOP_Y)
    P.glass(*pos, kind=kind, drink=drink, ice=kind in ("rocks", "highball"))
peanut = common.plain_material("Peanuts", (0.62, 0.42, 0.22), 0.0, 0.7)
for uu in (1.95, 3.75, 6.15):
    c = world(uu, 0.06, TOP_Y)
    common.lathe("Peanut bowl", [(0, 0), (0.04, 0), (0.07, 0.04), (0.068, 0.045), (0.036, 0.008), (0, 0.008)],
                 P.mat("dark_wood"), 18).data.transform(Matrix.Translation(c))
    for k in range(14):
        a, r = rng.uniform(0, 6.3), rng.uniform(0, 0.045)
        nut = common.sphere("Peanut", Vector((0, 0, 0)), 0.008, peanut, 6, 4)
        nut.data.transform(Matrix.Translation((c[0] + r * math.cos(a), TOP_Y + 0.025 + rng.uniform(0, 0.012),
                                               c[2] + r * math.sin(a))) @ Matrix.Rotation(a, 4, "Y") @
                           Matrix.Diagonal((1.6, 1, 1, 1)))
phone_px = np.zeros((64, 32, 3), np.float32)
phone_px[:] = common.srgb((0.05, 0.08, 0.2))
phone_px[40:56, 4:28] = common.srgb((0.3, 0.5, 0.9))
phone_px[20:30, 6:26] = common.srgb((0.9, 0.9, 0.95))
phone_glow = common.glow_image_material("Phone screen", phone_px, strength=0.5)
pp = world(3.95, 0.02, TOP_Y)
ph = common.obox("Phone", (pp[0], TOP_Y + 0.004, pp[2]), (0.075, 0.008, 0.155), yaw_at(3.95) + 0.3, P.mat("black"),
                 0.008)
scr = common.quad("Phone screen", [(-0.034, 0.0085, 0.073), (0.034, 0.0085, 0.073), (0.034, 0.0085, -0.073),
                                   (-0.034, 0.0085, -0.073)], phone_glow)
scr.data.transform(Matrix.Translation((pp[0], TOP_Y, pp[2])) @ Matrix.Rotation(yaw_at(3.95) + 0.3, 4, "Y"))
kp = world(5.25, 0.05, TOP_Y)
common.torus("Key ring", Vector((kp[0], TOP_Y + 0.003, kp[2])), 0.015, 0.002, steel, segments=16, sides=4)
for k in range(3):
    common.obox("Key", (kp[0] + 0.03 * math.cos(k), TOP_Y + 0.002, kp[2] + 0.03 * math.sin(k)), (0.05, 0.003, 0.016),
                k * 0.9, brass if k else steel, 0.002)
for uu in (0.7, 2.25, 3.85, 5.4, 7.0):
    if uu > U_CURVE_END - 0.2:
        continue
    c = world(uu, -0.05, TOP_Y)
    common.lathe("Votive", [(0, 0), (0.03, 0), (0.033, 0.07), (0.0, 0.07)],
                 common.glass_material("Amber votive", color=(0.9, 0.5, 0.2), alpha=0.5), 14
                 ).data.transform(Matrix.Translation(c))
    P.candle(c[0], TOP_Y + 0.004, c[2], h=0.035, r=0.022)

common.finish_and_export(out_path, uv_scale={"Bar top mahogany": 0.8, "Counter mahogany": 1.0, "Arm rest leather": 2.0})
