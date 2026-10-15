# Builds the bar room's shell: plank floor, brick walls, plaster ceiling with
# dark wood beams and crown molding, wainscot paneling with a chair rail,
# baseboards, and the entrance door with its frame.
#
#   blender -b --factory-startup --python tools/assets/room.py -- <texture dir> <out.glb>
#
# Model space = world space: the room's interior runs x -6..6, z -5..5,
# floor at y = 0, ceiling at 3.2. The two mirrors hang on the right wall
# (x = 6) at z = -2.2 and 1.1, so the wainscot leaves room for them.
import os
import sys

from mathutils import Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402

tex_dir, out_path = sys.argv[sys.argv.index("--") + 1:][:2]
common.reset_scene()

HX, HZ, H = 6.0, 5.0, 3.2
T = 0.2  # wall thickness

floor = common.pbr_material("Oak floor", tex_dir, "wood_floor", tint=(0.55, 0.42, 0.33), roughness_scale=0.8,
                            grain="v")
brick = common.pbr_material("Brick", tex_dir, "red_brick_03", tint=(0.85, 0.78, 0.74), roughness_scale=1.0,
                            normal_strength=1.4)
plaster = common.pbr_material("Ceiling plaster", tex_dir, "plastered_wall_02", tint=(0.42, 0.36, 0.31),
                              roughness_scale=1.0, size=512)
beam = common.pbr_material("Beam wood", tex_dir, "dark_wood", tint=(0.35, 0.28, 0.24), roughness_scale=0.9,
                           grain="u", size=512)
panel = common.pbr_material("Wainscot oak", tex_dir, "dark_wood", tint=(0.6, 0.5, 0.45), roughness_scale=0.6,
                            grain="u", size=512)
door_wood = common.pbr_material("Door wood", tex_dir, "wood_table_001", tint=(0.7, 0.6, 0.55),
                                roughness_scale=0.6, grain="v", size=512)
brass = common.plain_material("Brass", (0.91, 0.70, 0.38), 1.0, 0.3)
glass = common.plain_material("Door window", (0.02, 0.025, 0.03), 0.0, 0.05,
                              emission=(0.10, 0.12, 0.16), strength=1.0)  # night street glow


def box(name, lo, hi, mat, bevel=0.004, segments=2, grain_axis="x"):
    obj = common.box(name, lo, hi, mat, bevel, segments)
    obj["grain_axis"] = grain_axis
    return obj


# Floor and ceiling slabs.
box("Floor", (-HX, -0.1, -HZ), (HX, 0.0, HZ), floor, bevel=0, grain_axis="z")
box("Ceiling", (-HX, H, -HZ), (HX, H + 0.1, HZ), plaster, bevel=0)
# Brick walls (their inner faces are the room's walls).
box("Back wall", (-HX - T, 0, -HZ - T), (HX + T, H, -HZ), brick, bevel=0)
box("Front wall", (-HX - T, 0, HZ), (HX + T, H, HZ + T), brick, bevel=0)
box("Left wall", (-HX - T, 0, -HZ), (-HX, H, HZ), brick, bevel=0, grain_axis="z")
box("Right wall", (HX, 0, -HZ), (HX + T, H, HZ), brick, bevel=0, grain_axis="z")

# Ceiling beams running front to back, between the pendant lamps (x = 0, ±2.4).
for x in (-5.0, -3.4, -1.2, 1.2, 3.4, 5.0):
    box("Beam", (x - 0.11, H - 0.24, -HZ), (x + 0.11, H, HZ), beam, bevel=0.01, grain_axis="z")
# One cross beam over the bar.
box("Cross beam", (-HX, H - 0.2, -2.0), (HX, H, -1.82), beam, bevel=0.01, grain_axis="x")

# Crown molding where walls meet the ceiling (two stepped strips per wall).
for (lo, hi, ax) in [((-HX, H - 0.09, -HZ), (HX, H, -HZ + 0.05), "x"),
                     ((-HX, H - 0.09, HZ - 0.05), (HX, H, HZ), "x"),
                     ((-HX, H - 0.09, -HZ), (-HX + 0.05, H, HZ), "z"),
                     ((HX - 0.05, H - 0.09, -HZ), (HX, H, HZ), "z")]:
    box("Crown", lo, hi, beam, bevel=0.012, segments=3, grain_axis=ax)

# Wainscot: a run of raised panels with a chair rail and baseboard, along
# a wall segment. `wall` = "left" (x = -6, facing +x), "right" (x = 6) or
# "front" (z = 5, facing -z); a..b is the span along the wall.
PANEL_H = 1.0


def wainscot(wall, a, b):
    def piece(name, u0, u1, y0, y1, d0, d1, bevel, segments=2, vertical=False):
        # u runs along the wall, d is the distance out from the wall.
        if wall == "left":
            lo, hi, ax = (-HX + d0, y0, u0), (-HX + d1, y1, u1), "z"
        elif wall == "right":
            lo, hi, ax = (HX - d1, y0, u0), (HX - d0, y1, u1), "z"
        else:
            lo, hi, ax = (u0, y0, HZ - d1), (u1, y1, HZ - d0), "x"
        return box(name, lo, hi, panel, bevel, segments, "y" if vertical else ax)

    piece("Wainscot back", a, b, 0.0, PANEL_H, 0.0, 0.015, 0.0)
    piece("Baseboard", a, b, 0.0, 0.16, 0.0, 0.03, 0.006)
    piece("Base cap", a, b, 0.155, 0.175, 0.0, 0.036, 0.008, 3)
    piece("Lower rail", a, b, 0.175, 0.26, 0.0, 0.025, 0.004)
    piece("Upper rail", a, b, PANEL_H - 0.1, PANEL_H - 0.04, 0.0, 0.025, 0.004)
    piece("Chair rail", a, b, PANEL_H - 0.04, PANEL_H, 0.0, 0.05, 0.012, 3)
    n = max(1, round((b - a) / 0.75))
    stile = 0.07
    w = (b - a - stile * (n + 1)) / n
    for i in range(n + 1):
        u = a + i * (w + stile)
        piece("Stile", u, u + stile, 0.26, PANEL_H - 0.1, 0.0, 0.025, 0.004, vertical=True)
    for i in range(n):
        u = a + stile + i * (w + stile)
        piece("Field", u + 0.035, u + w - 0.035, 0.30, PANEL_H - 0.14, 0.0, 0.03, 0.014, 3, vertical=True)


def baseboard(wall, a, b):
    """Just a baseboard (where there's no paneling, e.g. the brick back wall)."""
    if wall == "back":
        box("Baseboard", (a, 0, -HZ), (b, 0.16, -HZ + 0.03), panel, 0.006)
        box("Base cap", (a, 0.155, -HZ), (b, 0.175, -HZ + 0.036), panel, 0.008, 3)
    elif wall == "right":
        box("Baseboard", (HX - 0.03, 0, a), (HX, 0.16, b), panel, 0.006, grain_axis="z")
        box("Base cap", (HX - 0.036, 0.155, a), (HX, 0.175, b), panel, 0.008, 3, grain_axis="z")


wainscot("left", -HZ, HZ)
# Right wall: around the two mirrors (frames span z -2.95..-1.45 and 0.35..1.85).
wainscot("right", -HZ + 0.6, -3.0)
wainscot("right", -1.4, 0.3)
wainscot("right", 1.9, HZ)
baseboard("right", -HZ, -HZ + 0.6)  # behind the jukebox
baseboard("right", -3.0, -1.4)
baseboard("right", 0.3, 1.9)
# Front wall: either side of the door (x -4.15..-2.85).
wainscot("front", -HX, -4.25)
wainscot("front", -2.75, HX)
baseboard("back", -HX, HX)

# Entrance door: frame (casing), a panelled door with a window, push plate
# and a brass handle. The door's room-side face is at z = 4.96.
DX, DW, DH = -3.5, 1.0, 2.15
z_face = HZ - 0.04
box("Casing left", (DX - DW / 2 - 0.12, 0, HZ - 0.035), (DX - DW / 2, DH + 0.12, HZ), panel, 0.01, 3, "y")
box("Casing right", (DX + DW / 2, 0, HZ - 0.035), (DX + DW / 2 + 0.12, DH + 0.12, HZ), panel, 0.01, 3, "y")
box("Casing head", (DX - DW / 2 - 0.15, DH, HZ - 0.045), (DX + DW / 2 + 0.15, DH + 0.16, HZ), panel, 0.012, 3)
box("Door", (DX - DW / 2, 0.0, z_face), (DX + DW / 2, DH, HZ - 0.005), door_wood, 0.006, 2, "y")
# Raised panels on the door's lower half, a window in the top half.
for (x0, x1) in ((DX - 0.4, DX - 0.04), (DX + 0.04, DX + 0.4)):
    box("Door panel", (x0, 0.18, z_face - 0.015), (x1, 1.0, z_face), door_wood, 0.012, 3, "y")
box("Window frame", (DX - 0.38, 1.15, z_face - 0.02), (DX + 0.38, 1.95, z_face), door_wood, 0.008, 2, "y")
box("Window", (DX - 0.32, 1.21, z_face - 0.022), (DX + 0.32, 1.89, z_face - 0.016), glass, 0.0)
for x in (DX - 0.107, DX + 0.107):  # muntins
    box("Muntin", (x - 0.012, 1.21, z_face - 0.03), (x + 0.012, 1.89, z_face - 0.02), door_wood, 0.004, 1, "y")
box("Muntin", (DX - 0.32, 1.538, z_face - 0.03), (DX + 0.32, 1.562, z_face - 0.02), door_wood, 0.004, 1)
box("Kick plate", (DX - DW / 2 + 0.03, 0.02, z_face - 0.004), (DX + DW / 2 - 0.03, 0.17, z_face), brass, 0.002)
# Handle: a vertical pull bar on two posts.
hx = DX + 0.38
for y in (0.92, 1.18):
    common.cylinder("Pull post", Vector((hx, y, z_face - 0.03)), 0.009, 0.06, "z", brass, segments=12)
common.cylinder("Pull bar", Vector((hx, 1.05, z_face - 0.06)), 0.014, 0.36, "y", brass, segments=16)
box("Push plate", (hx - 0.05, 1.3, z_face - 0.003), (hx + 0.05, 1.55, z_face), brass, 0.003)

common.finish_and_export(out_path, uv_scale={
    "Oak floor": 0.55, "Brick": 0.75, "Ceiling plaster": 0.3, "Beam wood": 0.8, "Wainscot oak": 1.0,
    "Door wood": 1.0, "Brass": 1.0, "Door window": 1.0})
