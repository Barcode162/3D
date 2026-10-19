# The badges on player cards, modelled in 3D and rendered to pictures (the
# user wanted Clash Royale style badges instead of emojis): a chunky shield
# with a gold rim and glossy enamel, and a rounded, toy-like emblem on it.
# One PNG per badge (transparent background) into web/badges/.
#
#   blender -b --factory-startup --python tools/assets/badges.py -- <out dir> [size]
#
# Badges: owner (crown), bartender (cocktail), dj (headphones), vip (star),
# staff (wrench), and three for games: race (trophy), darts (dartboard),
# mania (music note).
import math
import os
import sys

import bpy
from mathutils import Matrix, Vector  # noqa: F401

sys.path.insert(0, os.path.dirname(__file__))

args = sys.argv[sys.argv.index("--") + 1:]
out_dir = args[0]
SIZE = int(args[1]) if len(args) > 1 else 256
os.makedirs(out_dir, exist_ok=True)


from toy import *  # noqa: E402,F401,F403


# ---- Emblems (in front of the shield, around (0, 0.05), z from 0.18) ----
Z = 0.17


def crown():
    g, red, blue = gold(), mat("Ruby", (0.9, 0.05, 0.1), 0, 0.1, coat=1), mat("Sapphire", (0.1, 0.3, 1.0), 0, 0.1, coat=1)
    out = [(-0.52, -0.3), (0.52, -0.3), (0.58, 0.32), (0.3, 0.05), (0.0, 0.42), (-0.3, 0.05), (-0.58, 0.32)]
    extrude("Crown", out, Z, Z + 0.16, g, 0.05)
    extrude("Crown band", [(-0.56, -0.42), (0.56, -0.42), (0.56, -0.24), (-0.56, -0.24)], Z + 0.1, Z + 0.24, g, 0.05)
    for x, y in ((-0.58, 0.36), (0.0, 0.46), (0.58, 0.36)):
        sphere("Crown ball", (x, y, Z + 0.1), 0.08, g)
    sphere("Jewel", (0, -0.33, Z + 0.26), 0.07, red)
    for x in (-0.33, 0.33):
        sphere("Jewel", (x, -0.33, Z + 0.25), 0.055, blue)


def cocktail():
    glass = mat("Glass", (0.9, 0.95, 1.0), 0, 0.05, coat=1, transmission=0.7)
    drink = mat("Drink", (1.0, 0.25, 0.55), 0, 0.15, coat=1, emit=(1.0, 0.2, 0.5), strength=0.6)
    c = (0, 0, Z + 0.25)
    lathe("Martini", [(0.0, -0.05), (0.05, -0.02), (0.56, 0.5), (0.6, 0.53), (0.0, 0.53)], glass, (0, 0.0, Z + 0.25))
    lathe("Drink", [(0.0, 0.02), (0.47, 0.42), (0.0, 0.42)], drink, (0, 0.0, Z + 0.25))
    tube("Stem", [(0, -0.05, c[2]), (0, -0.45, c[2])], 0.04, silver())
    lathe("Foot", [(0.0, -0.5), (0.26, -0.5), (0.26, -0.45), (0.04, -0.42), (0.0, -0.42)], silver(), (0, 0, Z + 0.25))
    sphere("Olive", (0.18, 0.38, Z + 0.45), 0.09, mat("Olive", (0.35, 0.6, 0.1), 0, 0.25, coat=1))
    tube("Pick", [(-0.1, 0.05, Z + 0.4), (0.38, 0.65, Z + 0.48)], 0.018, gold())


def headphones():
    dark, g = mat("Headphone black", (0.04, 0.04, 0.06), 0.2, 0.25, coat=1), gold()
    tube("Headband", arc_pts(0, -0.05, 0.5, math.radians(200), math.radians(-20), 40, Z + 0.1), 0.07, dark)
    for s in (-1, 1):
        cylinder_z("Cup", (s * 0.5, -0.22, Z + 0.12), 0.21, 0.2, dark)
        cylinder_z("Cup ring", (s * 0.5, -0.22, Z + 0.24), 0.14, 0.06, g)
        sphere("Cup light", (s * 0.5, -0.22, Z + 0.28), 0.06, mat("Glow pink", (1, 0.3, 0.7), 0, 0.2, emit=(1, 0.3, 0.7), strength=3))


def star():
    extrude("Star", star_outline(0.25, 0.6), Z, Z + 0.2, gold(), 0.08, 5)
    sphere("Star shine", (-0.12, 0.18, Z + 0.24), 0.05, mat("Shine", (1, 1, 1), 0, 0.1, emit=(1, 1, 0.9), strength=4))


def wrench():
    s = silver()
    tube("Handle", [(-0.42, -0.42, Z + 0.12), (0.14, 0.14, Z + 0.12)], 0.13, s)
    # The open jaw: a ring with a gap.
    tube("Jaw", arc_pts(0.3, 0.3, 0.24, math.radians(-15), math.radians(245), 30, Z + 0.12), 0.13, s)
    cylinder_z("Grip", (-0.42, -0.42, Z + 0.12), 0.13, 0.22, mat("Grip red", (0.9, 0.15, 0.1), 0, 0.3, coat=1))


def trophy():
    g = gold()
    lathe("Cup", [(0.0, -0.05), (0.12, -0.05), (0.38, 0.2), (0.44, 0.55), (0.4, 0.56), (0.34, 0.25), (0.0, 0.1)], g,
          (0, 0, Z + 0.25))
    for s in (-1, 1):
        tube("Handle", arc_pts(s * 0.44, 0.32, 0.17, math.radians(90 if s > 0 else 90), math.radians(-90 if s > 0 else 270), 16,
                               Z + 0.25), 0.04, g)
    tube("Stem", [(0, -0.05, Z + 0.25), (0, -0.35, Z + 0.25)], 0.06, g)
    extrude("Base", [(-0.3, -0.55), (0.3, -0.55), (0.24, -0.35), (-0.24, -0.35)], Z + 0.08, Z + 0.4, mat("Base wood", (0.25, 0.1, 0.05), 0, 0.4, coat=1), 0.04)


def dartboard():
    rings = [(0.58, (0.05, 0.05, 0.05)), (0.5, (0.95, 0.9, 0.75)), (0.36, (0.1, 0.6, 0.2)), (0.24, (0.95, 0.9, 0.75)),
             (0.12, (0.85, 0.08, 0.1))]
    for i, (r, c) in enumerate(rings):
        cylinder_z(f"Ring {i}", (0, 0, Z + 0.06 + 0.035 * i), r, 0.08, enamel(f"Board {i}", c))
    tube("Dart", [(0.08, 0.08, Z + 0.3), (0.5, 0.5, Z + 0.55)], 0.035, silver())
    extrude("Flight", [(0.45, 0.5), (0.62, 0.72), (0.66, 0.5), (0.62, 0.45)], Z + 0.5, Z + 0.56, enamel("Flight", (1, 0.4, 0.1)), 0.01)


def note():
    black = mat("Note", (0.06, 0.05, 0.1), 0.1, 0.2, coat=1)
    for x, y in ((-0.32, -0.38), (0.32, -0.25)):
        o = sphere("Head", (x, y, Z + 0.15), 0.18, black)
        o.scale = (1.25, 0.9, 0.7)
        tube("Stem", [(x + 0.2, y, Z + 0.15), (x + 0.2, y + 0.75, Z + 0.15)], 0.04, black)
    extrude("Beam", [(-0.14, 0.33), (0.54, 0.48), (0.54, 0.62), (-0.14, 0.47)], Z + 0.08, Z + 0.22, black, 0.03)


BADGES = {
    "owner": ((0.35, 0.1, 0.8), crown),
    "bartender": ((0.0, 0.55, 0.6), cocktail),
    "dj": ((0.85, 0.1, 0.55), headphones),
    "vip": ((0.85, 0.06, 0.1), star),
    "staff": ((0.95, 0.45, 0.05), wrench),
    "race": ((0.1, 0.35, 0.95), trophy),
    "darts": ((0.1, 0.55, 0.2), dartboard),
    "mania": ((1.0, 0.35, 0.7), note),
}


def build(kind):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    color, emblem = BADGES[kind]
    extrude("Rim", shield_outline(1.12), -0.14, 0.06, gold(), 0.07, 5)
    extrude("Face", shield_outline(1.0), 0.0, 0.14, enamel("Enamel", color), 0.06, 5)
    emblem()
    turn_all()
    render(os.path.join(out_dir, kind + ".png"), SIZE)
    print("BADGE", kind)


for k in (sys.argv[sys.argv.index("--") + 3:] if len(args) > 2 else BADGES):
    build(k)
