# Builds the jukebox: a 1940s-style arched cabinet in lacquered wood with
# glowing color tubes along the arch, a domed window showing the record and
# turntable, a selector panel with title cards and buttons, a chrome speaker
# grille, and chrome trim.
#
#   blender -b --factory-startup --python tools/assets/jukebox.py -- <texture dir> <out.glb>
#
# Origin: center of the base on the floor; the front faces +z.
import math
import os
import sys

import bmesh
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402

tex_dir, out_path = sys.argv[sys.argv.index("--") + 1:][:2]
common.reset_scene()

HW = 0.45          # half width
ARCH_Y = 1.0       # where the straight sides turn into the arch
BACK, FRONT = -0.28, 0.25

wood = common.pbr_material("Jukebox walnut", tex_dir, "dark_wood", tint=(0.75, 0.42, 0.32), roughness_scale=0.3,
                           roughness_bias=0.02, grain="v", size=512)
chrome = common.plain_material("Chrome", (0.92, 0.92, 0.94), 1.0, 0.08)
dark = common.plain_material("Jukebox shadow", (0.012, 0.01, 0.01), 0.0, 0.7)
fabric = common.plain_material("Grille cloth", (0.16, 0.07, 0.03), 0.0, 0.9,
                               emission=(0.25, 0.09, 0.02), strength=1.0)  # lit from inside
# Glow strengths stay low: the tone mapper turns strong glow white.
tube_orange = common.plain_material("Glow orange", (0.2, 0.08, 0.02), 0.0, 0.3, emission=(1.0, 0.36, 0.04),
                                    strength=1.5)
tube_red = common.plain_material("Glow red", (0.2, 0.03, 0.05), 0.0, 0.3, emission=(1.0, 0.06, 0.2), strength=1.4)
tube_yellow = common.plain_material("Glow yellow", (0.2, 0.15, 0.05), 0.0, 0.3, emission=(1.0, 0.7, 0.2),
                                    strength=1.2)
vinyl = common.plain_material("Vinyl", (0.01, 0.01, 0.012), 0.0, 0.25)
record_label = common.plain_material("Record label", (0.6, 0.06, 0.05), 0.0, 0.5)
ivory = common.plain_material("Ivory buttons", (0.85, 0.8, 0.68), 0.0, 0.35)
window = common.glass_material("Dome glass", (0.9, 0.85, 0.75), alpha=0.1, roughness=0.03)


def arch_outline(hw, arch_y, bottom, steps=24):
    pts = [Vector((-hw, bottom, 0)), Vector((hw, bottom, 0))]
    pts += [Vector((hw * math.cos(a), arch_y + hw * math.sin(a), 0)) for a in
            (math.pi * k / steps for k in range(steps + 1))]
    return pts


def extrude(name, outline, z0, z1, mat, bevel=0.0):
    """A flat shape in the x-y plane, made solid between z0 and z1."""
    bm = bmesh.new()
    front = [bm.verts.new((p.x, p.y, z1)) for p in outline]
    back = [bm.verts.new((p.x, p.y, z0)) for p in outline]
    bm.faces.new(front)
    bm.faces.new(list(reversed(back)))
    n = len(outline)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new([back[i], back[j], front[j], front[i]])
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    obj = common.mesh_object(name, bm, mat)
    common._bevel(obj, bevel, 3)
    return obj


# Cabinet: the arched body, a slightly wider plinth, chrome trim along the front edge.
body = extrude("Cabinet", arch_outline(HW, ARCH_Y, 0.06), BACK, FRONT, wood, bevel=0.02)
body["grain_axis"] = "y"
common.box("Plinth", (-HW - 0.02, 0.0, BACK - 0.02), (HW + 0.02, 0.07, FRONT + 0.03), chrome, 0.008)
edge = arch_outline(HW - 0.012, ARCH_Y, 0.07, 32)[1:]
common.tube("Front trim", [p + Vector((0, 0, FRONT + 0.004)) for p in edge], 0.009, chrome, sides=8)

# Glow tubes: an outer orange U along the arch, an inner red one, and
# yellow side pillars on the lower cabinet.
zf = FRONT + 0.02
outer = [Vector((-0.39, 0.12, zf))] + [p + Vector((0, 0, zf)) for p in
                                       reversed(common.arc((0, ARCH_Y, 0), 0.39, 0, math.pi, 30))] + \
        [Vector((0.39, 0.12, zf))]
outer = list(reversed(outer[:1])) + outer[1:]
common.tube("Outer tube", outer, 0.022, tube_orange, sides=12)
inner = [Vector((-0.33, 0.62, zf + 0.005))] + [p + Vector((0, 0, zf + 0.005)) for p in
                                               reversed(common.arc((0, ARCH_Y, 0), 0.33, 0, math.pi, 28))] + \
        [Vector((0.33, 0.62, zf + 0.005))]
common.tube("Inner tube", inner, 0.016, tube_red, sides=10)
for x in (-0.39, 0.39):
    for y in (0.12, 0.62):
        common.cylinder("Tube cap", Vector((x, y, zf)), 0.026, 0.03, "y", chrome, segments=12)

# Dome window: a dark recess with the record and turntable, under glass.
dome_c = Vector((0, ARCH_Y + 0.02, 0))
dome = [Vector((0.27 * math.cos(a), dome_c.y + 0.27 * math.sin(a), 0)) for a in
        (math.pi * k / 24 for k in range(25))]
extrude("Dome recess", dome, FRONT - 0.01, FRONT + 0.002, dark)
common.cylinder("Turntable", Vector((0, dome_c.y + 0.07, FRONT + 0.006)), 0.13, 0.008, "z", chrome, segments=40)
common.cylinder("Record", Vector((0, dome_c.y + 0.07, FRONT + 0.012)), 0.12, 0.004, "z", vinyl, segments=48)
common.cylinder("Record label", Vector((0, dome_c.y + 0.07, FRONT + 0.0145)), 0.04, 0.002, "z", record_label,
                segments=24)
common.cylinder("Spindle", Vector((0, dome_c.y + 0.07, FRONT + 0.02)), 0.006, 0.014, "z", chrome, segments=8)
common.tube("Tone arm", [Vector((0.2, dome_c.y + 0.2, FRONT + 0.02)), Vector((0.16, dome_c.y + 0.16, FRONT + 0.024)),
                         Vector((0.05, dome_c.y + 0.1, FRONT + 0.024))], 0.006, chrome, sides=8)
# Stacked records in the changer behind.
for k in range(5):
    common.cylinder("Stack", Vector((-0.12 + k * 0.012, dome_c.y + 0.1, FRONT - 0.004 - k * 0.001)), 0.1, 0.002, "z",
                    vinyl, segments=24)
glass = extrude("Dome glass", [p + Vector((0, 0, 0)) for p in dome], FRONT + 0.03, FRONT + 0.034, window)
common.tube("Dome rim", [p + Vector((0, 0, FRONT + 0.03)) for p in dome] + [dome[0] + Vector((0, 0, FRONT + 0.03))],
            0.008, chrome, sides=8)

# Selector panel: a sloped strip of title cards behind glass, buttons below.
cards = np.ones((128, 512, 3), dtype=np.float32) * 0.9
for i in range(16):
    r, c = divmod(i, 8)
    x0, y0 = c * 64 + 3, r * 64 + 4
    cards[y0:y0 + 56, x0:x0 + 58] = (0.95, 0.93, 0.85)
    cards[y0 + 26:y0 + 30, x0:x0 + 58] = (0.7, 0.1, 0.08)  # red rule between the A and B sides
    common.draw_text(cards, "AB"[r] + str(c + 1), x0 + 4, y0 + 52, 2, (0.1, 0.1, 0.1))
    for line in range(2):
        w = 30 + (i * 7 + line * 13) % 22
        cards[y0 + 10 + line * 8:y0 + 13 + line * 8, x0 + 6:x0 + 6 + w] = (0.2, 0.2, 0.2)
        cards[y0 + 36 + line * 6:y0 + 38 + line * 6, x0 + 6:x0 + 6 + w - 8] = (0.3, 0.3, 0.3)
cards_mat = common.image_material("Title cards", cards, roughness=0.3)
tilt = Matrix.Translation((0, 0.84, FRONT + 0.06)) @ Matrix.Rotation(math.radians(-30), 4, "X")
common.transform(common.box("Selector housing", (-0.34, -0.07, -0.09), (0.34, 0.07, 0.0), chrome, 0.01), tilt)
bm = bmesh.new()
v = [bm.verts.new(p) for p in ((-0.32, -0.055, 0.001), (0.32, -0.055, 0.001), (0.32, 0.055, 0.001),
                               (-0.32, 0.055, 0.001))]
bm.faces.new(v)
card_obj = common.mesh_object("Cards", bm, cards_mat)
common.set_uvs(card_obj, lambda co, n: ((co.x + 0.32) / 0.64, (co.y + 0.055) / 0.11))
common.transform(card_obj, tilt)
for i in range(10):
    x = -0.27 + i * 0.06
    common.box("Button", (x - 0.018, 0.735, FRONT + 0.025), (x + 0.018, 0.755, FRONT + 0.05), ivory, 0.005)
common.box("Button rail", (-0.34, 0.725, FRONT + 0.005), (0.34, 0.765, FRONT + 0.03), chrome, 0.006)

# Speaker grille: lit cloth behind chrome bars, with a center medallion.
common.box("Grille cloth", (-0.3, 0.16, FRONT - 0.01), (0.3, 0.66, FRONT + 0.004), fabric, 0.0)
common.box("Grille frame", (-0.32, 0.14, FRONT - 0.005), (0.32, 0.16, FRONT + 0.02), chrome, 0.006)
common.box("Grille frame", (-0.32, 0.66, FRONT - 0.005), (0.32, 0.68, FRONT + 0.02), chrome, 0.006)
for i in range(11):
    x = -0.27 + i * 0.054
    common.cylinder("Grille bar", Vector((x, 0.41, FRONT + 0.012)), 0.006, 0.5, "y", chrome, segments=8)
common.cylinder("Medallion", Vector((0, 0.41, FRONT + 0.022)), 0.06, 0.012, "z", chrome, segments=32)
common.torus("Medallion ring", Vector((0, 0.41, FRONT + 0.03)), 0.06, 0.006, tube_yellow, "z", 32, 8)
# Coin entry plate.
common.box("Coin plate", (0.24, 0.86, FRONT + 0.0), (0.33, 0.92, FRONT + 0.01), chrome, 0.004)
common.box("Coin slot", (0.275, 0.885, FRONT + 0.009), (0.295, 0.89, FRONT + 0.012), dark, 0.0)

common.finish_and_export(out_path, uv_scale={"Jukebox walnut": 1.3})
