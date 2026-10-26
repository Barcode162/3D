# The darts corner around the existing dartboard cabinet (front wall,
# docs/archive/BAR_REDESIGN.md): three darts stuck in the board, a black foam
# surround, a wall holder with three more darts, the brass oche line at the
# regulation 2.37 m, a rubber throwing mat with distance marks, a track
# with two spotlights aimed at the board, and a neon "DARTS" sign.
#
#   blender -b --factory-startup --python tools/assets/darts_zone.py -- <texture dir> <out dir> [<things.json>]
#
# Since J73 each piece is a thing of its own (common.group): the darts in the
# board, the surround, the holder, the oche and mat, the spotlights, the neon.
#
# Model space = world space. The board's center is at (BX, BY), its face at
# z = FACE_Z (bar/bar_scene.cpp places dartboard.glb at x = 6.0 on the
# front wall).
import math
import os
import sys

import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402
import layout as L  # noqa: E402

import json  # noqa: E402

args = sys.argv[sys.argv.index("--") + 1:]
tex_dir, out_dir = args[:2]
things_path = args[2] if len(args) > 2 else None
common.reset_scene()

BX, BY = 6.0, 1.73
FACE_Z = L.Z1 - 0.04
OCHE_Z = FACE_Z - 2.37

tungsten = common.plain_material("Tungsten", (0.45, 0.45, 0.48), 1.0, 0.35)
tip_steel = common.plain_material("Dart tip", (0.8, 0.8, 0.82), 1.0, 0.2)
flight_cols = [common.plain_material(f"Flight {i}", c, 0.0, 0.4) for i, c in enumerate(
    [(0.8, 0.05, 0.05), (0.05, 0.25, 0.8), (0.95, 0.75, 0.05)])]
shaft_m = common.plain_material("Dart shaft", (0.05, 0.05, 0.05), 0.2, 0.35)
foam = common.plain_material("Surround foam", (0.02, 0.02, 0.02), 0.0, 0.9)
brass = common.plain_material("Oche brass", (0.91, 0.70, 0.38), 1.0, 0.25)
black = common.plain_material("Track black", (0.02, 0.02, 0.02), 0.3, 0.4)
lens = common.plain_material("Spot lens", (1.0, 0.95, 0.85), 0.0, 0.3, emission=(1.0, 0.85, 0.6), strength=10.0)
holder_wood = common.pbr_material("Holder wood", tex_dir, "dark_wood", tint=(0.6, 0.45, 0.38), size=256)
neon_red = common.plain_material("Neon red", (1.0, 0.25, 0.15), 0.0, 0.3, emission=(1.0, 0.15, 0.08), strength=2.0)

# Throwing mat with distance marks.
mat_px = np.zeros((256, 64, 3), np.float32)
mat_px[:] = common.srgb((0.06, 0.06, 0.065))
for y, label in ((int(256 * (1 - 2.37 / 2.8)), "237"), (int(256 * (1 - 2.44 / 2.8)), "244")):
    mat_px[y:y + 2, 4:60] = common.srgb((0.9, 0.9, 0.9))
    common.draw_text(mat_px, label, 32, y + 14, 1, common.srgb((0.9, 0.9, 0.9)), center=True)
mat_px[:, :2] = mat_px[:, -2:] = common.srgb((0.8, 0.1, 0.1))
throw_mat = common.image_material("Throw mat", mat_px, roughness=0.8)


def dart(tip, direction, flight):
    """A dart with its point at `tip`, pointing along `direction` (into the
    board); built along +z then turned."""
    d = Vector(direction).normalized()
    rot = d.to_track_quat("Z", "Y").to_matrix().to_4x4()
    m = Matrix.Translation(tip) @ rot
    # Local: the point at z = 0, the dart sticks out toward -z.
    parts = [
        common.lathe("Dart point", [(0, 0.0), (0.0012, -0.001), (0.0012, -0.028), (0, -0.028)], tip_steel, 8),
        common.lathe("Dart barrel", [(0, -0.026), (0.0035, -0.028), (0.0042, -0.045), (0.004, -0.065), (0.003, -0.072),
                                     (0, -0.072)], tungsten, 12),
        common.lathe("Dart shaft", [(0, -0.072), (0.0022, -0.072), (0.0022, -0.11), (0, -0.11)], shaft_m, 8),
    ]
    for o in parts:
        for v in o.data.vertices:  # lathe builds along y; lay it along z
            v.co = Vector((v.co.x, -v.co.z, v.co.y))
        o.data.transform(m)
    for k in range(4):  # flights: four thin wings
        a = k * math.pi / 2
        q = common.quad("Flight", [(0, 0, -0.1), (0.016, 0, -0.112), (0.018, 0, -0.14), (0, 0, -0.142)], flight)
        q.data.transform(m @ Matrix.Rotation(a, 4, "Z"))
        q2 = common.quad("Flight", [(0, 0, -0.142), (0.018, 0, -0.14), (0.016, 0, -0.112), (0, 0, -0.1)], flight)
        q2.data.transform(m @ Matrix.Rotation(a, 4, "Z"))


# Three darts in the board (a decent throw: treble 20 and two near it).
common.group("board_darts", (BX, BY, FACE_Z), 180)
for k, (dx, dy, tilt) in enumerate(((0.002, 0.103, 0.06), (-0.012, 0.09, -0.04), (0.02, 0.06, 0.08))):
    tip = Vector((BX + dx, BY + dy, FACE_Z - 0.004))
    dart(tip, (math.sin(tilt) * 0.3, -0.18 + tilt, 1.0), flight_cols[k])

# Black foam surround ring behind the board.
common.group("dart_surround", (BX, BY, L.Z1), 180)
common.lathe("Surround", [(0.23, 0.0), (0.45, 0.0), (0.45, 0.03), (0.23, 0.03)], foam, 48).data.transform(
    Matrix.Translation((BX, BY, FACE_Z + 0.002)) @ Matrix.Rotation(math.pi / 2, 4, "X"))

# A wall holder with three spare darts, beside the cabinet.
HX, HY = BX + 0.72, 1.35
common.group("dart_holder", (HX, HY, L.Z1), 180)
common.box("Dart holder", (HX - 0.07, HY - 0.04, L.Z1 - 0.03), (HX + 0.07, HY + 0.04, L.Z1), holder_wood, 0.006)
for k in range(3):
    dart(Vector((HX - 0.04 + k * 0.04, HY + 0.035, L.Z1 - 0.015)), (0.05 * (k - 1), -1.0, 0.05), flight_cols[k])

# Oche: a brass strip set in the floor, and the throwing mat.
common.group("oche", (BX, 0.0, OCHE_Z), 180)
common.box("Oche", (BX - 0.5, 0.0, OCHE_Z - 0.04), (BX + 0.5, 0.008, OCHE_Z), brass, 0.002)
q = common.quad("Throw mat", [(BX - 0.32, 0.004, FACE_Z - 0.05), (BX + 0.32, 0.004, FACE_Z - 0.05),
                              (BX + 0.32, 0.004, FACE_Z - 2.85), (BX - 0.32, 0.004, FACE_Z - 2.85)], throw_mat,
                uvs=((0, 1), (1, 1), (1, 0), (0, 0)))

# Track with two spotlights on the ceiling, aimed at the board.
TZ = FACE_Z - 1.4
common.group("dart_spotlights", (BX, L.H, TZ), 180)
common.box("Track", (BX - 0.6, L.H - 0.04, TZ - 0.02), (BX + 0.6, L.H, TZ + 0.02), black, 0.004)
for sx in (-0.35, 0.35):
    aim = Vector((BX, BY, FACE_Z)) - Vector((BX + sx, L.H - 0.15, TZ))
    rot = aim.to_track_quat("-Z", "Y").to_matrix().to_4x4()
    m = Matrix.Translation((BX + sx, L.H - 0.15, TZ)) @ rot
    can = common.cylinder("Spot can", Vector((0, 0, 0.0)), 0.05, 0.16, "z", black, segments=16)
    can.data.transform(m)
    lns = common.cylinder("Spot lens", Vector((0, 0, -0.081)), 0.04, 0.004, "z", lens, segments=16)
    lns.data.transform(m)
    common.box("Spot stem", (BX + sx - 0.01, L.H - 0.15, TZ - 0.01), (BX + sx + 0.01, L.H - 0.04, TZ + 0.01), black,
               0.002)

# Neon "DARTS" above the cabinet. It faces the room (-z), so its letters run
# toward -x to read left to right (they ran toward +x: it read backwards,
# the user's screenshots, KI-45).
common.group("neon_darts", (BX, 0.0, L.Z1), 180)
glyphs = common._glyphs()
s = 0.04
x0 = BX + len("DARTS") * 6 * s / 2
for i, ch in enumerate("DARTS"):
    for r, row in enumerate(glyphs[ch]):
        for c, dot in enumerate(row):
            if dot == "#":
                x = x0 - (i * 6 + c) * s
                y = 2.55 - r * s
                common.box("Neon", (x - s * 0.32, y - s * 0.32, L.Z1 - 0.06), (x + s * 0.32, y + s * 0.32, L.Z1 - 0.04),
                           neon_red, 0.006)

placed = common.finish_and_export_groups(out_dir, None, uv_scale={"Holder wood": 2.0})
if things_path:
    with open(things_path, "w") as f:
        json.dump(placed, f)
