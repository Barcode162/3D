# "Under construction" at the two doorways to the rooms still being built
# (docs/BAR_REDESIGN.md): crossed caution tape across each arch, a plastic
# sheet hanging behind it, striped sawhorse barricades, traffic cones, a
# sign on an easel, and inside the unfinished room a work light on a
# tripod, a step ladder, paint buckets, a drop cloth, lumber, drywall
# sheets and a toolbox.
#
#   blender -b --factory-startup --python tools/assets/construction.py -- <texture dir> <out.glb>
#
# Model space = world space.
import math
import os
import random
import sys

import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402
import layout as L  # noqa: E402

tex_dir, out_path = sys.argv[sys.argv.index("--") + 1:][:2]
common.reset_scene()
rng = random.Random(2)

# ---- Materials ----
tape_px = np.zeros((32, 512, 3), np.float32)
for x in range(512):
    stripe = ((x // 24) % 2) == 0
    tape_px[:, x] = common.srgb((0.98, 0.82, 0.05)) if stripe else common.srgb((0.04, 0.04, 0.04))
tape_px[6:26, :] = common.srgb((0.98, 0.82, 0.05))
for k in range(3):
    common.draw_text(tape_px, "CAUTION", 85 + k * 170, 24, 2, common.srgb((0.03, 0.03, 0.03)), center=True)
# A touch of glow: real tape is retro-reflective and pops even in a dim bar.
tape = common.glow_image_material("Caution tape", tape_px, strength=0.35, base_scale=1.0, roughness=0.35)
stripes_px = np.zeros((64, 256, 3), np.float32)
for x in range(256):
    for y in range(64):
        stripes_px[y, x] = common.srgb((0.95, 0.35, 0.03)) if ((x + y) // 32) % 2 == 0 else common.srgb((0.95, 0.95, 0.92))
stripes = common.image_material("Barricade stripes", stripes_px, roughness=0.4)
orange = common.plain_material("Cone orange", (0.95, 0.28, 0.02), 0.0, 0.45)
white_band = common.plain_material("Reflective white", (0.95, 0.95, 0.95), 0.0, 0.3)
black = common.plain_material("Black rubber", (0.03, 0.03, 0.03), 0.0, 0.7)
plastic = common.glass_material("Plastic sheet", color=(0.85, 0.9, 0.95), alpha=0.35, roughness=0.3)
steel = common.plain_material("Painted steel", (0.12, 0.12, 0.12), 0.6, 0.45)
alu = common.plain_material("Aluminum", (0.75, 0.76, 0.78), 1.0, 0.35)
yellow = common.plain_material("Tool yellow", (0.95, 0.7, 0.05), 0.0, 0.4)
lamp = common.plain_material("Work light", (1.0, 0.95, 0.85), 0.0, 0.3, emission=(1.0, 0.92, 0.75), strength=14.0)
lumber = common.pbr_material("Lumber", tex_dir, "plywood", tint=(1.0, 0.85, 0.65), size=256)
drywall = common.plain_material("Drywall sheet", (0.75, 0.73, 0.68), 0.0, 0.9)
cloth = common.pbr_material("Drop cloth", tex_dir, "fabric_pattern_07", tint=(0.7, 0.68, 0.62), size=256,
                            saturation=0.1)
paint = [common.plain_material(f"Paint {i}", c, 0.0, 0.4) for i, c in enumerate(
    [(0.9, 0.9, 0.88), (0.12, 0.3, 0.2), (0.6, 0.1, 0.08)])]
sign_px = np.zeros((128, 192, 3), np.float32)
sign_px[:] = common.srgb((0.98, 0.8, 0.05))
sign_px[:6] = sign_px[-6:] = 0.02
sign_px[:, :6] = sign_px[:, -6:] = 0.02
for i, line in enumerate(("UNDER", "CONSTRUCTION", "", "NEW ROOM", "COMING SOON")):
    if line:
        common.draw_text(sign_px, line, 96, 116 - i * 22, 2, common.srgb((0.03, 0.03, 0.03)), center=True)
sign_mat = common.glow_image_material("Construction sign", sign_px, strength=0.3, base_scale=1.0, roughness=0.5)


def place(obj, m):
    obj.data.transform(m)
    return obj


def frame_for(origin, yaw):
    """World matrix for a doorway's local frame: x along the wall, z out
    into the bar, y up."""
    return Matrix.Translation(origin) @ Matrix.Rotation(yaw, 4, "Y")


def tape_strip(m, a, b, width=0.075):
    """A strip of caution tape between two local points (x, y, z)."""
    a, b = Vector(a), Vector(b)
    d = b - a
    ln = d.length
    q = common.quad("Tape", [(0, -width / 2, 0), (ln, -width / 2, 0), (ln, width / 2, 0), (0, width / 2, 0)], tape,
                    uvs=((0, 0), (ln / 1.6, 0), (ln / 1.6, 1), (0, 1)))
    back = common.quad("Tape", [(ln, -width / 2, -0.002), (0, -width / 2, -0.002), (0, width / 2, -0.002),
                                (ln, width / 2, -0.002)], tape, uvs=((0, 0), (ln / 1.6, 0), (ln / 1.6, 1), (0, 1)))
    ang = math.atan2(d.y, d.x)
    for o in (q, back):
        place(o, m @ Matrix.Translation(a) @ Matrix.Rotation(ang, 4, "Z"))


def sawhorse(m, x, z, yaw):
    mm = m @ Matrix.Translation((x, 0, z)) @ Matrix.Rotation(yaw, 4, "Y")
    rail = common.box("Barricade rail", (-0.6, 0.78, -0.012), (0.6, 0.98, 0.012), stripes, 0.004)
    common.set_uvs(rail, lambda co, n: ((co.x + 0.6) / 1.2 * 2, (co.y - 0.78) / 0.2 * 0.5))
    place(rail, mm)
    low = common.box("Barricade rail", (-0.6, 0.38, -0.012), (0.6, 0.52, 0.012), stripes, 0.004)
    common.set_uvs(low, lambda co, n: ((co.x + 0.6) / 1.2 * 2, (co.y - 0.38) / 0.14 * 0.35))
    place(low, mm)
    for sx in (-0.5, 0.5):
        for sz in (-1, 1):
            leg = common.box("Barricade leg", (-0.02, 0, -0.02), (0.02, 1.0, 0.02), steel, 0.004)
            place(leg, mm @ Matrix.Translation((sx, 0, 0)) @ Matrix.Rotation(sz * 0.22, 4, "X"))
    lamp_c = common.cylinder("Barricade lamp", Vector((0.0, 1.06, 0)), 0.05, 0.03, "z",
                             common.plain_material("Amber lamp", (1, 0.6, 0.1), 0.0, 0.3, emission=(1.0, 0.5, 0.05),
                                                   strength=5.0), segments=14)
    place(lamp_c, mm)


def cone(m, x, z, s=1.0):
    mm = m @ Matrix.Translation((x, 0, z)) @ Matrix.Diagonal((s, s, s, 1))
    place(common.box("Cone base", (-0.18, 0, -0.18), (0.18, 0.03, 0.18), black, 0.01), mm)
    place(common.lathe("Cone", [(0, 0.03), (0.13, 0.03), (0.025, 0.7), (0, 0.7)], orange, 20), mm)
    for (y0, y1) in ((0.3, 0.38), (0.48, 0.54)):
        r0 = 0.13 - (0.13 - 0.025) * (y0 - 0.03) / 0.67 + 0.003
        r1 = 0.13 - (0.13 - 0.025) * (y1 - 0.03) / 0.67 + 0.003
        place(common.lathe("Cone band", [(0, y0), (r0, y0), (r1, y1), (0, y1)], white_band, 20), mm)


def easel_sign(m, x, z, yaw):
    mm = m @ Matrix.Translation((x, 0, z)) @ Matrix.Rotation(yaw, 4, "Y")
    for sx in (-0.3, 0.3):
        leg = common.box("Easel leg", (-0.015, 0, -0.015), (0.015, 1.6, 0.015), lumber, 0.003)
        place(leg, mm @ Matrix.Translation((sx, 0, 0.1)) @ Matrix.Rotation(-0.12, 4, "X"))
    back = common.box("Easel back leg", (-0.015, 0, -0.015), (0.015, 1.5, 0.015), lumber, 0.003)
    place(back, mm @ Matrix.Translation((0, 0, -0.2)) @ Matrix.Rotation(0.2, 4, "X"))
    board = common.box("Sign board", (-0.42, 0.75, 0.0), (0.42, 1.31, 0.02), lumber, 0.004)
    place(board, mm @ Matrix.Translation((0, 0, 0.13)) @ Matrix.Rotation(-0.12, 4, "X"))
    face = common.quad("Sign face", [(-0.4, 0.77, 0.021), (0.4, 0.77, 0.021), (0.4, 1.29, 0.021), (-0.4, 1.29, 0.021)],
                       sign_mat)
    place(face, mm @ Matrix.Translation((0, 0, 0.13)) @ Matrix.Rotation(-0.12, 4, "X"))


def plastic_sheet(m, w, h, z):
    """A hanging plastic sheet with soft vertical folds."""
    import bmesh
    bm = bmesh.new()
    nx, ny = 24, 6
    grid = []
    for i in range(nx + 1):
        col = []
        for j in range(ny + 1):
            x = -w / 2 + w * i / nx
            y = h * j / ny
            col.append(bm.verts.new((x, y, z + 0.04 * math.sin(i * 1.3) * (0.4 + 0.6 * (1 - j / ny)))))
        grid.append(col)
    for i in range(nx):
        for j in range(ny):
            bm.faces.new([grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]])
    o = common.mesh_object("Plastic", bm, plastic, smooth=True)
    place(o, m)
    # Back face too (the sheet is seen from both sides).
    bm2 = bmesh.new()
    bm2.from_mesh(o.data)
    for f in bm2.faces:
        f.normal_flip()
    o2 = common.mesh_object("Plastic back", bm2, plastic, smooth=True)
    o2.data.transform(Matrix.Translation((0, 0, -0.003)))


def work_room(m, depth, width):
    """Things inside the unfinished room (local frame, z negative = deeper)."""
    # Work light on a tripod, aimed at the back wall.
    tx, tz = width * 0.28, -depth * 0.45
    for k in range(3):
        a = k * 2 * math.pi / 3
        leg = common.tube("Tripod leg", [(tx + 0.35 * math.cos(a), 0.0, tz + 0.35 * math.sin(a)), (tx, 1.2, tz)],
                          0.012, yellow, sides=6)
        place(leg, m)
    place(common.cylinder("Tripod mast", Vector((tx, 1.45, tz)), 0.015, 0.5, "y", yellow, segments=8), m)
    head = common.box("Work light head", (-0.14, -0.1, -0.05), (0.14, 0.1, 0.05), yellow, 0.01)
    place(head, m @ Matrix.Translation((tx, 1.75, tz)) @ Matrix.Rotation(0.6, 4, "Y") @ Matrix.Rotation(-0.3, 4, "X"))
    lens = common.box("Work light lens", (-0.12, -0.08, 0.05), (0.12, 0.08, 0.055), lamp, 0.0)
    place(lens, m @ Matrix.Translation((tx, 1.75, tz)) @ Matrix.Rotation(0.6, 4, "Y") @ Matrix.Rotation(-0.3, 4, "X"))
    # Step ladder (A-frame, aluminum).
    lx, lz = -width * 0.25, -depth * 0.6
    lm = m @ Matrix.Translation((lx, 0, lz)) @ Matrix.Rotation(0.4, 4, "Y")
    for side in (-1, 1):
        for sx in (-0.22, 0.22):
            rail = common.box("Ladder rail", (sx - 0.02, 0, -0.012), (sx + 0.02, 1.6, 0.012), alu, 0.003)
            place(rail, lm @ Matrix.Rotation(side * 0.18, 4, "X"))
    for k in range(4):
        step = common.box("Ladder step", (-0.22, 0.3 + k * 0.38, -0.06), (0.22, 0.33 + k * 0.38, 0.06), alu, 0.004)
        place(step, lm @ Matrix.Translation((0, 0, 0.18 - 0.05 * k)))
    place(common.box("Ladder top", (-0.25, 1.58, -0.1), (0.25, 1.64, 0.1), alu, 0.008), lm)
    # Paint buckets, one open with a stir stick.
    for k, (bx, bz) in enumerate(((0.1, -0.4), (0.4, -0.55), (0.25, -0.2))):
        bxx, bzz = bx * width, bz * depth
        place(common.lathe("Bucket", [(0, 0), (0.14, 0), (0.16, 0.36), (0, 0.36)], paint[k], 18),
              m @ Matrix.Translation((bxx, 0, bzz)))
        place(common.torus("Bucket handle", Vector((0, 0.36, 0)), 0.16, 0.004, steel, axis="z", segments=16, sides=4),
              m @ Matrix.Translation((bxx, 0, bzz)))
    # Drop cloth on the floor, lumber stack, drywall leaning on the wall, toolbox.
    dc = common.box("Drop cloth", (-0.9, 0.0, -0.7), (0.9, 0.01, 0.7), cloth, 0.004)
    place(dc, m @ Matrix.Translation((-width * 0.05, 0.002, -depth * 0.35)) @ Matrix.Rotation(0.2, 4, "Y"))
    for k in range(6):
        board = common.box("Lumber", (-1.2, 0, -0.045), (1.2, 0.04, 0.045), lumber, 0.003)
        place(board, m @ Matrix.Translation((-width * 0.3 + (k % 3) * 0.1, 0.01 + (k // 3) * 0.041, -depth * 0.25))
              @ Matrix.Rotation(1.45, 4, "Y"))
    for k in range(3):
        sheet = common.box("Drywall", (-0.6, 0, -0.006), (0.6, 2.4, 0.006), drywall, 0.002)
        place(sheet, m @ Matrix.Translation((width * 0.18 + k * 0.03, 0.0, -depth + 0.15 + k * 0.02)) @
              Matrix.Rotation(-0.08, 4, "X"))
    tb = m @ Matrix.Translation((width * 0.35, 0, -depth * 0.2))
    place(common.box("Toolbox", (-0.25, 0, -0.12), (0.25, 0.22, 0.12), common.plain_material(
        "Toolbox red", (0.7, 0.05, 0.03), 0.4, 0.4), 0.01), tb)
    place(common.tube("Toolbox handle", [(-0.12, 0.22, 0), (-0.1, 0.28, 0), (0.1, 0.28, 0), (0.12, 0.22, 0)], 0.01,
                      black, sides=6), tb)


def doorway(origin, yaw, width, name):
    """Everything for one doorway. origin = middle of the opening at the
    wall's room-side face; yaw turns local +z to point into the bar."""
    m = frame_for(origin, yaw)
    hw = width / 2
    top = L.ARCH_SPRING + hw * 0.8
    # Crossed tape plus two level strips, slightly sagging.
    tape_strip(m, (-hw - 0.05, 0.35, 0.03), (hw + 0.05, top, 0.03))
    tape_strip(m, (-hw - 0.05, top, 0.035), (hw + 0.05, 0.35, 0.035))
    tape_strip(m, (-hw - 0.05, 1.05, 0.04), (hw + 0.05, 1.0, 0.04))
    tape_strip(m, (-hw - 0.05, 1.6, 0.045), (hw + 0.05, 1.55, 0.045))
    plastic_sheet(m, width - 0.1, L.ARCH_SPRING + hw * 0.9, -0.35)
    sawhorse(m, -0.35, 0.6, 0.12)
    sawhorse(m, 0.55, 0.75, -0.2)
    cone(m, -hw - 0.35, 0.45)
    cone(m, hw + 0.4, 0.5, 0.85)
    cone(m, 1.15, 0.95, 0.9)
    easel_sign(m, -hw - 0.75, 0.55, 0.35)
    work_room(m @ Matrix.Translation((0, 0, -L.T)), L.CORRIDOR_DEPTH, width + 0.8)


doorway(Vector(((L.DOOR_A_X0 + L.DOOR_A_X1) / 2, 0, L.Z0)), 0.0, L.DOOR_A_X1 - L.DOOR_A_X0, "Room 2")
doorway(Vector((L.X0, 0, (L.DOOR_B_Z0 + L.DOOR_B_Z1) / 2)), math.pi / 2, L.DOOR_B_Z1 - L.DOOR_B_Z0, "Room 3")

common.finish_and_export(out_path, uv_scale={"Lumber": 1.5, "Drop cloth": 1.0})
