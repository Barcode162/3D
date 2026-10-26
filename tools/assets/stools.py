# Builds the lower table stools (four splayed black legs, tan leather seat).
# The bar stools are Poly Haven's since the remake (J35.2: build_all.sh).
#
#   blender -b --factory-startup --python tools/assets/stools.py -- <texture dir> <out dir>
#
# Each stool stands on its origin (center of the base on the floor), y up.
import math
import os
import sys

from mathutils import Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402

tex_dir, out_dir = sys.argv[sys.argv.index("--") + 1:][:2]


def cushion(name, mat, r, y0, y1, segments=48):
    """A padded round seat: flat underside, rounded rim, softly domed top."""
    h = y1 - y0
    profile = [(0.0, y0), (r - 0.03, y0), (r - 0.008, y0 + 0.12 * h), (r, y0 + 0.35 * h),
               (r - 0.002, y0 + 0.6 * h), (r - 0.012, y0 + 0.82 * h), (r - 0.035, y0 + 0.95 * h),
               (r * 0.55, y1 - 0.004), (0.0, y1)]
    obj = common.lathe(name, profile, mat, segments)
    obj["uv_scale"] = 3.0
    return obj


def table_stool():
    common.reset_scene()
    black = common.plain_material("Black steel", (0.025, 0.025, 0.025), 0.7, 0.4)
    brass = common.plain_material("Brass", (0.91, 0.70, 0.38), 1.0, 0.3)
    leather = common.pbr_material("Tan leather", tex_dir, "fabric_leather_02", tint=(0.75, 0.6, 0.5),
                                  roughness_scale=0.8, size=512)
    piping = common.plain_material("Leather piping", (0.09, 0.045, 0.02), 0.0, 0.45)

    seat_y = 0.46
    # Four splayed legs from under the seat to the floor, with brass feet.
    for k in range(4):
        a = math.pi / 4 + k * math.pi / 2
        d = Vector((math.cos(a), 0, math.sin(a)))
        top = Vector((0, seat_y - 0.01, 0)) + d * 0.12
        foot = Vector((0, 0.02, 0)) + d * 0.2
        common.tube("Leg", [foot, top], 0.013, black, sides=12)
        common.cylinder("Foot", foot - Vector((0, 0.01, 0)), 0.017, 0.02, "y", brass, segments=16)
    # Rung ring between the legs.
    ring_y = 0.18
    r_ring = 0.12 + (0.2 - 0.12) * (seat_y - 0.01 - ring_y) / (seat_y - 0.03)
    common.torus("Rung", Vector((0, ring_y, 0)), r_ring, 0.009, black, "y", 40, 10)
    common.lathe("Seat plate", [(0.0, seat_y - 0.015), (0.15, seat_y - 0.015), (0.155, seat_y),
                                (0.0, seat_y)], black, 40)
    cushion("Cushion", leather, 0.185, seat_y - 0.004, seat_y + 0.075)
    common.torus("Piping", Vector((0, seat_y + 0.058, 0)), 0.16, 0.005, piping, "y", 48, 8)
    common.finish_and_export(os.path.join(out_dir, "table_stool.glb"), uv_scale={})


table_stool()
