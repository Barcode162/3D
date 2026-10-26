# The arcade (upgrade plan item 3): the room behind the back wall's arched
# doorway, next to the 4K Mania corner. Kept almost empty on purpose: a
# glowing "cosmic" arcade carpet, dark walls with neon tubes and neon
# shapes, an LED grid on the ceiling, low glow strips along the floor, an
# "ARCADE" neon sign over the doorway (bar side) and one over the inside of
# it, and a padded bench. The machines are their own models
# (arcade_cabinet.py), placed by the app.
#
#   blender -b --factory-startup --python tools/assets/arcade.py -- <texture dir> <out.glb> [<things.json>]
#
# Since J73 the ARCADE signs, the neon shapes on the walls and the bench are
# things of their own (common.group, next to <out.glb>); <out.glb> keeps the
# shell, its neon lines and the ceiling.
#
# Model space = world space (like hall.py). The room: x AX0..AX1, z AZ0..AZ1
# (AZ1 = the back of the bar's back wall), floor y = 0, ceiling AH.
import math
import os
import random
import sys

import numpy as np
from mathutils import Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402
import layout as L  # noqa: E402

import json  # noqa: E402

args = sys.argv[sys.argv.index("--") + 1:]
tex_dir, out_path = args[:2]
things_path = args[2] if len(args) > 2 else None
common.reset_scene()
rng = random.Random(7)

AX0, AX1 = L.ARCADE_X0, L.ARCADE_X1
AZ0, AZ1 = L.ARCADE_Z0, L.ARCADE_Z1
AH = L.ARCADE_H
DX0, DX1 = L.DOOR_A_X0, L.DOOR_A_X1
DOOR_TOP = L.ARCH_SPRING + (DX1 - DX0) / 2  # top of the arch

NEON = {
    "pink": (1.0, 0.12, 0.62), "cyan": (0.1, 0.85, 1.0), "yellow": (1.0, 0.85, 0.1),
    "lime": (0.45, 1.0, 0.2), "purple": (0.6, 0.25, 1.0), "orange": (1.0, 0.45, 0.08),
}


# ---- The carpet: dark blue with neon squiggles, rings, triangles, bolts
# and dots (the classic arcade "cosmic" carpet), seamless, glowing a little
# like it does under black light. ----
def carpet(size=1024, seed=3):
    r = np.random.default_rng(seed)
    img = np.zeros((size, size, 3), np.float32)
    n = common.noise2d(size, size, 24, seed=seed, octaves=3)
    img[:] = np.stack([0.010 + 0.012 * n, 0.010 + 0.010 * n, 0.035 + 0.03 * n], -1)
    colors = [common.srgb(c) for c in NEON.values()]

    def stamp(x, y, rad, col):
        xi, yi = int(round(x)) % size, int(round(y)) % size
        rr = int(math.ceil(rad))
        for dy in range(-rr, rr + 1):
            for dx in range(-rr, rr + 1):
                if dx * dx + dy * dy <= rad * rad:
                    img[(yi + dy) % size, (xi + dx) % size] = col

    def line(points, rad, col):
        for (x0, y0), (x1, y1) in zip(points[:-1], points[1:]):
            steps = max(1, int(math.hypot(x1 - x0, y1 - y0) / max(rad * 0.6, 0.8)))
            for k in range(steps + 1):
                t = k / steps
                stamp(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, rad, col)

    for k in range(70):
        cx, cy = r.uniform(0, size), r.uniform(0, size)
        col = colors[k % len(colors)]
        kind = k % 6
        s = r.uniform(18, 40)
        a0 = r.uniform(0, math.tau)
        if kind == 0:  # ring
            pts = [(cx + s * math.cos(a), cy + s * math.sin(a)) for a in np.linspace(0, math.tau, 40)]
            line(pts, 3.2, col)
        elif kind == 1:  # triangle outline
            pts = [(cx + s * math.cos(a0 + i * math.tau / 3), cy + s * math.sin(a0 + i * math.tau / 3)) for i in range(4)]
            line(pts, 3.0, col)
        elif kind == 2:  # squiggle
            d = (math.cos(a0), math.sin(a0))
            pts = [(cx + d[0] * t + -d[1] * 9 * math.sin(t / 9), cy + d[1] * t + d[0] * 9 * math.sin(t / 9))
                   for t in np.linspace(-s * 2, s * 2, 50)]
            line(pts, 3.0, col)
        elif kind == 3:  # lightning bolt
            d = (math.cos(a0), math.sin(a0))
            zz = [(-1.2, 0), (-0.2, 0.35), (-0.4, -0.25), (1.2, 0.1)]
            pts = [(cx + (d[0] * u - d[1] * v) * s, cy + (d[1] * u + d[0] * v) * s) for u, v in zz]
            line(pts, 3.2, col)
        elif kind == 4:  # plus / star
            for a in (a0, a0 + math.pi / 2):
                line([(cx - math.cos(a) * s * 0.5, cy - math.sin(a) * s * 0.5),
                      (cx + math.cos(a) * s * 0.5, cy + math.sin(a) * s * 0.5)], 3.0, col)
        else:  # dots
            for i in range(5):
                stamp(cx + r.uniform(-s, s), cy + r.uniform(-s, s), r.uniform(3, 6), col)
    return img


carpet_m = common.glow_image_material("Arcade carpet", carpet(), strength=0.55, base_scale=0.5, roughness=0.95)
wall_m = common.plain_material("Arcade wall", (0.03, 0.022, 0.05), 0.0, 0.85)
wainscot_m = common.plain_material("Arcade wainscot", (0.012, 0.012, 0.018), 0.2, 0.5)
ceiling_m = common.plain_material("Arcade ceiling", (0.012, 0.010, 0.016), 0.0, 0.9)
trim_m = common.plain_material("Arcade trim", (0.06, 0.06, 0.07), 0.8, 0.3)
neon = {k: common.plain_material(f"Neon {k}", c, 0.0, 0.3, emission=c, strength=1.1) for k, c in NEON.items()}
bench_m = common.plain_material("Bench vinyl", (0.35, 0.04, 0.25), 0.0, 0.35)


def box(name, lo, hi, mat, bevel=0.0):
    return common.box(name, lo, hi, mat, bevel)


T = 0.12  # the arcade's own wall thickness
# ---- Shell: floor, ceiling, walls (the front wall has the doorway). ----
box("Arcade floor", (AX0, -0.1, AZ0), (AX1, 0.0, AZ1), carpet_m)
box("Arcade ceiling", (AX0, AH, AZ0), (AX1, AH + 0.1, AZ1), ceiling_m)
box("Arcade back wall", (AX0 - T, 0, AZ0 - T), (AX1 + T, AH + 0.1, AZ0), wall_m)
box("Arcade left wall", (AX0 - T, 0, AZ0), (AX0, AH + 0.1, AZ1), wall_m)
box("Arcade right wall", (AX1, 0, AZ0), (AX1 + T, AH + 0.1, AZ1), wall_m)
fz = AZ1 - 0.01  # just in front of the bar's brick (no z-fighting)
box("Arcade front wall", (AX0, 0, fz - T), (DX0, AH, fz), wall_m)
box("Arcade front wall", (DX1, 0, fz - T), (AX1, AH, fz), wall_m)
box("Arcade front wall", (DX0, DOOR_TOP, fz - T), (DX1, AH, fz), wall_m)
# The arch's reveal through our wall (the brick arch is the bar's).
for x0, x1 in ((DX0 - 0.02, DX0), (DX1, DX1 + 0.02)):
    box("Door reveal", (x0, 0, fz - T), (x1, L.ARCH_SPRING, fz), trim_m)
# Wainscot (dark, glossy) to 1 m with a neon line on top, all round.
for (x0, z0, x1, z1) in ((AX0, AZ0, AX1, AZ0 + 0.02), (AX0, AZ0, AX0 + 0.02, AZ1 - T), (AX1 - 0.02, AZ0, AX1, AZ1 - T)):
    box("Wainscot", (x0, 0, z0), (x1, 1.0, z1), wainscot_m)
# Floor glow strips (the floor line lights the carpet's edge) and the high
# neon band under the ceiling.
y_low, y_high, r = 0.06, AH - 0.25, 0.018
paths = {
    "back": [Vector((AX0 + 0.05, 0, AZ0 + 0.05)), Vector((AX1 - 0.05, 0, AZ0 + 0.05))],
    "left": [Vector((AX0 + 0.05, 0, AZ0 + 0.05)), Vector((AX0 + 0.05, 0, AZ1 - T - 0.05))],
    "right": [Vector((AX1 - 0.05, 0, AZ0 + 0.05)), Vector((AX1 - 0.05, 0, AZ1 - T - 0.05))],
}
for k, (side, pts) in enumerate(paths.items()):
    common.tube(f"Floor glow {side}", [p + Vector((0, y_low, 0)) for p in pts], r, neon["pink"], sides=8)
    common.tube(f"High neon {side}", [p + Vector((0, y_high, 0)) for p in pts], r, neon["cyan"], sides=8)
    common.tube(f"Wainscot neon {side}", [p + Vector((0, 1.0, 0)) for p in pts], 0.012, neon["purple"], sides=6)
# Inside the front wall too (either side of the door).
for x0, x1 in ((AX0 + 0.05, DX0 - 0.15), (DX1 + 0.15, AX1 - 0.05)):
    for y, m in ((y_low, "pink"), (y_high, "cyan"), (1.0, "purple")):
        common.tube("Front neon", [Vector((x0, y, fz - T - 0.04)), Vector((x1, y, fz - T - 0.04))], r if y != 1.0 else 0.012,
                    neon[m], sides=8)
# The door frame inside: a neon outline of the arch.
arch = [Vector((DX0 - 0.08, 0.05, fz - T - 0.03)), Vector((DX0 - 0.08, L.ARCH_SPRING, fz - T - 0.03))]
arch += [Vector((p.x, p.y, fz - T - 0.03)) for p in common.arc(((DX0 + DX1) / 2, L.ARCH_SPRING, 0), (DX1 - DX0) / 2 + 0.08,
                                                            math.pi, 0, 16)]
arch += [Vector((DX1 + 0.08, 0.05, fz - T - 0.03))]
common.tube("Door neon", arch, 0.02, neon["yellow"], sides=8)


# ---- Neon shapes on the walls (bent tubes, like real neon art). ----
def wall_shape(points2d, origin, right, up, scale, mat, name):
    o, rt, u = Vector(origin), Vector(right), Vector(up)
    # Its own thing, standing on the wall (its front: the wall's normal).
    n = rt.cross(u)
    common.group("arcade_neon_" + name.lower(), (o.x, 0.0, o.z), math.degrees(math.atan2(n.x, n.z)))
    common.tube(name, [o + rt * (x * scale) + u * (y * scale) for x, y in points2d], 0.02, mat, sides=8)


zmid = (AZ0 + AZ1) / 2
# Left wall: a big lightning bolt and a ring.
wall_shape([(-0.6, 0.7), (0.1, 0.1), (-0.15, 0.0), (0.6, -0.7), (-0.1, -0.1), (0.15, 0.0), (-0.6, 0.7)],
           (AX0 + 0.06, 2.0, zmid + 1.6), (0, 0, -1), (0, 1, 0), 0.8, neon["yellow"], "Bolt")
wall_shape([(math.cos(a), math.sin(a)) for a in np.linspace(0, math.tau, 33)],
           (AX0 + 0.06, 2.05, zmid - 1.5), (0, 0, -1), (0, 1, 0), 0.55, neon["pink"], "Ring")
# Right wall: a triangle and a zigzag.
wall_shape([(math.cos(a) * 0.7, math.sin(a) * 0.7) for a in (math.pi / 2, math.pi / 2 + math.tau / 3,
                                                               math.pi / 2 + 2 * math.tau / 3, math.pi / 2)],
           (AX1 - 0.06, 2.05, zmid + 1.5), (0, 0, 1), (0, 1, 0), 0.9, neon["cyan"], "Triangle")
wall_shape([(-1.2 + 0.4 * k, 0.25 if k % 2 else -0.25) for k in range(7)],
           (AX1 - 0.06, 2.0, zmid - 1.6), (0, 0, 1), (0, 1, 0), 0.8, neon["lime"], "Zigzag")
# Back wall (behind the machines): two long waves.
for k, (y, m) in enumerate(((2.55, "purple"), (2.75, "pink"))):
    wall_shape([(x, 0.08 * math.sin(x * 3 + k)) for x in np.linspace(-3.6, 3.6, 60)],
               ((AX0 + AX1) / 2, y, AZ0 + 0.06), (1, 0, 0), (0, 1, 0), 1.0, neon[m], "Wave")

common.group(None)
# ---- Ceiling: an LED grid (squares of light strips) and a ring. ----
cols, rows = 3, 3
for i in range(cols):
    for j in range(rows):
        cx = AX0 + (AX1 - AX0) * (i + 0.5) / cols
        cz = AZ0 + (AZ1 - AZ0) * (j + 0.5) / rows
        s = 0.8
        m = neon["cyan"] if (i + j) % 2 == 0 else neon["pink"]
        y = AH - 0.02
        sq = [Vector((cx - s, y, cz - s)), Vector((cx + s, y, cz - s)), Vector((cx + s, y, cz + s)),
              Vector((cx - s, y, cz + s)), Vector((cx - s, y, cz - s))]
        common.tube("LED square", sq, 0.016, m, sides=6)
common.torus("Ceiling ring", Vector(((AX0 + AX1) / 2, AH - 0.35, zmid)), 1.1, 0.025, neon["purple"], axis="y",
             segments=48, sides=8)
for a in (0, math.pi / 2, math.pi, 1.5 * math.pi):
    common.cylinder("Ring wire", Vector(((AX0 + AX1) / 2 + 1.1 * math.cos(a), AH - 0.17, zmid + 1.1 * math.sin(a))),
                    0.004, 0.34, "y", trim_m, segments=6)


# ---- "ARCADE" neon signs: over the doorway on the bar side, and inside. ----
def sign_image(text, w=512, h=128):
    px = np.zeros((h, w, 3), np.float32)
    px[:] = (0.004, 0.002, 0.01)
    col = common.srgb(NEON["pink"])
    scale = 12
    tw = common.text_width(text, scale)
    # A soft halo first (bigger, dimmer), then the bright letters.
    for off, k in ((2, 0.18), (1, 0.35)):
        for dx in (-off, 0, off):
            for dy in (-off, 0, off):
                common.draw_text(px, text, w // 2 + dx * 3, (h + 7 * scale) // 2 + dy * 3, scale, tuple(c * k for c in col),
                                 center=True)
    common.draw_text(px, text, w // 2, (h + 7 * scale) // 2, scale, col, center=True)
    # Cyan border tube.
    cy = common.srgb(NEON["cyan"])
    px[4:8, 6:-6] = px[-8:-4, 6:-6] = cy
    px[6:-6, 4:8] = px[6:-6, -8:-4] = cy
    del tw
    return px


sign_m = common.glow_image_material("Arcade sign", sign_image("ARCADE"), strength=3.0, base_scale=0.3)
sx = (DX0 + DX1) / 2
# Bar side: on the brick above the arch (faces +z, into the bar).
common.group("arcade_sign_bar", (sx, 0.0, L.Z0), 0.0)
common.panel("Arcade sign", (sx, DOOR_TOP + 0.38, L.Z0 + 0.025), (1, 0, 0), (0, 1, 0), 1.6, 0.4, sign_m)
box("Sign backer", (sx - 0.84, DOOR_TOP + 0.16, L.Z0), (sx + 0.84, DOOR_TOP + 0.6, L.Z0 + 0.02), wainscot_m)
# Inside, over the door (faces -z, into the arcade).
common.group("arcade_sign_inside", (sx, 0.0, fz - T), 180.0)
common.panel("Arcade sign", (sx, DOOR_TOP + 0.2, fz - T - 0.02), (-1, 0, 0), (0, 1, 0), 1.4, 0.35, sign_m)

# ---- A padded bench along the left wall (somewhere to sit and watch). ----
bx0, bx1 = AX0 + 0.1, AX0 + 0.6
bz0, bz1 = zmid - 1.0, zmid + 1.0
common.group("arcade_bench", ((bx0 + bx1) / 2, 0.0, zmid), 90.0)
box("Bench base", (bx0, 0, bz0), (bx1, 0.38, bz1), wainscot_m, 0.01)
box("Bench pad", (bx0 + 0.01, 0.38, bz0 + 0.01), (bx1 - 0.01, 0.46, bz1 - 0.01), bench_m, 0.03)
common.tube("Bench glow", [Vector((bx1 + 0.005, 0.03, bz0)), Vector((bx1 + 0.005, 0.03, bz1))], 0.012, neon["cyan"], sides=6)

placed = common.finish_and_export_groups(os.path.dirname(out_path), os.path.basename(out_path)[:-4],
                                        uv_scale={"Arcade carpet": 0.45})
if things_path:
    with open(things_path, "w") as f:
        json.dump(placed, f)
