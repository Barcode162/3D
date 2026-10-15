# Builds the bar's glassware as .glb files (lathed profiles, real wall
# thickness, see-through glass material).
#
#   blender -b --factory-startup --python tools/assets/glasses.py -- <out dir>
#
# Each glass stands on its origin (bottom center), y up.
import math
import os
import sys


sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402

out_dir = sys.argv[sys.argv.index("--") + 1]


def rounded_rim(r_out, r_in, y, steps=4):
    """Points over a half-round rim from the outer wall to the inner wall."""
    mid = (r_out + r_in) / 2
    rad = (r_out - r_in) / 2
    return [(mid + rad * math.cos(math.pi * s / steps), y + rad * math.sin(math.pi * s / steps))
            for s in range(steps + 1)]


def export(name, build):
    common.reset_scene()
    build(common.glass_material())
    common.finish_and_export(os.path.join(out_dir, name + ".glb"), uv_scale={})


def pint(mat):
    # A shaker pint: 15 cm tall, tapering from 6 cm to 8.6 cm across.
    h, base, top, wall, floor = 0.150, 0.030, 0.043, 0.0028, 0.012
    profile = [(0.0, 0.0), (base - 0.002, 0.0), (base, 0.002), (base + 0.0005, 0.008)]
    profile += rounded_rim(top, top - wall, h - wall / 2)
    profile += [(base + 0.0005 - wall, floor + 0.004), (base - wall - 0.004, floor), (0.0, floor)]
    common.lathe("Pint glass", profile, mat)


def rocks(mat):
    # A heavy-based rocks (old fashioned) glass: 9 cm tall, 8 cm across.
    h, r, wall, floor = 0.090, 0.040, 0.0035, 0.018
    profile = [(0.0, 0.0), (r - 0.003, 0.0), (r, 0.003)]
    profile += rounded_rim(r, r - wall, h - wall / 2)
    profile += [(r - wall, floor + 0.004), (r - wall - 0.004, floor), (0.0, floor)]
    common.lathe("Rocks glass", profile, mat)


export("pint_glass", pint)
export("rocks_glass", rocks)
