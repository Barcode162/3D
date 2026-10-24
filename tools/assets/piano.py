# Builds a concert grand piano with its insides (docs/archive/BAR_REDESIGN.md,
# the hero piece): a curved bentside case in gloss black, the spruce
# soundboard, the gold cast-iron plate with its struts, about 240 strings
# (copper-wound bass strings crossing over the steel treble strings),
# tuning pins, agraffes, bridges with bridge pins, hitch pins, dampers, the
# hammer row, all 88 keys, the fallboard with the name, the music desk with
# sheet music, the lid open on its prop stick, three legs on brass casters,
# the lyre with three pedals, and a tufted bench.
#
#   blender -b --factory-startup --python tools/assets/piano.py -- <texture dir> <out.glb>
#
# Model space: keyboard along x (bass at -x), the player sits at -z, the
# tail points to +z, floor at y = 0.
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

# ---- Materials ----
lacquer = common.plain_material("Piano lacquer", (0.012, 0.012, 0.014), 0.0, 0.06)
satin = common.plain_material("Satin black", (0.02, 0.02, 0.022), 0.0, 0.35)
spruce = common.pbr_material("Soundboard spruce", tex_dir, "wood_table_001", tint=(1.25, 1.0, 0.62),
                             roughness_scale=0.6, grain="v", size=512)
maple = common.pbr_material("Bridge maple", tex_dir, "wood_table_001", tint=(0.95, 0.65, 0.4), size=256)
iron = common.plain_material("Gold plate", (0.80, 0.60, 0.28), 1.0, 0.38)
steel_str = common.plain_material("Steel string", (0.80, 0.80, 0.82), 1.0, 0.2)
copper_str = common.plain_material("Copper string", (0.92, 0.55, 0.35), 1.0, 0.3)
pin_steel = common.plain_material("Tuning pin", (0.6, 0.6, 0.62), 1.0, 0.3)
brass = common.plain_material("Piano brass", (0.91, 0.72, 0.40), 1.0, 0.22)
ivory = common.plain_material("White keys", (0.92, 0.90, 0.84), 0.0, 0.28)
ebony = common.plain_material("Black keys", (0.015, 0.015, 0.017), 0.0, 0.22)
felt_red = common.plain_material("Red felt", (0.45, 0.05, 0.06), 0.0, 0.95)
felt_white = common.plain_material("Hammer felt", (0.85, 0.83, 0.75), 0.0, 0.95)
felt_black = common.plain_material("Damper felt", (0.03, 0.03, 0.03), 0.0, 0.95)
wood_bits = common.pbr_material("Action wood", tex_dir, "wood_table_001", tint=(0.9, 0.7, 0.5), size=256)
leather = common.pbr_material("Bench leather", tex_dir, "fabric_leather_02", tint=(0.12, 0.1, 0.1), size=512)


def text_image(w, h, words, bg, fg, scale, center=True):
    px = np.zeros((h, w, 3), np.float32)
    px[:] = bg
    common.draw_text(px, words, w // 2, h // 2 + scale * 7 // 2, scale, fg, center=center)
    return px


name_mat = common.image_material("Piano name", text_image(256, 32, "BAR3D", (0.012, 0.012, 0.014),
                                                          common.srgb((0.9, 0.72, 0.35)), 3), roughness=0.1)

# Sheet music: staves, notes, a title.
SM_W, SM_H = 512, 256
sm = np.ones((SM_H, SM_W, 3), np.float32) * np.array(common.srgb((0.93, 0.91, 0.85)))
rng = np.random.default_rng(4)
for page in range(2):
    x0, x1 = 20 + page * 256, 236 + page * 256
    for staff in range(5):
        top = SM_H - 40 - staff * 42
        for line in range(5):
            sm[top - line * 5, x0:x1] = 0.05
        for n in range(14):
            nx = x0 + 12 + n * 15
            ny = top - rng.integers(0, 9) * 2.5
            yy, xx = np.mgrid[0:SM_H, 0:SM_W]
            sm[((xx - nx) ** 2 / 9 + (yy - ny) ** 2 / 5) < 1] = 0.03
            sm[int(ny):int(ny) + 14, nx + 2] = 0.03
        sm[top - 20:top, x0] = 0.05
common.draw_text(sm, "NOCTURNE", 128, SM_H - 12, 2, (0.05, 0.05, 0.05), center=True)
sheet = common.image_material("Sheet music", sm, roughness=0.8)

# ---- Case outline (top view, x, z) ----
W2 = 0.78       # half width
CASE_Y0, CASE_Y1 = 0.62, 0.97   # bottom and top of the rim
KEY_Y = 0.73    # top of the white keys


def bezier(p0, p1, p2, p3, n):
    out = []
    for i in range(n + 1):
        t = i / n
        a = (1 - t) ** 3
        b = 3 * (1 - t) ** 2 * t
        c = 3 * (1 - t) * t ** 2
        d = t ** 3
        out.append((a * p0[0] + b * p1[0] + c * p2[0] + d * p3[0], a * p0[1] + b * p1[1] + c * p2[1] + d * p3[1]))
    return out


# Counter-clockwise seen from above (x right, z toward the tail): front edge,
# treble cheek, bentside S-curve, rounded tail, straight spine.
outline = [(-W2, 0.0), (W2, 0.0), (W2, 0.62)]
outline += bezier((W2, 0.62), (W2, 1.25), (0.02, 1.2), (-0.12, 1.95), 18)[1:]
outline += bezier((-0.12, 1.95), (-0.2, 2.3), (-W2, 2.3), (-W2, 1.9), 14)[1:]
outline.append((-W2, 0.0))
outline = outline[:-1]


def offset_outline(pts, d):
    """Moves a closed outline inward by d (pts counter-clockwise in x/z with
    z down the page, i.e. clockwise in math terms; normals computed here)."""
    n = len(pts)
    area = sum(pts[i][0] * pts[(i + 1) % n][1] - pts[(i + 1) % n][0] * pts[i][1] for i in range(n))
    sign = 1 if area > 0 else -1
    out = []
    for i in range(n):
        p0, p1, p2 = pts[i - 1], pts[i], pts[(i + 1) % n]
        e1 = Vector((p1[0] - p0[0], p1[1] - p0[1])).normalized()
        e2 = Vector((p2[0] - p1[0], p2[1] - p1[1])).normalized()
        n1 = Vector((-e1.y, e1.x)) * sign
        n2 = Vector((-e2.y, e2.x)) * sign
        nm = (n1 + n2)
        if nm.length < 1e-6:
            nm = n1
        nm.normalize()
        scale = 1.0 / max(0.3, nm.dot(n1))
        out.append((p1[0] + nm.x * d * scale, p1[1] + nm.y * d * scale))
    return out


def ring_wall(name, outer, thickness, y0, y1, mat, close=True):
    """A wall standing on a closed outline, `thickness` thick inward."""
    inner = offset_outline(outer, thickness)
    bm = bmesh.new()
    n = len(outer)
    ob = [bm.verts.new((x, y0, z)) for (x, z) in outer]
    ot = [bm.verts.new((x, y1, z)) for (x, z) in outer]
    ib = [bm.verts.new((x, y0, z)) for (x, z) in inner]
    it = [bm.verts.new((x, y1, z)) for (x, z) in inner]
    for i in range(n if close else n - 1):
        j = (i + 1) % n
        bm.faces.new([ob[i], ob[j], ot[j], ot[i]])
        bm.faces.new([ib[j], ib[i], it[i], it[j]])
        bm.faces.new([ot[i], ot[j], it[j], it[i]])
        bm.faces.new([ob[j], ob[i], ib[i], ib[j]])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    obj = common.mesh_object(name, bm, mat)
    obj["smooth"] = True
    return obj, inner


# Inside of the case starts behind the keyboard area (z > 0.32).
def clip_back(pts, z_min):
    out = []
    n = len(pts)
    for i in range(n):
        a, b = pts[i], pts[(i + 1) % n]
        if a[1] >= z_min:
            out.append(a)
        if (a[1] - z_min) * (b[1] - z_min) < 0:
            t = (z_min - a[1]) / (b[1] - a[1])
            out.append((a[0] + (b[0] - a[0]) * t, z_min))
    return out


rim, inner = ring_wall("Rim", outline, 0.055, CASE_Y0, CASE_Y1, lacquer)
common.prism("Case bottom", outline, CASE_Y0, CASE_Y0 + 0.025, lacquer)
# Rim top capping strip (slightly rounded look).
ring_wall("Rim cap", offset_outline(outline, -0.004), 0.064, CASE_Y1, CASE_Y1 + 0.012, lacquer)

# ---- Soundboard, ribs, bridges ----
sb_outline = clip_back(offset_outline(outline, 0.055), 0.33)
common.prism("Soundboard", sb_outline, 0.78, 0.79, spruce)["grain_axis"] = "z"
# Inner rim liner (wood) just above the soundboard.
ring_wall("Liner", [(x, z) for (x, z) in sb_outline], 0.025, 0.79, 0.82, maple, close=True)


def inside_x_range(z):
    """Where the inside of the case spans along x at depth z."""
    xs = []
    pts = sb_outline
    n = len(pts)
    for i in range(n):
        a, b = pts[i], pts[(i + 1) % n]
        if (a[1] - z) * (b[1] - z) <= 0 and a[1] != b[1]:
            t = (z - a[1]) / (b[1] - a[1])
            xs.append(a[0] + (b[0] - a[0]) * t)
    return (min(xs), max(xs)) if len(xs) >= 2 else (0, 0)


def inside_z_end(x):
    """How far back the inside reaches at x (the bentside/tail)."""
    best = 0.0
    pts = sb_outline
    n = len(pts)
    for i in range(n):
        a, b = pts[i], pts[(i + 1) % n]
        if (a[0] - x) * (b[0] - x) <= 0 and a[0] != b[0]:
            t = (x - a[0]) / (b[0] - a[0])
            best = max(best, a[1] + (b[1] - a[1]) * t)
    return best


# ---- Strings ----
# Treble and middle strings run straight back from the agraffes (z = 0.42)
# to hitch pins near the bentside; the bass strings cross over them on a
# slant toward the tail, a bit higher.
STR_Y = 0.875
AGRAFFE_Z = 0.42
treble_x = np.linspace(-0.36, 0.66, 68)          # notes 21..88, three strings each
strings = []
for k, x in enumerate(treble_x):
    end = inside_z_end(x) - 0.06
    for s in (-1, 0, 1):
        xs = x + s * 0.0042
        strings.append(((xs, STR_Y, AGRAFFE_Z), (xs * 0.97 + 0.01, STR_Y, end), 0.0011 if k > 20 else 0.0014,
                        steel_str))
bass_x = np.linspace(-0.69, -0.40, 20)           # notes 1..20: 8 single, 12 double, copper wound
for k, x in enumerate(bass_x):
    n = 1 if k < 8 else 2
    t = k / 19
    ex = -0.62 + t * 0.55                            # they fan toward the tail
    ez = inside_z_end(ex) - 0.07
    for s in range(n):
        xs = x + (s - 0.5) * 0.006 * (n - 1)
        strings.append(((xs, STR_Y + 0.03, AGRAFFE_Z + 0.03), (ex + (s - 0.5) * 0.006 * (n - 1), STR_Y + 0.025, ez),
                        0.0028 - 0.0008 * t, copper_str))
for (a, b, r, mat) in strings:
    common.tube("String", [a, b], r, mat, sides=4, caps=False)
    # Hitch pin at the far end, tuning pin at the front.
    common.cylinder("Hitch pin", Vector((b[0], b[1] - 0.004, b[2] + 0.012)), 0.003, 0.02, "y", pin_steel, segments=6)
print("STRINGS", len(strings))

# Tuning pins in staggered rows under the music desk, on the pin block.
common.box("Pin block", (-W2 + 0.06, 0.82, 0.24), (W2 - 0.06, 0.86, 0.4), maple, 0.004)
for i, (a, b, r, mat) in enumerate(strings):
    row = i % 3
    px = a[0] + (0.002 if row == 1 else -0.002)
    pz = 0.27 + row * 0.035
    common.cylinder("Tuning pin", Vector((px, 0.885, pz)), 0.0035, 0.05, "y", pin_steel, segments=6)
    common.tube("String end", [(px, 0.9, pz), (a[0], a[1], a[2])], 0.0009, mat, sides=3, caps=False)

# Bridges (maple strips under the strings) with bridge pins.
treble_bridge = [(x * 0.97 + 0.01, inside_z_end(x) - 0.32 - 0.25 * (x + 0.36)) for x in np.linspace(-0.36, 0.66, 24)]
common.sweep("Treble bridge", treble_bridge, [(-0.015, 0.79), (0.015, 0.79), (0.012, STR_Y - 0.002),
                                              (-0.012, STR_Y - 0.002)], maple)
bass_bridge = [(-0.62 + t * 0.55 * 0.9, inside_z_end(-0.62 + t * 0.55) - 0.28) for t in np.linspace(0, 1, 8)]
common.sweep("Bass bridge", bass_bridge, [(-0.016, 0.79), (0.016, 0.79), (0.013, STR_Y + 0.022),
                                          (-0.013, STR_Y + 0.022)], maple)
for (x, z) in treble_bridge[::1]:
    for d in (-0.006, 0.006):
        common.cylinder("Bridge pin", Vector((x + d, STR_Y + 0.004, z + d)), 0.0015, 0.012, "y", brass, segments=4)

# ---- Cast-iron plate: a gold frame around the edge, a front bar with the
# agraffes, and struts fanning back (the gaps show the soundboard) ----
plate_outer = offset_outline(outline, 0.07)
plate_band = clip_back(plate_outer, 0.36)
ring_wall("Plate rim", plate_band, 0.075, 0.84, 0.865, iron)
common.box("Plate front bar", (-W2 + 0.07, 0.84, 0.36), (W2 - 0.07, 0.87, 0.47), iron, 0.006)
for x in np.linspace(-0.62, 0.62, 30):
    common.cylinder("Agraffe", Vector((x, 0.875, AGRAFFE_Z)), 0.005, 0.015, "y", brass, segments=6)
for (x0, x1) in ((-0.42, -0.5), (-0.05, -0.2), (0.28, 0.1), (0.6, 0.35)):
    z1 = inside_z_end(x1) - 0.12
    pts = [(x0, 0.47), (x1, z1)]
    d = Vector((x1 - x0, z1 - 0.47))
    ln = d.length
    ang = math.atan2(d.y, d.x)
    strut = common.box("Plate strut", (0, -0.0125, -0.025), (ln, 0.0125, 0.025), iron, 0.006)
    strut.data.transform(Matrix.Translation((x0, 0.8525, 0.47)) @ Matrix.Rotation(-ang, 4, "Y"))
common.box("Plate cross bar", (-0.55, 0.84, 1.18), (0.18, 0.865, 1.24), iron, 0.006)
# Plate bolts and the round "rosette" holes' rims.
for (x, z) in ((-0.6, 0.6), (-0.2, 0.75), (0.25, 0.7), (0.55, 0.62), (-0.45, 1.35), (0.0, 1.32), (-0.55, 1.75)):
    common.cylinder("Plate bolt", Vector((x, 0.87, z)), 0.012, 0.012, "y", brass, segments=10)
for (x, z, r) in ((-0.25, 0.95, 0.07), (0.15, 0.9, 0.06), (-0.4, 1.55, 0.06)):
    common.torus("Plate rosette", Vector((x, 0.855, z)), r, 0.008, iron, segments=20, sides=5)

# ---- Action: hammers under the strings, dampers on top ----
for x in np.concatenate([bass_x, treble_x]):
    common.box("Hammer", (x - 0.0055, 0.83, AGRAFFE_Z + 0.03), (x + 0.0055, 0.86, AGRAFFE_Z + 0.07), felt_white, 0.003)
    common.box("Hammer shank", (x - 0.0015, 0.825, AGRAFFE_Z - 0.06), (x + 0.0015, 0.83, AGRAFFE_Z + 0.05), wood_bits,
               0.0)
for x in np.concatenate([bass_x, treble_x[:48]]):
    common.box("Damper", (x - 0.006, STR_Y + 0.006, AGRAFFE_Z + 0.06), (x + 0.006, STR_Y + 0.02, AGRAFFE_Z + 0.11),
               felt_black, 0.002)
    common.box("Damper head", (x - 0.006, STR_Y + 0.02, AGRAFFE_Z + 0.07), (x + 0.006, STR_Y + 0.07, AGRAFFE_Z + 0.1),
               wood_bits, 0.002)
common.box("Damper rail", (-0.7, STR_Y + 0.07, AGRAFFE_Z + 0.075), (0.42, STR_Y + 0.085, AGRAFFE_Z + 0.095),
           wood_bits, 0.003)
# Red felt strips along the front of the plate.
common.box("Felt strip", (-W2 + 0.07, 0.87, 0.47), (W2 - 0.07, 0.874, 0.49), felt_red, 0.0)

# ---- Keyboard ----
WHITE_W, BLACK_W = 0.02345, 0.0137
KEY_Z0, KEY_Z1 = -0.155, 0.0     # front edge, fallboard
whites = [i for i in range(88) if ((i + 9) % 12) not in (1, 3, 6, 8, 10)]
x_start = -len(whites) * WHITE_W / 2
wx = {}
for k, i in enumerate(whites):
    x0 = x_start + k * WHITE_W
    wx[i] = x0
    common.box("White key", (x0 + 0.0006, KEY_Y - 0.022, KEY_Z0), (x0 + WHITE_W - 0.0006, KEY_Y, KEY_Z1), ivory,
               0.0015, 2)
for i in range(88):
    if ((i + 9) % 12) in (1, 3, 6, 8, 10):
        x = wx[i - 1] + WHITE_W  # boundary between the neighbors
        common.box("Black key", (x - BLACK_W / 2, KEY_Y - 0.004, KEY_Z0 + 0.052), (x + BLACK_W / 2, KEY_Y + 0.012,
                                                                                  KEY_Z1 - 0.005), ebony, 0.0025, 2)
kb_w = len(whites) * WHITE_W
common.box("Keybed", (-W2, CASE_Y0, KEY_Z0 - 0.02), (W2, KEY_Y - 0.022, 0.32), lacquer, 0.004)
common.box("Key slip", (-kb_w / 2 - 0.005, KEY_Y - 0.06, KEY_Z0 - 0.025), (kb_w / 2 + 0.005, KEY_Y - 0.012,
                                                                            KEY_Z0 - 0.004), lacquer, 0.004)
common.box("Felt under slip", (-kb_w / 2, KEY_Y - 0.022, KEY_Z0 - 0.006), (kb_w / 2, KEY_Y - 0.016, KEY_Z0),
           felt_red, 0.0)
for side in (-1, 1):
    x0 = side * (kb_w / 2) + (0 if side > 0 else -0.075)
    common.box("Cheek block", (x0, KEY_Y - 0.06, KEY_Z0 - 0.025), (x0 + 0.075, KEY_Y + 0.05, 0.32), lacquer, 0.012, 3)
# Fallboard (open, tucked back) with the name in gold, and the board in
# front of the tuning pins.
common.box("Fallboard", (-kb_w / 2, KEY_Y, 0.0), (kb_w / 2, KEY_Y + 0.12, 0.035), lacquer, 0.006)
common.panel("Name", (0.0, KEY_Y + 0.07, -0.001), (1, 0, 0), (0, 1, 0), 0.22, 0.0275, name_mat)
common.box("Name board", (-kb_w / 2, KEY_Y + 0.12, 0.0), (kb_w / 2, 0.9, 0.24), lacquer, 0.006)

# Music desk with sheet music, leaning back.
desk_tilt = Matrix.Translation((0, 0.97, 0.36)) @ Matrix.Rotation(-0.32, 4, "X")
desk = common.box("Music desk", (-0.42, 0.0, -0.012), (0.42, 0.32, 0.0), lacquer, 0.006)
desk.data.transform(desk_tilt)
ledge = common.box("Desk ledge", (-0.42, -0.015, -0.05), (0.42, 0.015, 0.0), lacquer, 0.004)
ledge.data.transform(desk_tilt)
sheet_q = common.quad("Sheet music", [(-0.31, 0.02, -0.014), (0.31, 0.02, -0.014), (0.31, 0.3, -0.014),
                                      (-0.31, 0.3, -0.014)], sheet)
sheet_q.data.transform(desk_tilt)

# ---- Lid, open on the treble side, on its prop stick ----
LID_ANGLE = math.radians(33)
hinge = Matrix.Translation((-W2 - 0.005, CASE_Y1 + 0.012, 0)) @ Matrix.Rotation(LID_ANGLE, 4, "Z") @ \
    Matrix.Translation((W2 + 0.005, -(CASE_Y1 + 0.012), 0))
lid_pts = clip_back(offset_outline(outline, -0.006), 0.38)
lid = common.prism("Lid", lid_pts, CASE_Y1 + 0.012, CASE_Y1 + 0.032, lacquer)
lid.data.transform(hinge)
# The front flap folded back onto the lid.
flap_pts = [(x, z) for (x, z) in [(-W2 - 0.006, 0.38), (W2 + 0.006, 0.38), (W2 + 0.006, 0.0), (-W2 - 0.006, 0.0)]]
flap = common.prism("Lid flap", [(x, 0.38 + (0.38 - z)) for (x, z) in flap_pts], CASE_Y1 + 0.032,
                    CASE_Y1 + 0.05, lacquer)
flap.data.transform(hinge)
for z in (0.5, 1.2, 1.8):  # brass hinges along the spine
    common.cylinder("Lid hinge", Vector((-W2 - 0.005, CASE_Y1 + 0.02, z)), 0.008, 0.1, "z", brass, segments=8)
# Prop stick from the rim up to the lid's underside.
foot = Vector((W2 - 0.08, CASE_Y1 + 0.012, 1.0))
lid_local = Vector((W2 - 0.38, CASE_Y1 + 0.012, 1.0))
top = hinge @ lid_local
stick_d = top - foot
stick = common.cylinder("Prop stick", Vector((0, 0, 0)), 0.012, stick_d.length, "y", lacquer, segments=10)
rot = stick_d.to_track_quat("Y", "Z").to_matrix().to_4x4()
stick.data.transform(Matrix.Translation((foot + top) / 2) @ rot)
common.box("Prop cup", (foot.x - 0.025, foot.y - 0.005, foot.z - 0.03), (foot.x + 0.025, foot.y + 0.012, foot.z + 0.03),
           brass, 0.004)

# ---- Legs with casters ----
def leg(x, z):
    common.box("Leg block", (x - 0.075, CASE_Y0 - 0.06, z - 0.075), (x + 0.075, CASE_Y0, z + 0.075), lacquer, 0.01)
    common.lathe("Leg", [(0, 0.08), (0.05, 0.08), (0.042, 0.16), (0.048, 0.3), (0.062, 0.48), (0.066, 0.52),
                         (0.058, CASE_Y0 - 0.06), (0, CASE_Y0 - 0.06)], lacquer, 20).data.transform(
        Matrix.Translation((x, 0, z)))
    common.lathe("Leg ferrule", [(0, 0.06), (0.052, 0.06), (0.052, 0.09), (0, 0.09)], brass, 18).data.transform(
        Matrix.Translation((x, 0, z)))
    common.cylinder("Caster wheel", Vector((x, 0.03, z)), 0.03, 0.03, "x", brass, segments=14)
    common.box("Caster fork", (x - 0.025, 0.03, z - 0.012), (x + 0.025, 0.065, z + 0.012), brass, 0.004)


leg(-W2 + 0.11, 0.22)
leg(W2 - 0.11, 0.22)
leg(-0.42, 1.9)

# ---- Lyre with three pedals ----
for side in (-0.09, 0.09):
    common.tube("Lyre arm", [(side, CASE_Y0, 0.12), (side * 1.6, 0.4, 0.13), (side * 0.9, 0.12, 0.12)], 0.012, lacquer,
                sides=8)
common.box("Pedal box", (-0.16, 0.06, 0.06), (0.16, 0.14, 0.18), lacquer, 0.01)
common.box("Lyre top", (-0.12, CASE_Y0 - 0.03, 0.08), (0.12, CASE_Y0, 0.16), lacquer, 0.006)
for k, x in enumerate((-0.07, 0.0, 0.07)):
    p = common.box("Pedal", (-0.018, -0.006, -0.12), (0.018, 0.006, 0.0), brass, 0.004)
    p.data.transform(Matrix.Translation((x, 0.09, 0.06)) @ Matrix.Rotation(-0.12, 4, "X"))
for side in (-1, 1):  # brace rods back to the case
    common.tube("Lyre brace", [(side * 0.1, 0.14, 0.17), (side * 0.12, CASE_Y0, 0.6)], 0.007, brass, sides=6)

# ---- Bench ----
BZ = -0.62
common.box("Bench seat", (-0.38, 0.47, BZ - 0.18), (0.38, 0.5, BZ + 0.18), lacquer, 0.008)
common.box("Bench cushion", (-0.37, 0.5, BZ - 0.17), (0.37, 0.56, BZ + 0.17), leather, 0.025, 3)
for i in range(4):
    for j in range(2):
        common.sphere("Button", Vector((-0.27 + i * 0.18, 0.562, BZ - 0.06 + j * 0.12)), 0.008, satin, 8, 4)
for (x, z) in ((-0.33, BZ - 0.14), (0.33, BZ - 0.14), (-0.33, BZ + 0.14), (0.33, BZ + 0.14)):
    common.lathe("Bench leg", [(0, 0), (0.025, 0), (0.03, 0.1), (0.022, 0.3), (0.03, 0.42), (0.032, 0.47), (0, 0.47)],
                 lacquer, 14).data.transform(Matrix.Translation((x, 0, z)))
common.box("Bench stretcher", (-0.33, 0.12, BZ - 0.012), (0.33, 0.15, BZ + 0.012), lacquer, 0.006)
for side in (-1, 1):
    common.cylinder("Bench knob", Vector((side * 0.39, 0.4, BZ)), 0.02, 0.03, "x", brass, segments=12)

common.finish_and_export(out_path, uv_scale={"Soundboard spruce": 1.2, "Bridge maple": 2.0, "Action wood": 3.0,
                                             "Bench leather": 3.0})
