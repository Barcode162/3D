# Chess pieces for the lounge's playable chessboard (upgrade plan item 13):
# Staunton-style pieces turned on a lathe, a carved knight head, a cross on
# the king, a crown on the queen, felt under each. White is ivory-coloured
# wood, black is ebony. One model per piece (placed and moved by bar/projects/chess/chess.cpp).
#
#   blender -b --factory-startup --python tools/assets/chess.py -- <texture dir> <out dir>
#
# Model space: the piece's base centred at the origin, y up, +z toward white.
# Sized for 0.055 m squares (the king is 0.09 m tall).
import math
import os
import sys

import bmesh  # noqa: F401
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402

tex_dir, out_dir = sys.argv[sys.argv.index("--") + 1:][:2]
S = 0.055 / 0.045  # the old board's square was 0.045 m

PROFILES = {
    "pawn": [(0, 0), (0.012, 0), (0.0125, 0.003), (0.011, 0.005), (0.009, 0.007), (0.0065, 0.009), (0.0045, 0.02),
             (0.006, 0.022), (0.0058, 0.0235), (0.0045, 0.0245), (0.0068, 0.028), (0.0072, 0.031), (0.0065, 0.0345),
             (0.004, 0.0375), (0.0, 0.0385)],
    "rook": [(0, 0), (0.0135, 0), (0.014, 0.003), (0.012, 0.006), (0.0095, 0.009), (0.0078, 0.012), (0.0072, 0.033),
             (0.0098, 0.0355), (0.0105, 0.038), (0.0105, 0.046), (0.0075, 0.046), (0.0075, 0.043), (0.0, 0.043)],
    "bishop": [(0, 0), (0.0135, 0), (0.014, 0.003), (0.012, 0.006), (0.009, 0.009), (0.0062, 0.013), (0.0048, 0.033),
               (0.0075, 0.035), (0.0072, 0.037), (0.0052, 0.038), (0.0072, 0.043), (0.0076, 0.048), (0.0062, 0.053),
               (0.0035, 0.057), (0.0022, 0.0585), (0.0028, 0.0605), (0.0015, 0.0625), (0.0, 0.063)],
    "queen": [(0, 0), (0.0145, 0), (0.015, 0.003), (0.013, 0.006), (0.0095, 0.01), (0.0065, 0.015), (0.0052, 0.044),
              (0.0085, 0.046), (0.0082, 0.048), (0.006, 0.049), (0.0072, 0.058), (0.0092, 0.064), (0.0068, 0.066),
              (0.0, 0.066)],
    "king": [(0, 0), (0.0145, 0), (0.015, 0.003), (0.013, 0.006), (0.0095, 0.01), (0.0065, 0.015), (0.0055, 0.048),
             (0.009, 0.05), (0.0087, 0.052), (0.0062, 0.053), (0.0075, 0.062), (0.0088, 0.069), (0.0062, 0.071),
             (0.0, 0.071)],
    "knight": [(0, 0), (0.0135, 0), (0.014, 0.003), (0.012, 0.006), (0.0095, 0.009), (0.0085, 0.012), (0.0, 0.012)],
}


def knight_head(m):
    """The horse's head and neck: a side outline extruded across, bevelled."""
    out = [(-0.009, 0.012), (0.008, 0.012), (0.01, 0.022), (0.009, 0.03), (0.0115, 0.038), (0.0125, 0.046),
           (0.0085, 0.05), (0.004, 0.0545), (-0.002, 0.056), (-0.0045, 0.06), (-0.006, 0.055), (-0.011, 0.051),
           (-0.0165, 0.046), (-0.0175, 0.041), (-0.0135, 0.039), (-0.008, 0.041), (-0.0045, 0.037), (-0.008, 0.028),
           (-0.0105, 0.02)]
    bm = bmesh.new()
    w = 0.0055
    front = [bm.verts.new((-w, y, x)) for x, y in out]
    back = [bm.verts.new((w, y, x)) for x, y in out]
    bm.faces.new(front)
    bm.faces.new(list(reversed(back)))
    n = len(out)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((front[i], front[j], back[j], back[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    o = common.mesh_object("Knight head", bm, m)
    b = o.modifiers.new("bevel", "BEVEL")
    b.width = 0.0022
    b.segments = 3
    b.limit_method = "ANGLE"
    o.modifiers.new("smooth", "SUBSURF").levels = 1
    # Eyes and the mane's ridge.
    return o


def build(kind, color, out_path):
    common.reset_scene()
    # Ivory-white and ebony-black, lacquered (wood grain made the white ones
    # look brown in the bar's warm light).
    wood = common.plain_material("Ivory" if color == "w" else "Ebony",
                                 (0.95, 0.9, 0.78) if color == "w" else (0.035, 0.03, 0.028), 0.0, 0.28)
    felt = common.plain_material("Felt", (0.08, 0.25, 0.1), 0.0, 1.0)
    body = common.lathe("Piece", [(r * S, y * S) for r, y in PROFILES[kind]], wood, 32)
    body["smooth"] = True
    common.cylinder("Felt", Vector((0, 0.0004, 0)), 0.0128 * S if kind == "pawn" else 0.0138 * S, 0.0008, "y", felt,
                    segments=24, bevel=0.0)
    if kind == "knight":
        head = knight_head(wood)
        head.data.transform(Matrix.Diagonal((S, S, S, 1)))
        head.rotation_euler = (0, 0, 0)
    if kind == "rook":  # crenellations: four notches in the top rim
        for k in range(4):
            a = k * math.pi / 2 + math.pi / 4
            notch = common.box("Notch", (-0.0018 * S, 0.0405 * S, 0.0068 * S), (0.0018 * S, 0.0465 * S, 0.0112 * S),
                               common.plain_material("Shadow", (0.02, 0.015, 0.01), 0.0, 0.8), 0.0)
            notch.data.transform(Matrix.Rotation(a, 4, "Y"))
    if kind == "king":  # a cross on top
        common.box("Cross", (-0.0012 * S, 0.071 * S, -0.0012 * S), (0.0012 * S, 0.083 * S, 0.0012 * S), wood, 0.0004)
        common.box("Cross bar", (-0.0042 * S, 0.0765 * S, -0.0012 * S), (0.0042 * S, 0.079 * S, 0.0012 * S), wood, 0.0004)
    if kind == "queen":  # a ring of little crown balls
        for k in range(8):
            a = k * math.tau / 8
            common.sphere("Crown", Vector((math.cos(a) * 0.0078 * S, 0.0655 * S, math.sin(a) * 0.0078 * S)), 0.0016 * S,
                          wood, 10, 6)
        common.sphere("Crown top", Vector((0, 0.0685 * S, 0)), 0.0028 * S, wood, 12, 8)
    if kind == "bishop":  # the mitre's slit
        common.box("Slit", (-0.0062 * S, 0.046 * S, -0.0008 * S), (0.0062 * S, 0.0535 * S, 0.0008 * S),
                   common.plain_material("Shadow", (0.02, 0.015, 0.01), 0.0, 0.8), 0.0).data.transform(
            Matrix.Rotation(0.6, 4, "X"))
    common.finish_and_export(out_path, uv_scale={})


os.makedirs(out_dir, exist_ok=True)
for kind in PROFILES:
    for color in ("w", "b"):
        build(kind, color, os.path.join(out_dir, f"chess_{kind}_{color}.glb"))
