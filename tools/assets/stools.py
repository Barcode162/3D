# Builds the bar stools (chrome pedestal, foot ring, tufted oxblood leather
# seat with piping) and the lower table stools (four splayed black legs,
# tan leather seat).
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


def bar_stool():
    common.reset_scene()
    chrome = common.plain_material("Chrome", (0.9, 0.9, 0.92), 1.0, 0.08)
    black = common.plain_material("Black steel", (0.03, 0.03, 0.03), 0.6, 0.45)
    leather = common.pbr_material("Oxblood leather", tex_dir, "leather_red_02", tint=(0.9, 0.75, 0.75),
                                  roughness_scale=0.8, size=512)
    piping = common.plain_material("Leather piping", (0.12, 0.025, 0.02), 0.0, 0.45)

    # Weighted base: a low chrome dome.
    common.lathe("Base", [(0.0, 0.0), (0.205, 0.0), (0.21, 0.006), (0.205, 0.014), (0.17, 0.026),
                          (0.08, 0.038), (0.045, 0.046), (0.0, 0.046)], chrome, 48)
    common.cylinder("Column", Vector((0, 0.36, 0)), 0.026, 0.64, "y", chrome, segments=24)
    # Collar where the column meets the seat mount, and the mount itself.
    common.cylinder("Collar", Vector((0, 0.66, 0)), 0.036, 0.04, "y", black, segments=24)
    common.lathe("Seat mount", [(0.0, 0.68), (0.04, 0.68), (0.13, 0.698), (0.135, 0.708), (0.0, 0.708)], black, 32)
    # Foot ring on four spokes.
    ring_y = 0.31
    common.torus("Foot ring", Vector((0, ring_y, 0)), 0.19, 0.011, chrome, "y", 48, 12)
    for k in range(4):
        a = math.pi / 4 + k * math.pi / 2
        d = Vector((math.cos(a), 0, math.sin(a)))
        common.tube("Spoke", [Vector((0, ring_y, 0)) + d * 0.02, Vector((0, ring_y, 0)) + d * 0.185], 0.008,
                    chrome, sides=10)
    common.torus("Ring hub", Vector((0, ring_y, 0)), 0.03, 0.008, chrome, "y", 24, 8)
    # Seat: chrome pan, padded cushion, piping along the top seam, a
    # button in the middle (tufted look).
    common.lathe("Seat pan", [(0.0, 0.705), (0.175, 0.705), (0.185, 0.712), (0.185, 0.722), (0.0, 0.722)], chrome, 48)
    cushion("Cushion", leather, 0.2, 0.718, 0.82)
    common.torus("Piping", Vector((0, 0.80, 0)), 0.172, 0.0055, piping, "y", 48, 8)
    common.torus("Piping low", Vector((0, 0.735, 0)), 0.196, 0.0055, piping, "y", 48, 8)
    common.sphere("Button", Vector((0, 0.818, 0)), 0.012, piping, 12, 6)
    common.finish_and_export(os.path.join(out_dir, "bar_stool.glb"), uv_scale={})


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


bar_stool()
table_stool()
