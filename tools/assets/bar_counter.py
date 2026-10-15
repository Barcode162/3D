# Builds the bar counter in Blender and exports it as a .glb.
#
#   blender -b --factory-startup --python tools/assets/bar_counter.py -- <texture dir> <out.glb>
#
# The texture dir holds Poly Haven (CC0) 1k JPEGs: wood_table_001_* (bar top)
# and dark_wood_* (front panels), each _diff / _rough / _nor_gl.
#
# Layout (meters, model space): x runs along the bar (7 m long, centered),
# y is up from the floor, +z faces the customers. The scene places it at the
# old blockout's spot.
import math
import os
import sys

import bmesh
import bpy
from mathutils import Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402

tex_dir, out_path = sys.argv[sys.argv.index("--") + 1:][:2]
common.reset_scene()

LENGTH = 7.0
HALF = LENGTH / 2

# Materials. Wood textures are tinted/adjusted into new images first, so the
# glTF file carries exactly what the engine should show.
top_wood = common.wood_material(
    "Varnished mahogany", tex_dir, "wood_table_001", tint=(1.0, 0.92, 0.85), roughness_scale=0.45,
    roughness_bias=0.04, grain="v")
panel_wood = common.wood_material(
    "Stained oak panels", tex_dir, "dark_wood", tint=(1.0, 0.9, 0.85), roughness_scale=0.7, grain="u")
brass = common.plain_material("Brass", (0.91, 0.70, 0.38), metallic=1.0, roughness=0.28)
kick = common.plain_material("Black kick plate", (0.02, 0.02, 0.02), metallic=0.0, roughness=0.6)


def box(name, x0, x1, y0, y1, z0, z1, mat, bevel=0.006, segments=2, grain_axis="x"):
    obj = common.box(name, (x0, y0, z0), (x1, y1, z1), mat, bevel, segments)
    obj["grain_axis"] = grain_axis
    return obj


# Core body (behind the paneling).
box("Body", -HALF, HALF, 0.10, 1.00, -0.35, 0.30, panel_wood, bevel=0.003)
# Recessed toe kick and the plinth molding above it.
box("Toe kick", -HALF + 0.02, HALF - 0.02, 0.0, 0.10, -0.33, 0.24, kick, bevel=0.002)
box("Plinth", -HALF - 0.02, HALF + 0.02, 0.10, 0.17, -0.37, 0.37, panel_wood, bevel=0.012, segments=3)

# Front paneling: rails (horizontal) and stiles (vertical) form a frame,
# with a raised, beveled field in each opening.
FRONT = 0.34
box("Bottom rail", -HALF, HALF, 0.17, 0.29, 0.29, FRONT, panel_wood, bevel=0.008)
box("Top rail", -HALF, HALF, 0.86, 0.98, 0.29, FRONT, panel_wood, bevel=0.008)
panels = 7
stile_w = 0.11
opening = (LENGTH - stile_w * (panels + 1)) / panels
for i in range(panels + 1):
    x = -HALF + i * (opening + stile_w)
    box(f"Stile {i}", x, x + stile_w, 0.29, 0.86, 0.29, FRONT, panel_wood, bevel=0.008, grain_axis="y")
for i in range(panels):
    x0 = -HALF + stile_w + i * (opening + stile_w)
    # Thin back board, then the raised field with a wide, soft bevel.
    box(f"Panel back {i}", x0, x0 + opening, 0.29, 0.86, 0.29, 0.31, panel_wood, bevel=0.0)
    box(f"Panel field {i}", x0 + 0.05, x0 + opening - 0.05, 0.34, 0.81, 0.30, 0.33, panel_wood,
        bevel=0.022, segments=3, grain_axis="y")

# Crown molding under the top: two stepped, rounded boards.
box("Crown lower", -HALF - 0.01, HALF + 0.01, 0.98, 1.01, -0.36, 0.37, panel_wood, bevel=0.010, segments=3)
box("Crown upper", -HALF - 0.03, HALF + 0.03, 1.01, 1.04, -0.38, 0.40, panel_wood, bevel=0.012, segments=3)

# The bar top: a thick slab with a rounded front edge, overhanging the front.
box("Bar top", -HALF - 0.15, HALF + 0.15, 1.04, 1.10, -0.42, 0.52, top_wood, bevel=0.022, segments=5)
# Raised drink rail along the customer edge (a soft, rounded molding).
box("Drink rail", -HALF - 0.15, HALF + 0.15, 1.10, 1.135, 0.40, 0.52, top_wood, bevel=0.016, segments=5)

# Brass foot rail on brackets, with ball ends.
RAIL_Z, RAIL_Y, RAIL_R = FRONT + 0.17, 0.20, 0.025
common.cylinder("Foot rail", Vector((0, RAIL_Y, RAIL_Z)), RAIL_R, LENGTH - 0.2, "x", brass, segments=32)
for end in (-1, 1):
    common.sphere("Rail end", Vector((end * (HALF - 0.1), RAIL_Y, RAIL_Z)), RAIL_R * 1.25, brass)
brackets = 6
for i in range(brackets):
    x = -HALF + 0.35 + i * (LENGTH - 0.7) / (brackets - 1)
    depth = RAIL_Z - FRONT
    common.cylinder("Bracket arm", Vector((x, RAIL_Y, FRONT + depth / 2)), 0.012, depth, "z", brass, segments=16)
    common.cylinder("Bracket flange", Vector((x, RAIL_Y, FRONT + 0.004)), 0.035, 0.008, "z", brass, segments=24)
    common.sphere("Bracket saddle", Vector((x, RAIL_Y, RAIL_Z)), RAIL_R * 1.15, brass)

# Bartender side: a plain back with an under-bar shelf.
box("Back shelf", -HALF + 0.05, HALF - 0.05, 0.55, 0.58, -0.55, -0.35, panel_wood)

common.finish_and_export(out_path, uv_scale={"Varnished mahogany": 0.9, "Stained oak panels": 0.8})
