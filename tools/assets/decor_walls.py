# The decor pass for the new bar (docs/BAR_REDESIGN.md): Edison string
# lights across the hall, neon signs (COCKTAILS, GAME ON, a music note,
# PLAY), a gallery of framed photos, a collector wall of tin signs, license
# plates and records, a big clock and a sports TV on the soffit, a
# chalkboard A-frame, plants, corner speakers, brass wall sconces, fire
# extinguishers, smoke detectors, outlets and switches, coat hooks with
# coats, an umbrella stand, beer crates and a keg, and a little table in
# the bay window.
#
#   blender -b --factory-startup --python tools/assets/decor_walls.py -- <texture dir> <out.glb>
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

tex_dir, out_path = sys.argv[sys.argv.index("--") + 1:][:2]
common.reset_scene()
P = props_lib.Props(tex_dir, seed=99)
rng = random.Random(13)

bulb = common.plain_material("Edison bulb", (1.0, 0.8, 0.5), 0.0, 0.3, emission=(1.0, 0.62, 0.28), strength=9.0)
cord = common.plain_material("Cord", (0.02, 0.02, 0.02), 0.0, 0.6)
brass = P.mat("brass")
black = P.mat("black")
neon_cols = {
    "pink": common.plain_material("Neon pink", (1.0, 0.3, 0.7), 0.0, 0.3, emission=(1.0, 0.18, 0.55), strength=2.2),
    "cyan": common.plain_material("Neon cyan", (0.3, 0.9, 1.0), 0.0, 0.3, emission=(0.2, 0.85, 1.0), strength=2.2),
    "amber": common.plain_material("Neon amber", (1.0, 0.7, 0.3), 0.0, 0.3, emission=(1.0, 0.55, 0.15), strength=2.2),
    "green": common.plain_material("Neon green", (0.4, 1.0, 0.4), 0.0, 0.3, emission=(0.3, 1.0, 0.35), strength=2.2),
}
acrylic = common.glass_material("Neon backer", color=(0.5, 0.5, 0.6), alpha=0.18)


def place(obj, m):
    obj.data.transform(m)
    return obj


def wall_frame(x, z, facing):
    """Matrix for things on a wall: local x along the wall (to the viewer's
    right), local +z out of the wall. facing = the wall's normal into the
    room: "+x", "-x", "+z", "-z"."""
    yaw = {"+z": 0.0, "-z": math.pi, "+x": math.pi / 2, "-x": -math.pi / 2}[facing]
    return Matrix.Translation((x, 0, z)) @ Matrix.Rotation(yaw, 4, "Y")


# ---- Edison string lights: sagging strands across the hall ----
def strand(a, b, sag, n_bulbs):
    a, b = Vector(a), Vector(b)
    pts = []
    for i in range(41):
        t = i / 40
        p = a.lerp(b, t)
        p.y -= sag * 4 * t * (1 - t)
        pts.append(p)
    common.tube("String cord", pts, 0.004, cord, sides=4)
    for k in range(1, n_bulbs + 1):
        t = k / (n_bulbs + 1)
        p = a.lerp(b, t)
        p.y -= sag * 4 * t * (1 - t)
        common.cylinder("Socket", Vector((p.x, p.y - 0.03, p.z)), 0.012, 0.04, "y", black, segments=8)
        common.sphere("Bulb", Vector((p.x, p.y - 0.075, p.z)), 0.028, bulb, 10, 6).data.transform(
            Matrix.Translation((p.x, p.y - 0.075, p.z)) @ Matrix.Diagonal((1, 1.3, 1, 1)) @
            Matrix.Translation((-p.x, -(p.y - 0.075), -p.z)))


Y_HANG = L.H - 0.45
for (a, b) in (((-8.6, Y_HANG, -3.3), (5.5, Y_HANG, 6.6)), ((-8.6, Y_HANG, 6.6), (5.5, Y_HANG, -3.3)),
               ((-4.0, Y_HANG, -3.3), (8.6, Y_HANG, 6.0)), ((-8.6, Y_HANG, 2.2), (8.6, Y_HANG, 2.2))):
    length = (Vector(a) - Vector(b)).length
    strand(a, b, 0.45, int(length / 0.55))

# ---- Neon signs (pixel-font letters as lit blocks on clear acrylic) ----
glyphs = common._glyphs()


def neon_word(word, m, y, s, color, colors=None):
    x0 = -len(word) * 6 * s / 2
    for i, ch in enumerate(word):
        for r, row in enumerate(glyphs.get(ch, [])):
            for c, dot in enumerate(row):
                if dot == "#":
                    x = x0 + (i * 6 + c) * s
                    yy = y - r * s
                    mat = neon_cols[colors[i % len(colors)]] if colors else neon_cols[color]
                    place(common.box("Neon", (x - s * 0.33, yy - s * 0.33, 0.04), (x + s * 0.33, yy + s * 0.33, 0.055),
                                     mat, 0.0), m)
    place(common.box("Neon backer", (x0 - 0.06, y - 7 * s - 0.04, 0.0), (-x0 + 0.04, y + 0.06, 0.03), acrylic, 0.008), m)
    for xx in (x0, -x0 - 0.02):
        place(common.cylinder("Standoff", Vector((xx, y - 3 * s, 0.015)), 0.008, 0.03, "z", brass, segments=8), m)


# On the soffit's front face, over the bar.
neon_word("COCKTAILS", Matrix.Translation((-6.3, 0, L.SOFFIT_Z1 + 0.035)), 3.62, 0.042, "pink")
# Above the racing sims on the right wall.
neon_word("GAME ON", wall_frame(L.X1, -2.0, "-x"), 3.55, 0.05, "cyan", colors=["cyan", "green"])
# Near the jukebox on the front wall.
neon_word("PLAY", wall_frame(-3.8, L.Z1, "-z"), 3.2, 0.04, "amber")
# A music note over the stage (drawn as tubes), front wall.
note_m = wall_frame(-7.2, L.Z1, "-z") @ Matrix.Translation((0, 2.9, 0.05))
for (a, b) in (((-0.12, -0.3), (-0.12, 0.25)), ((0.18, -0.2), (0.18, 0.35)), ((-0.12, 0.25), (0.18, 0.35)),
               ((-0.12, 0.15), (0.18, 0.25))):
    place(common.tube("Neon note", [(a[0], a[1], 0), (b[0], b[1], 0)], 0.012, neon_cols["pink"], sides=6), note_m)
for (cx, cy) in ((-0.2, -0.3), (0.1, -0.2)):
    ring = [(cx + 0.08 * math.cos(t), cy + 0.055 * math.sin(t), 0) for t in np.linspace(0, 2 * math.pi, 17)]
    place(common.tube("Neon note head", ring, 0.012, neon_cols["pink"], sides=6), note_m)
place(common.box("Note backer", (-0.35, -0.42, -0.05), (0.32, 0.45, -0.02), acrylic, 0.008), note_m)

# ---- Gallery wall above the jukebox (front wall, green plaster) ----
gm = wall_frame(0, L.Z1, "-z")
for k, (x, y, w, h) in enumerate(((-5.0, 2.3, 0.45, 0.6), (-4.4, 2.55, 0.5, 0.38), (-4.45, 2.05, 0.3, 0.38),
                                  (-3.25, 2.5, 0.6, 0.45), (-2.65, 2.15, 0.35, 0.45), (-2.6, 2.75, 0.3, 0.3),
                                  (-4.95, 1.6, 0.35, 0.28), (-3.9, 1.95, 0.0, 0.0))):
    if w == 0:
        continue
    # wall_frame for -z flips x: world x = -local x.
    P.frame(x, y, L.Z1, w, h, k + 2, yaw=math.pi, depth=0.03, border=0.03 if k % 2 else 0.045,
            mat=brass if k % 3 == 0 else None)

# ---- Collector wall: back wall right of doorway A, above the crates ----
tin_px = np.zeros((256, 256, 3), np.float32)
designs = [((0.7, 0.08, 0.05), (0.95, 0.85, 0.6), "LAGER"), ((0.05, 0.15, 0.4), (0.95, 0.8, 0.3), "STOUT"),
           ((0.1, 0.3, 0.12), (0.95, 0.95, 0.85), "ALE"), ((0.9, 0.75, 0.2), (0.15, 0.05, 0.05), "COLA")]
for k, (bg, fg, word) in enumerate(designs):
    r, c = divmod(k, 2)
    cell = np.zeros((128, 128, 3), np.float32)
    cell[:] = common.srgb(bg)
    cell[6:10, :] = cell[-10:-6, :] = cell[:, 6:10] = cell[:, -10:-6] = common.srgb(fg)
    common.draw_text(cell, word, 64, 74, 4, common.srgb(fg), center=True)
    common.draw_text(cell, "EST 1962", 64, 34, 1, common.srgb(fg), center=True)
    cell *= (0.8 + 0.2 * common.noise2d(128, 128, 8, k))[..., None]  # rust and wear
    tin_px[r * 128:(r + 1) * 128, c * 128:(c + 1) * 128] = cell
tin = common.image_material("Tin signs", tin_px, metallic=0.6, roughness=0.45)
plate_px = np.zeros((64, 256, 3), np.float32)
for k, (bg, fg, txt) in enumerate((((0.9, 0.9, 0.85), (0.1, 0.2, 0.6), "BAR 3D"), ((0.95, 0.8, 0.2), (0.1, 0.1, 0.1), "JAZZ 88"),
                                   ((0.2, 0.3, 0.6), (0.95, 0.95, 0.95), "PIANO1"), ((0.9, 0.3, 0.2), (1, 1, 1), "4K MANIA"))):
    cell = np.zeros((64, 64, 3), np.float32)
    cell[:] = common.srgb(bg)
    common.draw_text(cell, txt[:6], 32, 38, 1, common.srgb(fg), center=True)
    plate_px[:, k * 64:(k + 1) * 64] = cell
plates = common.image_material("License plates", plate_px, metallic=0.5, roughness=0.4)
vinyl = common.plain_material("Vinyl", (0.01, 0.01, 0.012), 0.0, 0.25)
labels = [common.plain_material(f"Record label {i}", c, 0.0, 0.5) for i, c in enumerate(
    [(0.8, 0.1, 0.1), (0.95, 0.8, 0.2), (0.2, 0.4, 0.8)])]
cw = Matrix.Translation((0, 0, L.Z0))   # back wall, facing +z, x as is
for k, (x, y, s) in enumerate(((4.85, 2.9, 0.42), (5.55, 2.55, 0.38), (5.0, 2.2, 0.32), (5.6, 3.3, 0.3))):
    r, c = divmod(k, 2)
    u0, v0 = c * 0.5 + 0.01, r * 0.5 + 0.01
    place(common.box("Tin sign", (x - s / 2, y - s / 2, 0.0), (x + s / 2, y + s / 2, 0.01), black, 0.004), cw)
    place(common.quad("Tin face", [(x - s / 2, y - s / 2, 0.011), (x + s / 2, y - s / 2, 0.011), (x + s / 2, y + s / 2, 0.011),
                                   (x - s / 2, y + s / 2, 0.011)], tin,
                      uvs=((u0, v0), (u0 + 0.48, v0), (u0 + 0.48, v0 + 0.48), (u0, v0 + 0.48))), cw)
for k in range(4):
    x, y = 4.9 + (k % 2) * 0.4, 1.55 + (k // 2) * 0.24
    place(common.quad("Plate", [(x - 0.15, y - 0.075, 0.006), (x + 0.15, y - 0.075, 0.006), (x + 0.15, y + 0.075, 0.006),
                                (x - 0.15, y + 0.075, 0.006)], plates,
                      uvs=((k / 4, 0), ((k + 1) / 4, 0), ((k + 1) / 4, 1), (k / 4, 1))), cw)
    place(common.box("Plate back", (x - 0.152, y - 0.077, 0.0), (x + 0.152, y + 0.077, 0.005), P.mat("steel"), 0.002), cw)
for k, (x, y) in enumerate(((5.75, 1.9), (4.75, 3.45), (5.9, 2.95))):
    place(common.cylinder("Record", Vector((x, y, 0.006)), 0.15, 0.004, "z", vinyl, segments=32), cw)
    place(common.cylinder("Record label", Vector((x, y, 0.009)), 0.05, 0.002, "z", labels[k], segments=20), cw)
    for rr in (0.08, 0.11, 0.135):
        place(common.torus("Groove", Vector((x, y, 0.0085)), rr, 0.0012,
                           common.plain_material("Groove shine", (0.1, 0.1, 0.11), 0.5, 0.2), "z", 32, 3), cw)

# ---- Soffit fascia: a big clock and a TV showing a game ----
clock_px = np.ones((128, 128, 3), np.float32) * np.array(common.srgb((0.95, 0.92, 0.85)))
yy, xx = np.mgrid[0:128, 0:128]
d = np.sqrt((xx - 64) ** 2 + (yy - 64) ** 2)
clock_px[d > 60] = 0.05
for h in range(12):
    a = h * math.pi / 6
    common.draw_text(clock_px, str(12 if h == 0 else h), int(64 + 46 * math.sin(a)), int(64 + 46 * math.cos(a)) + 4, 1,
                     (0.05, 0.05, 0.05), center=True)
for (a, ln, wdt) in ((math.radians(-40), 30, 2), (math.radians(110), 44, 1)):
    for t in range(ln):
        px_, py_ = int(64 + t * math.sin(a)), int(64 + t * math.cos(a))
        clock_px[py_ - wdt:py_ + wdt, px_ - wdt:px_ + wdt] = 0.03
clock_mat = common.image_material("Wall clock", clock_px, roughness=0.3)
CX, CY, CZ = -2.6, 3.42, L.SOFFIT_Z1 + 0.035
common.cylinder("Clock rim", Vector((CX, CY, CZ + 0.03)), 0.3, 0.06, "z", brass, segments=40)
common.panel("Clock face", (CX, CY, CZ + 0.062), (1, 0, 0), (0, 1, 0), 0.54, 0.54, clock_mat)
tv_px = np.zeros((144, 256, 3), np.float32)
tv_px[:] = common.srgb((0.1, 0.45, 0.15))
for k in range(0, 256, 32):
    tv_px[:, k:k + 16] *= 1.15
tv_px[70:74, :] = 0.9
tv_px[:, 126:130] = 0.9
ring_d = np.sqrt((np.mgrid[0:144, 0:256][1] - 128) ** 2 + (np.mgrid[0:144, 0:256][0] - 72) ** 2)
tv_px[(ring_d > 26) & (ring_d < 30)] = 0.9
for (px_, py_, col) in ((60, 40, (0.9, 0.1, 0.1)), (90, 100, (0.9, 0.1, 0.1)), (170, 60, (0.1, 0.2, 0.9)),
                        (200, 90, (0.1, 0.2, 0.9)), (140, 80, (1, 1, 1))):
    tv_px[py_ - 4:py_ + 4, px_ - 3:px_ + 3] = col
tv_px[124:144, 0:120] = common.srgb((0.05, 0.05, 0.1))
common.draw_text(tv_px, "BAR 2-1 FC", 60, 140, 1, (1, 1, 1), center=True)
common.draw_text(tv_px, "78:12", 100, 132, 1, common.srgb((1, 0.9, 0.2)), center=True)
tv_mat = common.glow_image_material("TV picture", tv_px, strength=1.1)
TX = 0.9
common.box("TV body", (TX - 0.62, 3.05, CZ), (TX + 0.62, 3.75, CZ + 0.05), black, 0.01)
common.panel("TV screen", (TX, 3.4, CZ + 0.0505), (1, 0, 0), (0, 1, 0), 1.18, 0.66, tv_mat)

# ---- Chalkboard A-frame by the door ----
cb_px = np.zeros((256, 160, 3), np.float32)
cb_px[:] = common.srgb((0.08, 0.1, 0.09))
cb_px *= (0.85 + 0.3 * common.noise2d(256, 160, 6, 2))[..., None]
chalk = common.srgb((0.92, 0.92, 0.88))
for i, (line, sc) in enumerate((("TONIGHT", 2), ("", 1), ("LIVE PIANO", 1), ("9 PM", 2), ("", 1), ("HAPPY HOUR", 1),
                                ("5 - 7", 2), ("", 1), ("DARTS", 1), ("TOURNEY", 1), ("THURSDAY", 1))):
    if line:
        common.draw_text(cb_px, line, 80, 240 - i * 20, sc, chalk if i % 3 else common.srgb((1.0, 0.75, 0.4)),
                         center=True)
chalkboard = common.image_material("A-frame chalk", cb_px, roughness=0.9)
af = Matrix.Translation((1.95, 0, L.Z1 - 1.25)) @ Matrix.Rotation(0.5, 4, "Y")
AF_H = 1.0 / math.cos(0.2)  # each board's length (the sign stands 1 m tall)
for side in (-1, 1):
    # Hinged at the top (like a real A-frame), the feet apart at the bottom.
    # Built standing from y = 0 to AF_H, then turned about its top edge.
    m = (af @ Matrix.Translation((0, 1.0, 0)) @ Matrix.Rotation(-side * 0.2, 4, "X") @
         Matrix.Translation((0, -AF_H, 0)) @ Matrix.Diagonal((1, AF_H, 1, 1)))
    place(common.box("A-frame", (-0.32, 0.0, -0.012), (0.32, 1.0, 0.012), P.mat("wood"), 0.006), m)
    place(common.quad("Chalk", [(0.27, 0.12, side * 0.013), (-0.27, 0.12, side * 0.013), (-0.27, 0.95, side * 0.013),
                                (0.27, 0.95, side * 0.013)] if side < 0 else
                      [(-0.27, 0.12, 0.013), (0.27, 0.12, 0.013), (0.27, 0.95, 0.013), (-0.27, 0.95, 0.013)],
                      chalkboard), m)
# The hinge along the top, and a chain holding the feet.
common.tube("A-frame hinge", [af @ Vector((-0.3, 0.985, 0)), af @ Vector((0.3, 0.985, 0))], 0.012, P.mat("brass"), sides=8)
for x in (-0.25, 0.25):
    common.tube("A-frame chain", [af @ Vector((x, 0.35, -0.13)), af @ Vector((x, 0.33, 0.0)), af @ Vector((x, 0.35, 0.13))],
                0.003, P.mat("steel"), sides=4)

# ---- Plants ----
P.plant(-1.85, 0.0, L.Z1 - 0.3, size=1.6, pot_r=0.22, pot_h=0.42, leaves=34, seed=21)     # fig by the bay
P.plant(4.5, 0.0, L.Z1 - 0.35, size=1.3, pot_r=0.2, pot_h=0.38, leaves=26, seed=22)       # by the door
P.plant(-5.15, L.STEP_H, L.Z1 - 0.35, size=1.1, pot_r=0.18, pot_h=0.34, leaves=22, seed=23)
P.plant(8.55, 0.0, 6.6, size=1.2, pot_r=0.2, pot_h=0.4, leaves=24, seed=24)
for (x, z) in ((-1.2, 7.55), (0.8, 7.55)):  # hanging in the bay window
    common.tube("Plant hanger", [(x, L.BAY_H, z), (x, 2.35, z)], 0.003, cord, sides=4)
    common.lathe("Hanging pot", [(0, 0), (0.1, 0.02), (0.13, 0.16), (0.0, 0.16)], P.mat("white"), 16).data.transform(
        Matrix.Translation((x, 2.2, z)))
    P.plant(x, 2.36, z, size=0.8, leaves=14, seed=int(x * 10) + 40, hanging=True)

# ---- Corner speakers, sconces, safety things ----
for (x, z, yaw) in ((-8.7, -3.3, math.pi / 4 + math.pi / 2), (8.7, 6.7, -math.pi * 3 / 4), (-8.7, 6.7, -math.pi / 4 + math.pi),
                    (8.7, -4.0, -math.pi / 4)):
    m = Matrix.Translation((x, 3.35, z)) @ Matrix.Rotation(yaw, 4, "Y") @ Matrix.Rotation(-0.25, 4, "X")
    place(common.box("Speaker", (-0.15, -0.22, -0.13), (0.15, 0.22, 0.13), black, 0.015), m)
    place(common.box("Speaker grill", (-0.135, -0.2, 0.13), (0.135, 0.2, 0.135),
                     common.plain_material("Grill cloth", (0.04, 0.04, 0.045), 0.0, 0.9), 0.004), m)
sconce_glass = common.plain_material("Sconce glass", (1.0, 0.85, 0.6), 0.0, 0.3, emission=(1.0, 0.68, 0.35), strength=3.5)
for (x, z, facing) in ((-1.0 + 0, L.Z1, "-z"), (-5.45 - 0.0, L.Z1, "-z"), (L.X0, -4.6, "+x"), (L.X0, 3.15, "+x")):
    m = wall_frame(x, z, facing)
    place(common.box("Sconce plate", (-0.06, 1.85, 0.0), (0.06, 2.05, 0.015), brass, 0.004), m)
    place(common.tube("Sconce arm", [(0, 1.95, 0.01), (0, 1.97, 0.12), (0, 2.05, 0.16)], 0.007, brass, sides=6), m)
    place(common.lathe("Sconce shade", [(0, 0), (0.035, 0.0), (0.06, 0.12), (0.055, 0.12), (0.0, 0.015)], sconce_glass, 14),
          m @ Matrix.Translation((0, 2.03, 0.16)))
red = common.plain_material("Extinguisher red", (0.7, 0.03, 0.02), 0.2, 0.35)
for (x, z, facing) in ((2.75, L.Z0, "+z"), (-5.6, L.Z1, "-z")):
    m = wall_frame(x, z, facing)
    place(common.cylinder("Extinguisher", Vector((0, 0.95, 0.1)), 0.075, 0.5, "y", red, segments=16), m)
    place(common.sphere("Ext top", Vector((0, 1.2, 0.1)), 0.075, red, 16, 8), m)
    place(common.box("Ext valve", (-0.02, 1.25, 0.07), (0.02, 1.32, 0.13), black, 0.004), m)
    place(common.tube("Ext hose", [(0.02, 1.3, 0.12), (0.09, 1.2, 0.14), (0.08, 0.9, 0.15)], 0.008, black, sides=6), m)
    place(common.box("Ext bracket", (-0.06, 0.85, 0.0), (0.06, 1.05, 0.025), black, 0.004), m)
for (x, z) in ((-4.0, 0.0), (3.0, 3.0), (-6.0, 5.0), (4.0, -2.0), (-1.0, 5.5)):
    common.cylinder("Smoke detector", Vector((x, L.H - 0.02, z)), 0.07, 0.035, "y", P.mat("white"), segments=18)
    common.sphere("Detector LED", Vector((x + 0.03, L.H - 0.04, z)), 0.004,
                  common.plain_material("Detector LED", (0.2, 1, 0.2), 0.0, 0.3, emission=(0.2, 1.0, 0.2), strength=2.2),
                  6, 4)
for (x, z, facing, y) in ((2.2, L.Z1, "-z", 1.25), (4.35, L.Z1, "-z", 0.35), (L.X1, 0.7, "-x", 1.25),
                          (-5.6, L.Z1, "-z", 0.35), (1.1, L.Z0, "+z", 0.35)):
    m = wall_frame(x, z, facing)
    place(common.box("Wall plate", (-0.04, y - 0.06, 0.0), (0.04, y + 0.06, 0.008), P.mat("white"), 0.003), m)
    place(common.box("Switch", (-0.008, y - 0.02, 0.008), (0.008, y + 0.02, 0.016), P.mat("white"), 0.002), m)

# ---- By the door: coat hooks with coats, umbrella stand, a trash can ----
hm = wall_frame(0, L.Z1, "-z")
HX0, HX1, HY = -4.35 - 0.1, -4.35 + 0.95, 1.75   # local x (world x = -local)
place(common.box("Coat rail", (HX0, HY - 0.06, 0.0), (HX1, HY + 0.06, 0.025), P.mat("dark_wood"), 0.008), hm)
coat_cols = [common.pbr_material(f"Coat {i}", tex_dir, "fabric_pattern_07", tint=c, size=256, saturation=0.3)
             for i, c in enumerate([(0.25, 0.2, 0.15), (0.12, 0.15, 0.25), (0.4, 0.1, 0.08)])]
for k in range(5):
    x = HX0 + 0.1 + k * (HX1 - HX0 - 0.2) / 4
    place(common.tube("Hook", [(x, HY, 0.02), (x, HY - 0.02, 0.08), (x, HY + 0.03, 0.1)], 0.006, brass, sides=6), hm)
    if k in (0, 2, 3):
        c = coat_cols[[0, 1, 2, 2][k % 4] if k < 4 else 0]
        coat = common.lathe("Coat", [(0.0, 0.0), (0.2, 0.02), (0.22, 0.4), (0.18, 0.75), (0.08, 0.92), (0.0, 0.95)], c, 14)
        place(coat, hm @ Matrix.Translation((x, HY - 0.95, 0.13)) @ Matrix.Diagonal((1.0, 1.0, 0.45, 1.0)))
place(common.lathe("Hat", [(0, 0), (0.17, 0), (0.17, 0.01), (0.1, 0.012), (0.095, 0.11), (0.0, 0.12)], black, 18),
      hm @ Matrix.Translation((HX0 + 0.1 + 1 * (HX1 - HX0 - 0.2) / 4, HY + 0.07, 0.12)) @ Matrix.Rotation(0.3, 4, "X"))
us = Matrix.Translation((4.35 + 0.25, 0, L.Z1 - 0.3))
place(common.lathe("Umbrella stand", [(0, 0), (0.12, 0), (0.13, 0.55), (0.12, 0.55), (0.11, 0.02), (0, 0.02)],
                   P.mat("brass"), 18), us)
for k, col in enumerate(((0.05, 0.05, 0.05), (0.5, 0.05, 0.05))):
    um = common.plain_material(f"Umbrella {k}", col, 0.0, 0.5)
    place(common.lathe("Umbrella", [(0, 0.15), (0.03, 0.25), (0.05, 0.75), (0.015, 0.85), (0, 0.85)], um, 10),
          us @ Matrix.Translation((0.03 * (k * 2 - 1), 0, 0)) @ Matrix.Rotation(0.1 * (k * 2 - 1), 4, "Z"))
    place(common.tube("Umbrella handle", [(0.0, 0.85, 0), (0, 0.98, 0), (0.04, 1.02, 0), (0.06, 0.98, 0)], 0.008, black,
                      sides=6), us @ Matrix.Translation((0.03 * (k * 2 - 1), 0, 0)) @ Matrix.Rotation(0.1 * (k * 2 - 1), 4, "Z"))
place(common.lathe("Trash can", [(0, 0), (0.17, 0), (0.19, 0.6), (0.2, 0.62), (0, 0.62)], P.mat("steel"), 20),
      Matrix.Translation((1.95, 0, L.Z1 - 0.3)))

# ---- Storage corner by doorway A: beer crates and a keg ----
crate = common.plain_material("Crate plastic", (0.6, 0.08, 0.05), 0.0, 0.5)
for k, (x, z, y, a) in enumerate(((4.95, L.Z0 + 0.3, 0.0, 0.0), (5.4, L.Z0 + 0.32, 0.0, 0.05), (5.15, L.Z0 + 0.3, 0.3, -0.1),
                                  (5.65, L.Z0 + 0.75, 0.0, 0.3))):
    m = Matrix.Translation((x, y, z)) @ Matrix.Rotation(a, 4, "Y")
    place(common.box("Crate", (-0.2, 0.0, -0.15), (0.2, 0.28, 0.15), crate, 0.01), m)
    for i in range(4):
        for j in range(3):
            P.bottle(x + (-0.15 + i * 0.1) * math.cos(a), y + 0.04, z + (-0.1 + j * 0.1), "beer", color=rng.choice([0, 9]),
                     segments=6)
keg = Matrix.Translation((5.75, 0, L.Z0 + 0.3))
place(common.lathe("Keg", [(0, 0), (0.18, 0), (0.2, 0.05), (0.2, 0.2), (0.205, 0.22), (0.2, 0.24), (0.2, 0.36), (0.205, 0.38),
                           (0.2, 0.4), (0.2, 0.55), (0.18, 0.6), (0.0, 0.6)], P.mat("steel"), 24), keg)
place(common.cylinder("Keg coupler", Vector((0, 0.63, 0)), 0.04, 0.06, "y", black, segments=12), keg)

# ---- Bay window: a little table with a candle and two drinks ----
common.cylinder("Bay table", Vector((-0.2, 0.55, 7.25)), 0.28, 0.03, "y", P.mat("dark_wood"), segments=24)
common.cylinder("Bay table post", Vector((-0.2, 0.27, 7.25)), 0.03, 0.54, "y", black, segments=10)
common.cylinder("Bay table foot", Vector((-0.2, 0.01, 7.25)), 0.2, 0.02, "y", black, segments=20)
P.candle(-0.2, 0.565, 7.25, h=0.06, r=0.03)
P.glass(-0.05, 0.565, 7.15, "coupe", "pink")
P.glass(-0.35, 0.565, 7.35, "wine", "white_wine")

common.finish_and_export(out_path, uv_scale={"Coat 0": 2.0, "Coat 1": 2.0, "Coat 2": 2.0})
