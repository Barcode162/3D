# Stage props around the grand piano (docs/BAR_REDESIGN.md): a microphone
# on a boom stand, an acoustic guitar with strings on its stand, a combo
# amp, a ceiling lighting bar with three PAR spotlights aimed at the piano,
# cables on the floor, and a stool.
#
#   blender -b --factory-startup --python tools/assets/stage.py -- <texture dir> <out.glb>
#
# Model space = world space; the stage floor is at y = STAGE_H.
import math
import os
import sys

import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402
import layout as L  # noqa: E402

tex_dir, out_path = sys.argv[sys.argv.index("--") + 1:][:2]
common.reset_scene()

Y = L.STAGE_H
black = common.plain_material("Stage black", (0.02, 0.02, 0.022), 0.4, 0.4)
chrome = common.plain_material("Stage chrome", (0.85, 0.85, 0.88), 1.0, 0.15)
grille = common.pbr_material("Amp grille", tex_dir, "fabric_pattern_07", tint=(0.35, 0.3, 0.25), size=256,
                             saturation=0.2)
tolex = common.pbr_material("Amp tolex", tex_dir, "leather_red_02", tint=(0.08, 0.08, 0.08), size=256)
spruce = common.pbr_material("Guitar top", tex_dir, "wood_table_001", tint=(1.2, 0.85, 0.5), grain="v", size=256)
rosewood = common.pbr_material("Guitar back", tex_dir, "dark_wood", tint=(0.5, 0.3, 0.2), size=256)
string_m = common.plain_material("Guitar string", (0.8, 0.75, 0.6), 1.0, 0.25)
lens = common.plain_material("PAR lens", (1.0, 0.9, 0.7), 0.0, 0.3, emission=(1.0, 0.75, 0.45), strength=8.0)
cream = common.plain_material("Amp cream", (0.85, 0.8, 0.65), 0.0, 0.4)

# ---- Microphone on a boom stand ----
MX, MZ = -6.25, 6.3
for k in range(3):
    a = k * 2 * math.pi / 3 + 0.3
    common.tube("Mic leg", [(MX, Y + 0.12, MZ), (MX + 0.32 * math.cos(a), Y + 0.01, MZ + 0.32 * math.sin(a))], 0.009,
                black, sides=6)
common.cylinder("Mic pole", Vector((MX, Y + 0.6, MZ)), 0.011, 1.0, "y", black, segments=10)
common.cylinder("Mic clutch", Vector((MX, Y + 1.12, MZ)), 0.018, 0.05, "y", chrome, segments=10)
boom_end = (MX - 0.42, Y + 1.5, MZ - 0.2)
common.tube("Mic boom", [(MX + 0.15, Y + 1.08, MZ + 0.07), (MX, Y + 1.12, MZ), boom_end], 0.008, black, sides=6)
d = Vector(boom_end) - Vector((MX, Y + 1.12, MZ))
d.normalize()
mic_base = Vector(boom_end)
common.tube("Mic body", [mic_base, mic_base + d * 0.13], 0.02, black, sides=12)
common.sphere("Mic grille", mic_base + d * 0.16, 0.028, chrome, 14, 10)
common.sphere("Boom weight", Vector((MX + 0.17, Y + 1.07, MZ + 0.08)), 0.03, black, 10, 6)
common.tube("Mic cable", [mic_base, (MX - 0.1, Y + 1.0, MZ - 0.05), (MX - 0.05, Y + 0.3, MZ), (MX + 0.3, Y + 0.005, MZ + 0.2),
                          (-6.9, Y + 0.005, 6.7)], 0.005, black, sides=5)

# ---- Acoustic guitar on an A-frame stand ----
GX, GZ = -5.72, 6.62
g = Matrix.Translation((GX, Y + 0.12, GZ)) @ Matrix.Rotation(-math.pi / 2 - 0.6, 4, "Y") @ \
    Matrix.Rotation(-0.25, 4, "X")


def body_outline(n=48):
    """Top view of a dreadnought-ish body: two lobes, waist in between."""
    pts = []
    for i in range(n):
        t = 2 * math.pi * i / n
        y = 0.25 + 0.25 * math.cos(t)          # 0 (bottom) .. 0.5 (top), along the guitar
        lobe = 0.2 if y < 0.22 else 0.15
        waist = 1.0 - 0.28 * math.exp(-((y - 0.27) / 0.06) ** 2)
        x = lobe * waist * math.sin(t)
        pts.append((x, y))
    return pts


outline = body_outline()
body = common.prism("Guitar body", [(x, -y) for (x, y) in outline], -0.05, 0.05, rosewood)
for v in body.data.vertices:  # prism builds (x, height, z); make it x, y (along), z (thickness)
    v.co = Vector((v.co.x, -v.co.z, v.co.y))
body.data.flip_normals()
body.data.transform(g)
top = common.prism("Guitar top", [(x * 0.99, -y * 0.99 - 0.002) for (x, y) in outline], 0.05, 0.053, spruce)
for v in top.data.vertices:
    v.co = Vector((v.co.x, -v.co.z, v.co.y))
top.data.flip_normals()
top.data.transform(g)
hole = common.cylinder("Sound hole", Vector((0, 0.33, 0.054)), 0.045, 0.002, "z", black, segments=24)
hole.data.transform(g)
ring = common.torus("Rosette", Vector((0, 0.33, 0.055)), 0.052, 0.004, cream, axis="z", segments=24, sides=4)
ring.data.transform(g)
br = common.box("Bridge", (-0.07, 0.11, 0.053), (0.07, 0.135, 0.062), black, 0.004)
br.data.transform(g)
nk = common.box("Neck", (-0.024, 0.5, 0.02), (0.024, 0.95, 0.055), rosewood, 0.008)
nk.data.transform(g)
fb = common.box("Fretboard", (-0.026, 0.42, 0.055), (0.026, 0.95, 0.061), black, 0.002)
fb.data.transform(g)
for k in range(18):
    fy = 0.95 - 0.47 * (1 - 0.944 ** k)
    fr = common.box("Fret", (-0.026, fy - 0.001, 0.061), (0.026, fy + 0.001, 0.0625), chrome, 0.0)
    fr.data.transform(g)
hs = common.box("Headstock", (-0.04, 0.95, 0.02), (0.04, 1.13, 0.045), rosewood, 0.006)
hs.data.transform(g @ Matrix.Translation((0, 0.95, 0.03)) @ Matrix.Rotation(-0.25, 4, "X") @ Matrix.Translation((0, -0.95, -0.03)))
for k in range(6):
    x = -0.02 + k * 0.008
    st = common.tube("String", [(x * 1.3, 0.125, 0.064), (x, 0.95, 0.064)], 0.0007 + 0.0002 * (5 - k), string_m, sides=3,
                     caps=False)
    st.data.transform(g)
    side = -1 if k < 3 else 1
    peg = common.cylinder("Tuning peg", Vector((side * 0.05, 1.0 + (k % 3) * 0.04, 0.03)), 0.006, 0.03, "x", chrome,
                          segments=8)
    peg.data.transform(g)
# The stand: two legs, a back support, a neck yoke.
for side in (-1, 1):
    common.tube("Stand leg", [(GX + side * 0.12, Y, GZ + 0.12), (GX, Y + 0.65, GZ - 0.05)], 0.008, black, sides=6)
common.tube("Stand back", [(GX - 0.2, Y, GZ - 0.25), (GX, Y + 0.65, GZ - 0.05)], 0.008, black, sides=6)

# ---- Combo amp against the front wall ----
AX, AZ = -6.95, L.Z1 - 0.22
common.box("Amp", (AX - 0.3, Y, AZ - 0.13), (AX + 0.3, Y + 0.48, AZ + 0.13), tolex, 0.015, 3)
common.box("Amp grille", (AX - 0.27, Y + 0.03, AZ - 0.132), (AX + 0.27, Y + 0.36, AZ - 0.126), grille, 0.004)
common.box("Amp panel", (AX - 0.28, Y + 0.38, AZ - 0.135), (AX + 0.28, Y + 0.46, AZ - 0.126), cream, 0.004)
for k in range(6):
    common.cylinder("Amp knob", Vector((AX - 0.2 + k * 0.07, Y + 0.42, AZ - 0.145)), 0.011, 0.02, "z", black, segments=10)
common.tube("Amp handle", [(AX - 0.1, Y + 0.48, AZ), (AX - 0.08, Y + 0.52, AZ), (AX + 0.08, Y + 0.52, AZ),
                           (AX + 0.1, Y + 0.48, AZ)], 0.01, black, sides=6)
common.cylinder("Amp light", Vector((AX + 0.24, Y + 0.42, AZ - 0.14)), 0.006, 0.01, "z",
                common.plain_material("Amp jewel", (1, 0.2, 0.1), 0.0, 0.3, emission=(1, 0.15, 0.05), strength=4.0),
                segments=8)

# ---- Lighting bar with three PAR cans aimed at the piano ----
BX, BY = -6.3, L.H - 0.35
common.cylinder("Light bar", Vector((BX, BY, 5.4)), 0.025, 3.0, "z", black, segments=10)
for z in (4.3, 6.5):
    common.cylinder("Bar drop", Vector((BX, (BY + L.H) / 2, z)), 0.012, L.H - BY, "y", black, segments=8)
for z, target in ((4.6, (-7.2, 1.0, 5.2)), (5.4, (-7.6, 1.0, 5.6)), (6.2, (-7.0, 1.0, 6.0))):
    aim = Vector(target) - Vector((BX, BY - 0.15, z))
    rot = aim.to_track_quat("-Z", "Y").to_matrix().to_4x4()
    m = Matrix.Translation((BX, BY - 0.15, z)) @ rot
    can = common.cylinder("PAR can", Vector((0, 0, 0.02)), 0.09, 0.26, "z", black, segments=16)
    can.data.transform(m)
    ln = common.cylinder("PAR lens", Vector((0, 0, -0.112)), 0.075, 0.004, "z", lens, segments=16)
    ln.data.transform(m)
    yoke = common.tube("PAR yoke", [(BX, BY, z), (BX, BY - 0.15, z)], 0.01, black, sides=6)

# ---- A bar stool by the mic, cables ----
SX, SZ = -6.8, 6.35
common.cylinder("Stool seat", Vector((SX, Y + 0.74, SZ)), 0.17, 0.05, "y", tolex, segments=20)
for k in range(4):
    a = k * math.pi / 2 + 0.4
    common.tube("Stool leg", [(SX + 0.1 * math.cos(a), Y + 0.72, SZ + 0.1 * math.sin(a)),
                              (SX + 0.2 * math.cos(a), Y, SZ + 0.2 * math.sin(a))], 0.012, chrome, sides=6)
common.torus("Stool ring", Vector((SX, Y + 0.28, SZ)), 0.16, 0.008, chrome, segments=20, sides=5)
common.tube("Cable", [(AX + 0.2, Y + 0.1, AZ - 0.1), (-6.6, Y + 0.004, 6.4), (-5.9, Y + 0.004, 6.0),
                      (-5.45, Y + 0.004, 5.6)], 0.006, black, sides=5)

common.finish_and_export(out_path, uv_scale={"Amp grille": 6.0, "Amp tolex": 3.0})
