# Builds the new back bar against the back wall, from the floor to the
# soffit (docs/archive/BAR_REDESIGN.md): paneled lower cabinets with a marble top,
# eight mirrored bays with five lit shelves of bottles each, a glowing onyx
# centerpiece with the premium bottles, a wine lattice, hanging stemware, a
# rolling library ladder on a brass rail, and the working clutter on the
# counter: espresso machine with cups, a vintage brass register, glass
# stacks, a citrus cutting board, a banker's lamp, an ice bucket with
# champagne, cocktail books, a plant. The remake (J35.4): bottles of
# see-through glass with the drink inside and printed labels of made-up
# brands (props_lib, labels.py); light strips solid from post to post.
#
#   blender -b --factory-startup --python tools/assets/back_bar_tall.py -- <texture dir> <out.glb> [<bottles.json>]
#
# The bottles are their own things since J72: this writes where each stands
# (<bottles.json>, for the place file) and tools/assets/bottles.py makes
# their models.
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

import json  # noqa: E402

args = sys.argv[sys.argv.index("--") + 1:]
tex_dir, out_path = args[:2]
bottles_path = args[2] if len(args) > 2 else None
BOTTLES = []
common.reset_scene()
P = props_lib.Props(tex_dir, seed=33, record=BOTTLES)
rng = random.Random(8)

X0, X1, ZB = L.BACK_BAR_X0, L.BACK_BAR_X1, L.Z0
DEPTH, CAB_H = 0.55, 0.95
TOP = CAB_H + 0.04          # counter surface
SHELF_D = 0.3
SHELVES = (1.32, 1.68, 2.04, 2.40, 2.76)
CEIL = L.SOFFIT_Y - 0.065   # under the soffit slats
BAYS = 8
BAY_W = (X1 - X0) / BAYS
POST_W = 0.09
CENTER_BAY = 4             # the onyx bay
WINE_BAY = 0
STEM_BAY = 6

wood = common.pbr_material("Back bar wood", tex_dir, "dark_wood", tint=(0.5, 0.32, 0.26), roughness_scale=0.55,
                            size=512)
marble = common.pbr_material("Marble top", tex_dir, "marble_01", tint=(0.9, 0.88, 0.85), roughness_scale=0.3,
                             grain="u", size=512)
mirror = common.plain_material("Smoked mirror", (0.30, 0.24, 0.19), 1.0, 0.06)
led = common.plain_material("LED strip", (1.0, 0.75, 0.45), 0.0, 0.3, emission=(1.0, 0.55, 0.22), strength=1.8)
kick = common.plain_material("Kick plate", (0.02, 0.02, 0.02), 0.0, 0.6)
brass = P.mat("brass")
shelf_glass = common.glass_material("Shelf glass", color=(0.7, 0.85, 0.8), alpha=0.25)

# Backlit onyx: the marble texture tinted amber, glowing.
_, _, mpx = common._load_pixels(os.path.join(tex_dir, "marble_01_diff_1k.jpg"), 512)
onyx_px = mpx[..., :3] * np.array([1.0, 0.55, 0.18])[None, None, :]
onyx = common.glow_image_material("Backlit onyx", onyx_px, strength=1.6, base_scale=0.6)


def box(name, lo, hi, mat, bevel=0.005, segments=2, grain_axis="x"):
    obj = common.box(name, lo, hi, mat, bevel, segments)
    obj["grain_axis"] = grain_axis
    return obj


def bay_x(i):
    return X0 + i * BAY_W


# ---- Lower cabinets ----
box("Cabinet body", (X0, 0.1, ZB), (X1, CAB_H, ZB + DEPTH - 0.02), wood, 0.003)
box("Toe kick", (X0 + 0.02, 0.0, ZB), (X1 - 0.02, 0.1, ZB + DEPTH - 0.06), kick, 0.002)
doors = 16
dw = (X1 - X0) / doors
for i in range(doors):
    x0 = X0 + i * dw + 0.008
    x1 = x0 + dw - 0.016
    zf = ZB + DEPTH
    if i in (7, 8):  # open shelves with stacked glassware in the middle
        box("Open shelf", (x0, 0.45, ZB + 0.05), (x1, 0.47, zf - 0.03), wood, 0.003)
        for j in range(4):
            for row, y in enumerate((0.12, 0.47)):
                gx = x0 + 0.08 + j * 0.09
                for st in range(3):  # stacks of rocks glasses
                    g = common.lathe("Stacked glass", [(0, 0), (0.04, 0), (0.041, 0.085), (0.0, 0.085)],
                                     P.mat("glass"), 12)
                    g.data.transform(Matrix.Translation((gx, y + st * 0.07, zf - 0.15)))
        continue
    box("Door", (x0, 0.12, zf - 0.02), (x1, CAB_H - 0.02, zf), wood, 0.006, 2, "y")
    box("Door field", (x0 + 0.07, 0.2, zf - 0.005), (x1 - 0.07, CAB_H - 0.1, zf + 0.012), wood, 0.016, 3, "y")
    kx = x1 - 0.05 if i % 2 == 0 else x0 + 0.05
    common.cylinder("Pull", Vector((kx, 0.75, zf + 0.025)), 0.008, 0.14, "y", brass, segments=8)
    for yy in (0.68, 0.82):
        common.cylinder("Pull post", Vector((kx, yy, zf + 0.012)), 0.005, 0.025, "z", brass, segments=6)
box("Marble top", (X0 - 0.03, CAB_H, ZB), (X1 + 0.03, TOP, ZB + DEPTH + 0.04), marble, 0.012, 4)

# ---- Upper bays ----
for i in range(BAYS + 1):
    x = bay_x(i) - (POST_W if i == BAYS else 0)
    box("Post", (x, TOP, ZB), (x + POST_W, CEIL, ZB + SHELF_D + 0.03), wood, 0.008, 2, "y")
    box("Post cap", (x - 0.01, TOP, ZB), (x + POST_W + 0.01, TOP + 0.08, ZB + SHELF_D + 0.05), wood, 0.01, 3, "y")
    box("Post fluting", (x + 0.025, TOP + 0.15, ZB + SHELF_D + 0.03), (x + POST_W - 0.025, CEIL - 0.3,
                                                                        ZB + SHELF_D + 0.036), kick, 0.002, 1, "y")
box("Crown", (X0 - 0.04, CEIL - 0.16, ZB), (X1 + 0.04, CEIL, ZB + SHELF_D + 0.1), wood, 0.015, 3)
box("Crown dentils", (X0, CEIL - 0.2, ZB + SHELF_D + 0.03), (X1, CEIL - 0.16, ZB + SHELF_D + 0.06), wood, 0.004)
for k in range(int((X1 - X0) / 0.07)):
    x = X0 + 0.02 + k * 0.07
    box("Dentil", (x, CEIL - 0.24, ZB + SHELF_D + 0.03), (x + 0.04, CEIL - 0.2, ZB + SHELF_D + 0.07), wood, 0.003)

for b in range(BAYS):
    bx0 = bay_x(b) + (POST_W if b > 0 else POST_W)
    bx1 = bay_x(b + 1) - (0.0 if b < BAYS - 1 else POST_W)
    if b == CENTER_BAY:
        # Backlit onyx panel in a brass frame, two glass shelves of the good stuff.
        common.panel("Onyx", ((bx0 + bx1) / 2, (TOP + CEIL - 0.24) / 2, ZB + 0.02), (1, 0, 0), (0, 1, 0), bx1 - bx0,
                     CEIL - 0.24 - TOP, onyx)
        for (lo, hi) in (((bx0, TOP), (bx0 + 0.03, CEIL - 0.24)), ((bx1 - 0.03, TOP), (bx1, CEIL - 0.24)),
                         ((bx0, CEIL - 0.27), (bx1, CEIL - 0.24))):
            box("Onyx frame", (lo[0], lo[1], ZB + 0.02), (hi[0], hi[1], ZB + 0.05), brass, 0.004)
        for y in (1.6, 2.25):
            box("Glass shelf", (bx0, y - 0.006, ZB + 0.03), (bx1, y + 0.006, ZB + SHELF_D), shelf_glass, 0.002)
            for k in range(5):
                x = bx0 + 0.12 + k * (bx1 - bx0 - 0.24) / 4
                P.bottle(x, y + 0.006, ZB + 0.17, rng.choice(["cognac", "magnum", "whiskey", "square"]))
        continue
    common.panel("Mirror", ((bx0 + bx1) / 2, (TOP + CEIL) / 2, ZB + 0.012), (1, 0, 0), (0, 1, 0), bx1 - bx0,
                 CEIL - TOP, mirror)
    if b == WINE_BAY:
        # Diamond lattice wine rack, bottles lying down, necks out.
        lat = [((bx0, TOP + 0.05), (bx1, CEIL - 0.3))]
        (lx0, ly0), (lx1, ly1) = lat[0]
        step = 0.17
        for k in range(-12, 14):
            for sgn in (-1, 1):
                pts = []
                for t in np.linspace(0, ly1 - ly0, 2):
                    pts.append((lx0 + k * step + sgn * t, ly0 + t))
                # Clip the diagonal slat to the bay.
                (ax, ay), (bx_, by) = pts
                ts = [0.0, 1.0]
                for (v0, v1, lo, hi) in ((ax, bx_, lx0, lx1),):
                    if v1 != v0:
                        t0, t1 = (lo - v0) / (v1 - v0), (hi - v0) / (v1 - v0)
                        ts = [max(ts[0], min(t0, t1)), min(ts[1], max(t0, t1))]
                if ts[0] >= ts[1]:
                    continue
                p0 = (ax + (bx_ - ax) * ts[0], ay + (by - ay) * ts[0])
                p1 = (ax + (bx_ - ax) * ts[1], ay + (by - ay) * ts[1])
                ln = math.dist(p0, p1)
                if ln < 0.05:
                    continue
                mid = ((p0[0] + p1[0]) / 2, (p0[1] + p1[1]) / 2)
                slat = common.box("Lattice", (-ln / 2, -0.008, 0), (ln / 2, 0.008, SHELF_D), wood, 0.002)
                slat.data.transform(Matrix.Translation((mid[0], mid[1], ZB)) @
                                    Matrix.Rotation(sgn * math.pi / 4, 4, "Z"))
        # Lying down, neck out toward +z (it was -pi/2: necks into the wall, so the
        # rack looked empty; the user's screenshots, KI-45).
        lay = Matrix.Rotation(math.pi / 2, 4, "X")
        for row in range(30):  # the whole rack (only its bottom third had bottles)
            for col in range(5):
                x = lx0 + 0.09 + col * step + (step / 2 if row % 2 else 0)
                y = ly0 + 0.09 + row * step / 2 * 1.0
                if x > lx1 - 0.08 or y > ly1 - 0.08:
                    continue
                P.bottle(x, y, ZB + 0.02, "wine", color=rng.choice([1, 3, 6, 8]), yaw=0.0, tilt=lay)
        box("Wine top shelf", (bx0, CEIL - 0.3, ZB), (bx1, CEIL - 0.27, ZB + SHELF_D), wood, 0.004)
        continue
    for si, y in enumerate(SHELVES):
        box("Shelf", (bx0, y - 0.0175, ZB + 0.012), (bx1, y + 0.0175, ZB + SHELF_D), wood, 0.006)
        box("Shelf lip", (bx0, y - 0.03, ZB + SHELF_D - 0.012), (bx1, y + 0.025, ZB + SHELF_D + 0.005), wood, 0.006)
        # The light strip (J35.4): post to post, a 2 cm diffuser just below
        # the lip's front, so it reads as one solid line from below and
        # from the front (it was 1 cm, behind the lip: broken dashes).
        box("LED", (bx0 + 0.005, y - 0.05, ZB + SHELF_D - 0.03), (bx1 - 0.005, y - 0.03, ZB + SHELF_D - 0.004), led,
            0.0)
        if b == STEM_BAY and si == 1:
            continue  # stemware hangs under this shelf
        if b == STEM_BAY and si == 2:
            # Wine glasses hanging upside down from three rails.
            for r in range(3):
                zr = ZB + 0.06 + r * 0.09
                common.cylinder("Stem rail", Vector(((bx0 + bx1) / 2, y - 0.03, zr)), 0.004, bx1 - bx0 - 0.04, "x",
                                P.mat("chrome"), segments=6)
                x = bx0 + 0.06
                while x < bx1 - 0.05:
                    g = common.lathe("Hanging glass", [(0, 0), (0.035, 0), (0.035, 0.003), (0.004, 0.008),
                                                       (0.0035, 0.1), (0.03, 0.12), (0.043, 0.16), (0.04, 0.21),
                                                       (0.034, 0.23), (0.0, 0.23)], P.mat("glass"), 12)
                    g.data.transform(Matrix.Translation((x, y - 0.035, zr)) @ Matrix.Rotation(math.pi, 4, "X"))
                    x += 0.1
            continue
        top_shelf = si == len(SHELVES) - 1
        if top_shelf and b % 2 == 1:
            # Decor on some top shelves: old lanterns, a trophy, books.
            P.book_row(bx0 + 0.05, y + 0.0175, ZB + 0.15, (bx1 - bx0) * 0.45, depth=0.15, max_h=0.24)
            tx = bx1 - 0.25
            common.lathe("Trophy", [(0, 0), (0.06, 0), (0.06, 0.04), (0.02, 0.06), (0.015, 0.14), (0.06, 0.2),
                                    (0.07, 0.26), (0.0, 0.24)], brass, 16).data.transform(
                Matrix.Translation((tx, y + 0.0175, ZB + 0.15)))
            continue
        x = bx0 + 0.04
        while x < bx1 - 0.08:
            kind = rng.choice(P.KINDS + ["champagne", "liqueur"])
            P.bottle(x + 0.04, y + 0.0175, ZB + SHELF_D * 0.55 + rng.uniform(-0.03, 0.03), kind,
                     yaw=rng.uniform(-0.3, 0.3))
            x += rng.uniform(0.12, 0.17)

# ---- Library ladder on a brass rail ----
RAIL_Y, RAIL_Z = CEIL - 0.32, ZB + SHELF_D + 0.18
common.cylinder("Ladder rail", Vector(((X0 + X1) / 2, RAIL_Y, RAIL_Z)), 0.016, X1 - X0 - 0.2, "x", brass, segments=12)
for i in range(BAYS + 1):
    x = min(max(bay_x(i), X0 + 0.1), X1 - 0.1)
    common.cylinder("Rail bracket", Vector((x, RAIL_Y, (ZB + SHELF_D + RAIL_Z) / 2 + 0.01)), 0.01,
                    RAIL_Z - ZB - SHELF_D + 0.02, "z", brass, segments=8)
# Parked at the right end of its rail and standing steep, so its feet
# leave the bartender's aisle free (it was mid-way, its feet 0.55 m out).
LX = X1 - 0.5
foot_z = ZB + DEPTH + 0.32
lean = math.atan2(RAIL_Z - foot_z, RAIL_Y - 0.06)
lad_len = math.hypot(foot_z - RAIL_Z, RAIL_Y - 0.06)
ladder_wood = common.pbr_material("Ladder wood", tex_dir, "wood_table_001", tint=(0.75, 0.5, 0.35), size=256,
                                  grain="v")
tilt_m = Matrix.Translation((LX, 0.06, foot_z)) @ Matrix.Rotation(lean, 4, "X")
for side in (-0.22, 0.22):
    s = common.box("Ladder side", (side - 0.02, 0, -0.035), (side + 0.02, lad_len - 0.02, 0.035), ladder_wood, 0.006)
    s.data.transform(tilt_m)
    s["grain_axis"] = "y"
    hook = common.tube("Ladder hook", [(0, lad_len - 0.03, 0), (0, lad_len + 0.02, -0.0), (0, lad_len + 0.05, -0.03),
                                       (0, lad_len + 0.03, -0.06)], 0.008, brass, sides=6)
    hook.data.transform(tilt_m @ Matrix.Translation((side, 0, 0)))
    wheel = common.cylinder("Ladder wheel", Vector((side, 0.0, 0.0)), 0.05, 0.025, "x", P.mat("black"), segments=14)
    wheel.data.transform(Matrix.Translation((LX, 0.05 - 0.06, foot_z)) @ Matrix.Translation((0, 0.06, 0)))
for k in range(8):
    rung = common.box("Rung", (-0.2, -0.015, -0.06), (0.2, 0.015, 0.06), ladder_wood, 0.006)
    rung.data.transform(tilt_m @ Matrix.Translation((0, 0.3 + k * (lad_len - 0.5) / 7, 0.0)))

# ---- On the counter ----
cz = ZB + DEPTH / 2
# Espresso machine with two group heads, portafilters, steam wand, cups on top.
ex = X1 - 0.75
chrome = P.mat("chrome")
box("Espresso body", (ex - 0.36, TOP, cz - 0.2), (ex + 0.36, TOP + 0.42, cz + 0.2), chrome, 0.03, 3)
box("Espresso panel", (ex - 0.34, TOP + 0.22, cz + 0.2), (ex + 0.34, TOP + 0.4, cz + 0.205),
    common.plain_material("Espresso red", (0.6, 0.05, 0.04), 0.3, 0.3), 0.01)
box("Drip tray", (ex - 0.33, TOP, cz + 0.1), (ex + 0.33, TOP + 0.03, cz + 0.3), chrome, 0.006)
for gx in (ex - 0.15, ex + 0.15):
    common.cylinder("Group head", Vector((gx, TOP + 0.2, cz + 0.24)), 0.04, 0.06, "y", chrome, segments=14)
    common.cylinder("Portafilter", Vector((gx, TOP + 0.16, cz + 0.25)), 0.035, 0.03, "y", chrome, segments=14)
    h = common.cylinder("Portafilter handle", Vector((0, 0, 0)), 0.012, 0.12, "z", P.mat("black"), segments=8)
    h.data.transform(Matrix.Translation((gx, TOP + 0.16, cz + 0.33)))
    common.lathe("Espresso cup", [(0, 0), (0.022, 0), (0.028, 0.05), (0.025, 0.05), (0.0, 0.006)], P.mat("white"), 14
                 ).data.transform(Matrix.Translation((gx, TOP + 0.03, cz + 0.22)))
common.tube("Steam wand", [(ex + 0.3, TOP + 0.3, cz + 0.22), (ex + 0.33, TOP + 0.2, cz + 0.26),
                           (ex + 0.33, TOP + 0.08, cz + 0.27)], 0.006, chrome, sides=6)
for k in range(6):
    common.lathe("Cup on top", [(0, 0), (0.03, 0), (0.04, 0.07), (0.037, 0.07), (0.0, 0.006)], P.mat("white"), 14
                 ).data.transform(Matrix.Translation((ex - 0.27 + k * 0.105, TOP + 0.42, cz - 0.05)) @
                                  Matrix.Rotation(math.pi, 4, "X") @ Matrix.Translation((0, -0.07, 0)))
# Vintage brass cash register.
rx = -3.95
box("Register base", (rx - 0.24, TOP, cz - 0.2), (rx + 0.24, TOP + 0.14, cz + 0.22), brass, 0.02, 3)
reg = common.box("Register body", (-0.22, 0, -0.16), (0.22, 0.24, 0.12), brass, 0.02, 3)
reg.data.transform(Matrix.Translation((rx, TOP + 0.14, cz)) @ Matrix.Rotation(-0.25, 4, "X"))
box("Register sign", (rx - 0.15, TOP + 0.4, cz - 0.08), (rx + 0.15, TOP + 0.5, cz - 0.06), brass, 0.01)
keys = common.plain_material("Register keys", (0.9, 0.88, 0.8), 0.0, 0.3)
for r in range(4):
    for c in range(7):
        k = common.cylinder("Register key", Vector((rx - 0.15 + c * 0.05, TOP + 0.2 + r * 0.035, cz + 0.12 - r * 0.04)),
                            0.012, 0.02, "y", keys, segments=8)
common.cylinder("Register crank", Vector((rx + 0.27, TOP + 0.25, cz)), 0.01, 0.1, "x", brass, segments=8)
# Glass stacks, citrus cutting board, banker's lamp, ice bucket, books, plant.
for k, gx in enumerate((-8.4, -8.25, -7.4, -7.25, -1.2, -1.05)):
    for st in range(4):
        common.lathe("Stacked pint", [(0, 0), (0.03, 0), (0.044, 0.15), (0.0, 0.15)], P.mat("glass"), 14
                     ).data.transform(Matrix.Translation((gx, TOP + st * 0.03, cz + 0.05)))
board_x = -5.3
box("Cutting board", (board_x - 0.2, TOP, cz - 0.05), (board_x + 0.2, TOP + 0.025, cz + 0.2), P.mat("wood"), 0.006)
for k in range(5):
    P.wedge(board_x - 0.12 + k * 0.05, TOP + 0.03, cz + 0.08, k % 2, yaw=k * 0.7)
lemon = common.sphere("Lemon", Vector((board_x + 0.12, TOP + 0.055, cz + 0.1)), 0.03, P.mat("fruit"), 12, 8)
common.swatch_uvs(lemon, (1 * 4 + 2) / 32, 0.5)
knife = common.box("Knife blade", (-0.1, 0, -0.012), (0.06, 0.002, 0.012), P.mat("steel"), 0.001)
knife.data.transform(Matrix.Translation((board_x, TOP + 0.027, cz + 0.0)) @ Matrix.Rotation(0.3, 4, "Y"))
kh = common.box("Knife handle", (0.06, -0.004, -0.01), (0.15, 0.008, 0.01), P.mat("black"), 0.004)
kh.data.transform(Matrix.Translation((board_x, TOP + 0.027, cz + 0.0)) @ Matrix.Rotation(0.3, 4, "Y"))
# Banker's lamp: brass base and stem, green glass shade, warm glow.
lx = -2.4
common.cylinder("Lamp base", Vector((lx, TOP + 0.015, cz)), 0.08, 0.03, "y", brass, segments=18)
common.cylinder("Lamp stem", Vector((lx, TOP + 0.17, cz)), 0.01, 0.3, "y", brass, segments=8)
shade = common.cylinder("Lamp shade", Vector((0, 0, 0)), 0.07, 0.26, "x",
                        common.plain_material("Green shade", (0.02, 0.25, 0.08), 0.0, 0.15), segments=18)
shade.data.transform(Matrix.Translation((lx, TOP + 0.33, cz + 0.04)) @ Matrix.Diagonal((1, 1, 0.75, 1)))
bulb = common.cylinder("Lamp glow", Vector((lx, TOP + 0.29, cz + 0.04)), 0.05, 0.22, "x",
                       common.plain_material("Lamp bulb", (1, 0.85, 0.6), 0.0, 0.3, emission=(1.0, 0.7, 0.35),
                                             strength=6.0), segments=12)
bulb.data.transform(Matrix.Diagonal((1, 0.4, 0.75, 1)))
bulb.data.transform(Matrix.Translation((0, 0, 0)))
# Ice bucket with champagne.
bx = -0.55
common.lathe("Ice bucket", [(0, 0), (0.08, 0), (0.1, 0.22), (0.11, 0.23), (0.105, 0.23), (0.09, 0.02), (0, 0.02)],
             P.mat("steel"), 20).data.transform(Matrix.Translation((bx, TOP, cz)))
P.bottle(bx, TOP + 0.04, cz, "champagne", color=1, tilt=Matrix.Rotation(0.3, 4, "X"))
for k in range(8):
    common.obox("Bucket ice", (bx + 0.05 * math.cos(k), TOP + 0.2, cz + 0.05 * math.sin(k)), (0.03, 0.03, 0.03), k,
                P.mat("ice"), 0.006)
P.book_row(-7.0, TOP, cz + 0.05, 0.35, depth=0.2, max_h=0.25)
P.plant(-4.75, TOP, cz, size=0.5, pot_r=0.08, pot_h=0.13, leaves=14, seed=3)
P.plant(-8.6, CEIL - 0.3, ZB + 0.15, size=0.7, leaves=10, seed=5, hanging=True)
P.frame(-3.1, TOP + 0.13, cz - 0.15, 0.2, 0.26, 3)

common.finish_and_export(out_path, uv_scale={"Back bar wood": 0.9, "Marble top": 0.6, "Ladder wood": 1.0})
if bottles_path:
    with open(bottles_path, "w") as f:
        json.dump(BOTTLES, f)
print("BOTTLES", len(BOTTLES))
