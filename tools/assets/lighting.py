# Builds the light fixtures: the pendant lamps over the bar (enamel dome
# shade, Edison bulb), the neon "BAR" sign on the back wall, and the table
# candles (glass votive, wax candle, flame).
#
#   blender -b --factory-startup --python tools/assets/lighting.py -- <texture dir> <out dir>
import math
import os
import sys

from mathutils import Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402

tex_dir, out_dir = sys.argv[sys.argv.index("--") + 1:][:2]


def pendant():
    """Origin: where the cord meets the ceiling; the lamp hangs down (-y)."""
    common.reset_scene()
    steel = common.plain_material("Shade steel", (0.03, 0.035, 0.03), 0.8, 0.35)
    enamel = common.plain_material("Shade enamel", (0.9, 0.85, 0.75), 0.0, 0.4)
    brass = common.plain_material("Brass", (0.91, 0.70, 0.38), 1.0, 0.3)
    cord = common.plain_material("Cloth cord", (0.04, 0.03, 0.025), 0.0, 0.85)
    bulb = common.plain_material("Edison bulb", (1.0, 0.75, 0.45), 0.0, 0.1, emission=(1.0, 0.62, 0.3),
                                 strength=10.0)
    filament = common.plain_material("Filament", (1.0, 0.8, 0.5), 0.0, 0.3, emission=(1.0, 0.85, 0.55),
                                     strength=20.0)
    common.lathe("Canopy", [(0.0, -0.03), (0.02, -0.03), (0.065, -0.012), (0.07, 0.0), (0.0, 0.0)], steel, 32)
    common.cylinder("Cord", Vector((0, -0.34, 0)), 0.0045, 0.62, "y", cord, segments=8)
    common.lathe("Socket", [(0.0, -0.73), (0.022, -0.73), (0.024, -0.68), (0.016, -0.64), (0.012, -0.645),
                            (0.0, -0.645)], brass, 20)
    # Dome shade: outer steel skin and an enamel lining, joined at the rim.
    outer = [(0.0, -0.63), (0.04, -0.63), (0.07, -0.645), (0.1, -0.675), (0.14, -0.73), (0.18, -0.8),
             (0.21, -0.86), (0.222, -0.875)]
    inner = [(0.217, -0.872), (0.205, -0.858), (0.175, -0.8), (0.135, -0.735), (0.095, -0.682), (0.065, -0.655),
             (0.035, -0.642), (0.0, -0.642)]
    common.lathe("Shade", outer + [(0.224, -0.878)], steel, 40)
    common.lathe("Lining", [(0.224, -0.878)] + inner, enamel, 40)
    common.torus("Rim", Vector((0, -0.876, 0)), 0.222, 0.004, brass, "y", 48, 6)
    # Bulb: a pear-shaped globe below the socket, with a glowing filament.
    common.lathe("Bulb", [(0.0, -0.73), (0.016, -0.73), (0.03, -0.76), (0.046, -0.8), (0.048, -0.84),
                          (0.04, -0.875), (0.02, -0.89), (0.0, -0.893)], bulb, 24)
    common.tube("Filament", [Vector((-0.012, -0.79, 0)), Vector((-0.006, -0.82, 0.004)), Vector((0, -0.79, 0)),
                             Vector((0.006, -0.82, -0.004)), Vector((0.012, -0.79, 0))], 0.0025, filament, sides=6)
    common.finish_and_export(os.path.join(out_dir, "pendant_lamp.glb"), uv_scale={})


# Neon letters as strokes: each a list of (x, y) points in a 0..1 box
# (width 0.7 of the height), drawn as glowing tubes.
def _letter_strokes(ch):
    arcs = lambda cx, cy, r, a0, a1, n=10: [(cx + r * math.cos(a0 + (a1 - a0) * k / n),
                                            cy + r * math.sin(a0 + (a1 - a0) * k / n)) for k in range(n + 1)]
    if ch == "B":
        return [[(0, 0), (0, 1)],
                [(0, 1), (0.42, 1)] + arcs(0.42, 0.76, 0.24, math.pi / 2, -math.pi / 2)[1:] + [(0, 0.52)],
                [(0, 0.52), (0.45, 0.52)] + arcs(0.45, 0.26, 0.26, math.pi / 2, -math.pi / 2)[1:] + [(0, 0)]]
    if ch == "A":
        return [[(0, 0), (0.35, 1), (0.7, 0)], [(0.12, 0.36), (0.58, 0.36)]]
    if ch == "R":
        return [[(0, 0), (0, 1)],
                [(0, 1), (0.42, 1)] + arcs(0.42, 0.75, 0.25, math.pi / 2, -math.pi / 2)[1:] + [(0, 0.5)],
                [(0.3, 0.5), (0.7, 0)]]
    raise ValueError(ch)


def neon_sign():
    """Origin: center of the sign's back panel (against the wall); front +z."""
    common.reset_scene()
    panel = common.plain_material("Smoked acrylic", (0.015, 0.012, 0.014), 0.0, 0.08)
    chrome = common.plain_material("Chrome", (0.9, 0.9, 0.92), 1.0, 0.1)
    pink = common.plain_material("Neon pink", (0.12, 0.02, 0.06), 0.0, 0.2, emission=(1.0, 0.03, 0.4), strength=1.3)
    cyan = common.plain_material("Neon cyan", (0.02, 0.08, 0.12), 0.0, 0.2, emission=(0.0, 0.5, 1.0), strength=1.3)
    amber = common.plain_material("Neon amber", (0.12, 0.06, 0.01), 0.0, 0.2, emission=(1.0, 0.38, 0.0), strength=1.2)
    green = common.plain_material("Neon green", (0.04, 0.1, 0.02), 0.0, 0.2, emission=(0.15, 1.0, 0.05), strength=1.0)
    W, H = 2.2, 0.5
    common.box("Panel", (-W / 2, -H / 2, 0.0), (W / 2, H / 2, 0.012), panel, 0.004)
    for (x, y) in ((-W / 2 + 0.05, -H / 2 + 0.05), (W / 2 - 0.05, -H / 2 + 0.05), (-W / 2 + 0.05, H / 2 - 0.05),
                   (W / 2 - 0.05, H / 2 - 0.05)):
        common.cylinder("Standoff", Vector((x, y, 0.016)), 0.008, 0.012, "z", chrome, segments=10)
    z = 0.045
    r = 0.011
    # Rounded-rectangle frame in pink.
    fw, fh, cr = W / 2 - 0.05, H / 2 - 0.04, 0.06
    frame = []
    for (cx, cy, a0) in ((fw - cr, fh - cr, 0), (-fw + cr, fh - cr, math.pi / 2), (-fw + cr, -fh + cr, math.pi),
                         (fw - cr, -fh + cr, 3 * math.pi / 2)):
        frame += [Vector((cx + cr * math.cos(a0 + math.pi / 2 * k / 6), cy + cr * math.sin(a0 + math.pi / 2 * k / 6),
                          z)) for k in range(7)]
    common.tube("Frame", frame, r * 0.8, pink, sides=8, closed=True)
    # "BAR" in cyan block letters.
    h, gap = 0.3, 0.07
    widths = {"B": 0.7, "A": 0.7, "R": 0.7}
    total = sum(widths[c] * h for c in "BAR") + gap * 2
    x = -total / 2
    for ch in "BAR":
        for stroke in _letter_strokes(ch):
            common.tube("Letter", [Vector((x + px * h, -h / 2 + py * h, z)) for (px, py) in stroke], r, cyan,
                        sides=8)
        x += widths[ch] * h + gap
    # A martini glass on each side: amber glass, green olive.
    for side in (-1, 1):
        cx = side * 0.72
        glass = [(-0.11, 0.12), (0.0, -0.02), (0.11, 0.12), (-0.11, 0.12)]
        common.tube("Martini", [Vector((cx + px, py, z)) for (px, py) in glass], r * 0.8, amber, sides=8)
        common.tube("Stem", [Vector((cx, -0.02, z)), Vector((cx, -0.13, z))], r * 0.8, amber, sides=8)
        common.tube("Foot", [Vector((cx - 0.06, -0.135, z)), Vector((cx + 0.06, -0.135, z))], r * 0.8, amber, sides=8)
        olive = [Vector((cx + 0.025 + 0.022 * math.cos(a), 0.075 + 0.022 * math.sin(a), z + 0.004))
                 for a in (2 * math.pi * k / 12 for k in range(12))]
        common.tube("Olive", olive, r * 0.7, green, sides=6, closed=True)
        common.tube("Pick", [Vector((cx - 0.02, 0.03, z)), Vector((cx + 0.07, 0.15, z))], 0.003, chrome, sides=6)
    common.finish_and_export(os.path.join(out_dir, "neon_sign.glb"), uv_scale={})


def candle():
    """Origin: bottom center of the votive glass on the table."""
    common.reset_scene()
    glass = common.glass_material("Amber votive", (0.95, 0.6, 0.25), alpha=0.35, roughness=0.08)
    wax = common.plain_material("Wax", (0.9, 0.82, 0.68), 0.0, 0.6, emission=(0.35, 0.18, 0.06), strength=1.0)
    wick = common.plain_material("Wick", (0.02, 0.02, 0.02), 0.0, 0.9)
    flame = common.plain_material("Flame", (1.0, 0.7, 0.3), 0.0, 0.5, emission=(1.0, 0.55, 0.15), strength=8.0)
    core = common.plain_material("Flame core", (1.0, 0.9, 0.7), 0.0, 0.5, emission=(1.0, 0.9, 0.65), strength=12.0)
    h, r, wall = 0.075, 0.034, 0.003
    profile = [(0.0, 0.0), (r - 0.003, 0.0), (r, 0.003), (r + 0.002, h * 0.5), (r, h - 0.002),
               (r - wall / 2, h), (r - wall, h - 0.002), (r - wall + 0.001, h * 0.5), (r - wall, 0.008),
               (0.0, 0.008)]
    common.lathe("Votive", profile, glass, 28)
    common.cylinder("Candle", Vector((0, 0.008 + 0.0225, 0)), r - wall - 0.004, 0.045, "y", wax, segments=20)
    common.cylinder("Wick", Vector((0, 0.058, 0)), 0.0012, 0.01, "y", wick, segments=6)
    common.lathe("Flame", [(0.0, 0.059), (0.006, 0.064), (0.0075, 0.072), (0.005, 0.082), (0.0015, 0.092),
                           (0.0, 0.096)], flame, 12)
    common.lathe("Flame core", [(0.0, 0.0605), (0.003, 0.064), (0.0035, 0.07), (0.0015, 0.076), (0.0, 0.078)],
                 core, 8)
    common.finish_and_export(os.path.join(out_dir, "candle.glb"), uv_scale={})


pendant()
neon_sign()
candle()
