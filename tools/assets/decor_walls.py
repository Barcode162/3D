# The decor of the hall (docs/archive/BAR_REDESIGN.md), taken apart (the
# world remake J71): Edison string lights across the hall, neon signs
# (COCKTAILS, GAME ON, PLAY, a music note), a gallery of framed photos, a
# collector wall of tin signs, licence plates and records, a big clock and a
# sports TV on the soffit, a chalkboard A-frame, plants, corner speakers,
# brass wall sconces, fire extinguishers, smoke detectors, switches, a coat
# rack with coats, an umbrella stand, a trash can, beer crates and a keg,
# and a little table in the bay window. Every one is its own model, built
# in the room's place and saved standing on its own origin; copies of one
# model (speakers, sconces, crates...) are one file placed several times.
#
#   blender -b --factory-startup --python tools/assets/decor_walls.py -- <texture dir> <out dir> [<places.json>]
#
# <places.json>: where each thing stands (id, model, kind, at, turn), for the
# place file (assets/world/places/bar.json).
import math
import os
import random
import sys

import json

import bpy
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402
import layout as L  # noqa: E402
import props_lib  # noqa: E402

args = sys.argv[sys.argv.index("--") + 1:]
tex_dir, out_dir = args[:2]
places_path = args[2] if len(args) > 2 else None
rng = random.Random(13)
PLACES = []
P = brass = black = None


def begin():
    """A new model: an empty scene and fresh shared materials."""
    global P, brass, black
    common.reset_scene()
    P = props_lib.Props(tex_dir, seed=99)
    brass, black = P.mat("brass"), P.mat("black")


def save(model, origin=(0, 0, 0), turn=0.0, image_format="JPEG"):
    """Moves what was built (in the room's place) onto the model's own origin
    and facing (`turn` degrees about +y, its front = +z) and writes it."""
    m = Matrix.Rotation(-math.radians(turn), 4, "Y") @ Matrix.Translation((-origin[0], -origin[1], -origin[2]))
    for o in bpy.context.scene.objects:
        if o.type == "MESH":
            o.data.transform(m)
    common.finish_and_export(os.path.join(out_dir, model + ".glb"),
                             uv_scale={"Coat 0": 2.0, "Coat 1": 2.0, "Coat 2": 2.0}, image_format=image_format)


def put(thing, model, kind, origin, turn=0.0):
    """A thing for the place file: `model` placed at `origin`, turned."""
    PLACES.append({"id": thing, "kind": kind, "model": model + ".glb",
                   "at": [round(v, 4) for v in origin], "turn": round(turn, 3)})


def neon_mats():
    return {
        "pink": common.plain_material("Neon pink", (1.0, 0.3, 0.7), 0.0, 0.3, emission=(1.0, 0.18, 0.55), strength=2.2),
        "cyan": common.plain_material("Neon cyan", (0.3, 0.9, 1.0), 0.0, 0.3, emission=(0.2, 0.85, 1.0), strength=2.2),
        "amber": common.plain_material("Neon amber", (1.0, 0.7, 0.3), 0.0, 0.3, emission=(1.0, 0.55, 0.15), strength=2.2),
        "green": common.plain_material("Neon green", (0.4, 1.0, 0.4), 0.0, 0.3, emission=(0.3, 1.0, 0.35), strength=2.2),
    }


FACING = {"+z": 0.0, "-z": 180.0, "+x": 90.0, "-x": -90.0}


def place(obj, m):
    obj.data.transform(m)
    return obj


def wall_frame(x, z, facing):
    """Matrix for things on a wall: local x along the wall (to the viewer's
    right), local +z out of the wall. facing = the wall's normal into the
    room: "+x", "-x", "+z", "-z"."""
    yaw = {"+z": 0.0, "-z": math.pi, "+x": math.pi / 2, "-x": -math.pi / 2}[facing]
    return Matrix.Translation((x, 0, z)) @ Matrix.Rotation(yaw, 4, "Y")


# ---- Edison string lights: sagging strands across the hall (one thing) ----
def strand(a, b, sag, n_bulbs, bulb, cord):
    a, b = Vector(a), Vector(b)
    pts = []
    for i in range(41):
        t = i / 40
        p = a.lerp(b, t)
        p.y -= sag * 4 * t * (1 - t)
        pts.append(p)
    common.tube("String cord", pts, 0.004, cord, sides=4)
    for k in range(1, n_bulbs + 1):
        t = k / (n_bulbs + 1)
        p = a.lerp(b, t)
        p.y -= sag * 4 * t * (1 - t)
        common.cylinder("Socket", Vector((p.x, p.y - 0.03, p.z)), 0.012, 0.04, "y", black, segments=8)
        common.sphere("Bulb", Vector((p.x, p.y - 0.075, p.z)), 0.028, bulb, 10, 6).data.transform(
            Matrix.Translation((p.x, p.y - 0.075, p.z)) @ Matrix.Diagonal((1, 1.3, 1, 1)) @
            Matrix.Translation((-p.x, -(p.y - 0.075), -p.z)))


def string_lights():
    begin()
    bulb = common.plain_material("Edison bulb", (1.0, 0.98, 0.95), 0.0, 0.3, emission=(1.0, 0.97, 0.92), strength=9.0)  # daylight white (J115.2)
    cord = common.plain_material("Cord", (0.02, 0.02, 0.02), 0.0, 0.6)
    y = L.H - 0.45
    for (a, b) in (((-8.6, y, -3.3), (5.5, y, 6.6)), ((-8.6, y, 6.6), (5.5, y, -3.3)),
                   ((-4.0, y, -3.3), (8.6, y, 6.0)), ((-8.6, y, 2.2), (8.6, y, 2.2))):
        length = (Vector(a) - Vector(b)).length
        strand(a, b, 0.45, int(length / 0.55), bulb, cord)
    save("string_lights")
    put("string_lights", "string_lights", "glow_prop", (0, 0, 0))


# ---- Neon signs (pixel-font letters as lit blocks on clear acrylic) ----
glyphs = common._glyphs()


def neon_word(word, m, y, s, color, colors=None):
    neon_cols = neon_mats()
    acrylic = common.glass_material("Neon backer", color=(0.5, 0.5, 0.6), alpha=0.18)
    x0 = -len(word) * 6 * s / 2
    for i, ch in enumerate(word):
        for r, row in enumerate(glyphs.get(ch, [])):
            for c, dot in enumerate(row):
                if dot == "#":
                    x = x0 + (i * 6 + c) * s
                    yy = y - r * s
                    mat = neon_cols[colors[i % len(colors)]] if colors else neon_cols[color]
                    place(common.box("Neon", (x - s * 0.33, yy - s * 0.33, 0.04), (x + s * 0.33, yy + s * 0.33, 0.055),
                                     mat, 0.0), m)
    place(common.box("Neon backer", (x0 - 0.06, y - 7 * s - 0.04, 0.0), (-x0 + 0.04, y + 0.06, 0.03), acrylic, 0.008), m)
    for xx in (x0, -x0 - 0.02):
        place(common.cylinder("Standoff", Vector((xx, y - 3 * s, 0.015)), 0.008, 0.03, "z", brass, segments=8), m)


def neon_signs():
    # On the soffit's front face, over the bar.
    for (name, word, x, z, facing, y, size, color, colors) in (
            ("neon_cocktails", "COCKTAILS", -6.3, L.SOFFIT_Z1 + 0.035, "+z", 3.62, 0.042, "pink", None),
            ("neon_game_on", "GAME ON", L.X1, -2.0, "-x", 3.55, 0.05, "cyan", ["cyan", "green"]),  # over the sims
            ("neon_play", "PLAY", -3.8, L.Z1, "-z", 3.2, 0.04, "amber", None)):                    # by the jukebox
        begin()
        neon_word(word, wall_frame(x, z, facing), y, size, color, colors)
        save(name, (x, 0, z), FACING[facing])
        put(name, name, "neon_sign", (x, 0, z), FACING[facing])
    # A music note over the stage (drawn as tubes), front wall.
    begin()
    neon_cols = neon_mats()
    acrylic = common.glass_material("Neon backer", color=(0.5, 0.5, 0.6), alpha=0.18)
    note_m = wall_frame(-7.2, L.Z1, "-z") @ Matrix.Translation((0, 2.9, 0.05))
    for (a, b) in (((-0.12, -0.3), (-0.12, 0.25)), ((0.18, -0.2), (0.18, 0.35)), ((-0.12, 0.25), (0.18, 0.35)),
                   ((-0.12, 0.15), (0.18, 0.25))):
        place(common.tube("Neon note", [(a[0], a[1], 0), (b[0], b[1], 0)], 0.012, neon_cols["pink"], sides=6), note_m)
    for (cx, cy) in ((-0.2, -0.3), (0.1, -0.2)):
        ring = [(cx + 0.08 * math.cos(t), cy + 0.055 * math.sin(t), 0) for t in np.linspace(0, 2 * math.pi, 17)]
        place(common.tube("Neon note head", ring, 0.012, neon_cols["pink"], sides=6), note_m)
    place(common.box("Note backer", (-0.35, -0.42, -0.05), (0.32, 0.45, -0.02), acrylic, 0.008), note_m)
    save("neon_note", (-7.2, 0, L.Z1), 180)
    put("neon_note", "neon_note", "neon_sign", (-7.2, 0, L.Z1), 180)


# ---- Gallery wall above the jukebox (front wall, green plaster) ----
def gallery():
    for k, (x, y, w, h) in enumerate(((-5.0, 2.3, 0.45, 0.6), (-4.4, 2.55, 0.5, 0.38), (-4.45, 2.05, 0.3, 0.38),
                                      (-3.25, 2.5, 0.6, 0.45), (-2.65, 2.15, 0.35, 0.45), (-2.6, 2.75, 0.3, 0.3),
                                      (-4.95, 1.6, 0.35, 0.28))):
        begin()
        # wall_frame for -z flips x: world x = -local x.
        P.frame(x, y, L.Z1, w, h, k + 2, yaw=math.pi, depth=0.03, border=0.03 if k % 2 else 0.045,
                mat=brass if k % 3 == 0 else None)
        name = f"gallery_photo_{k + 1}"
        save(name, (x, y, L.Z1), 180)
        put(name, name, "wall_prop", (x, y, L.Z1), 180)


# ---- Collector wall: back wall right of doorway A, above the crates ----
def tin_material():
    tin_px = np.zeros((256, 256, 3), np.float32)
    designs = [((0.7, 0.08, 0.05), (0.95, 0.85, 0.6), "LAGER"), ((0.05, 0.15, 0.4), (0.95, 0.8, 0.3), "STOUT"),
               ((0.1, 0.3, 0.12), (0.95, 0.95, 0.85), "ALE"), ((0.9, 0.75, 0.2), (0.15, 0.05, 0.05), "COLA")]
    for k, (bg, fg, word) in enumerate(designs):
        r, c = divmod(k, 2)
        cell = np.zeros((128, 128, 3), np.float32)
        cell[:] = common.srgb(bg)
        cell[6:10, :] = cell[-10:-6, :] = cell[:, 6:10] = cell[:, -10:-6] = common.srgb(fg)
        common.draw_text(cell, word, 64, 74, 4, common.srgb(fg), center=True)
        common.draw_text(cell, "EST 1962", 64, 34, 1, common.srgb(fg), center=True)
        cell *= (0.8 + 0.2 * common.noise2d(128, 128, 8, k))[..., None]  # rust and wear
        tin_px[r * 128:(r + 1) * 128, c * 128:(c + 1) * 128] = cell
    return common.image_material("Tin signs", tin_px, metallic=0.6, roughness=0.45)


def plates_material():
    plate_px = np.zeros((64, 256, 3), np.float32)
    for k, (bg, fg, txt) in enumerate((((0.9, 0.9, 0.85), (0.1, 0.2, 0.6), "BAR 3D"), ((0.95, 0.8, 0.2), (0.1, 0.1, 0.1), "JAZZ 88"),
                                       ((0.2, 0.3, 0.6), (0.95, 0.95, 0.95), "PIANO1"), ((0.9, 0.3, 0.2), (1, 1, 1), "4K MANIA"))):
        cell = np.zeros((64, 64, 3), np.float32)
        cell[:] = common.srgb(bg)
        common.draw_text(cell, txt[:6], 32, 38, 1, common.srgb(fg), center=True)
        plate_px[:, k * 64:(k + 1) * 64] = cell
    return common.image_material("License plates", plate_px, metallic=0.5, roughness=0.4)


def collector_wall():
    cw = Matrix.Translation((0, 0, L.Z0))   # back wall, facing +z, x as is
    for k, (x, y, s) in enumerate(((4.85, 2.9, 0.42), (5.55, 2.55, 0.38), (5.0, 2.2, 0.32), (5.6, 3.3, 0.3))):
        begin()
        tin = tin_material()
        r, c = divmod(k, 2)
        u0, v0 = c * 0.5 + 0.01, r * 0.5 + 0.01
        place(common.box("Tin sign", (x - s / 2, y - s / 2, 0.0), (x + s / 2, y + s / 2, 0.01), black, 0.004), cw)
        place(common.quad("Tin face", [(x - s / 2, y - s / 2, 0.011), (x + s / 2, y - s / 2, 0.011),
                                       (x + s / 2, y + s / 2, 0.011), (x - s / 2, y + s / 2, 0.011)], tin,
                          uvs=((u0, v0), (u0 + 0.48, v0), (u0 + 0.48, v0 + 0.48), (u0, v0 + 0.48))), cw)
        name = f"tin_sign_{k + 1}"
        save(name, (x, y, L.Z0))
        put(name, name, "wall_prop", (x, y, L.Z0))
    for k in range(4):
        begin()
        plates = plates_material()
        x, y = 4.9 + (k % 2) * 0.4, 1.55 + (k // 2) * 0.24
        place(common.quad("Plate", [(x - 0.15, y - 0.075, 0.006), (x + 0.15, y - 0.075, 0.006),
                                    (x + 0.15, y + 0.075, 0.006), (x - 0.15, y + 0.075, 0.006)], plates,
                          uvs=((k / 4, 0), ((k + 1) / 4, 0), ((k + 1) / 4, 1), (k / 4, 1))), cw)
        place(common.box("Plate back", (x - 0.152, y - 0.077, 0.0), (x + 0.152, y + 0.077, 0.005), P.mat("steel"), 0.002), cw)
        name = f"license_plate_{k + 1}"
        save(name, (x, y, L.Z0))
        put(name, name, "wall_prop", (x, y, L.Z0))
    for k, (x, y) in enumerate(((5.75, 1.9), (4.75, 3.45), (5.9, 2.95))):
        begin()
        vinyl = common.plain_material("Vinyl", (0.01, 0.01, 0.012), 0.0, 0.25)
        label = common.plain_material(f"Record label {k}", [(0.8, 0.1, 0.1), (0.95, 0.8, 0.2), (0.2, 0.4, 0.8)][k], 0.0, 0.5)
        groove = common.plain_material("Groove shine", (0.1, 0.1, 0.11), 0.5, 0.2)
        # Flat on the wall (it hung 4 mm off it).
        place(common.cylinder("Record", Vector((x, y, 0.002)), 0.15, 0.004, "z", vinyl, segments=32), cw)
        place(common.cylinder("Record label", Vector((x, y, 0.005)), 0.05, 0.002, "z", label, segments=20), cw)
        for rr in (0.08, 0.11, 0.135):
            place(common.torus("Groove", Vector((x, y, 0.0045)), rr, 0.0012, groove, "z", 32, 3), cw)
        name = f"record_{k + 1}"
        save(name, (x, y, L.Z0))
        put(name, name, "wall_prop", (x, y, L.Z0))


# ---- Soffit fascia: a big clock and a TV showing a game ----
CX, CY, CZ = -2.6, 3.42, L.SOFFIT_Z1 + 0.035
TX = 0.9


def clock_and_tv():
    begin()
    clock_px = np.ones((128, 128, 3), np.float32) * np.array(common.srgb((0.95, 0.92, 0.85)))
    yy, xx = np.mgrid[0:128, 0:128]
    d = np.sqrt((xx - 64) ** 2 + (yy - 64) ** 2)
    clock_px[d > 60] = 0.05
    for h in range(12):
        a = h * math.pi / 6
        common.draw_text(clock_px, str(12 if h == 0 else h), int(64 + 46 * math.sin(a)), int(64 + 46 * math.cos(a)) + 4, 1,
                         (0.05, 0.05, 0.05), center=True)
    for (a, ln, wdt) in ((math.radians(-40), 30, 2), (math.radians(110), 44, 1)):
        for t in range(ln):
            px_, py_ = int(64 + t * math.sin(a)), int(64 + t * math.cos(a))
            clock_px[py_ - wdt:py_ + wdt, px_ - wdt:px_ + wdt] = 0.03
    clock_mat = common.image_material("Wall clock", clock_px, roughness=0.3)
    common.cylinder("Clock rim", Vector((CX, CY, CZ + 0.03)), 0.3, 0.06, "z", brass, segments=40)
    # A round face inside the rim (a square picture's dark corners stuck out
    # past the rim): the picture's circle (radius 60 of 64 px) fills it.
    face = common.cylinder("Clock face", Vector((CX, CY, CZ + 0.061)), 0.27, 0.002, "z", clock_mat, segments=40,
                           bevel=0.0)
    common.set_uvs(face, lambda co, n: (0.5 + (co.x - CX) / 0.27 * (60 / 128), 0.5 + (co.y - CY) / 0.27 * (60 / 128)))
    save("wall_clock", (CX, CY, CZ))
    put("wall_clock", "wall_clock", "wall_prop", (CX, CY, CZ))
    begin()
    tv_px = np.zeros((144, 256, 3), np.float32)
    tv_px[:] = common.srgb((0.1, 0.45, 0.15))
    for k in range(0, 256, 32):
        tv_px[:, k:k + 16] *= 1.15
    tv_px[70:74, :] = 0.9
    tv_px[:, 126:130] = 0.9
    grid = np.mgrid[0:144, 0:256]
    ring_d = np.sqrt((grid[1] - 128) ** 2 + (grid[0] - 72) ** 2)
    tv_px[(ring_d > 26) & (ring_d < 30)] = 0.9
    for (px_, py_, col) in ((60, 40, (0.9, 0.1, 0.1)), (90, 100, (0.9, 0.1, 0.1)), (170, 60, (0.1, 0.2, 0.9)),
                            (200, 90, (0.1, 0.2, 0.9)), (140, 80, (1, 1, 1))):
        tv_px[py_ - 4:py_ + 4, px_ - 3:px_ + 3] = col
    tv_px[124:144, 0:120] = common.srgb((0.05, 0.05, 0.1))
    common.draw_text(tv_px, "BAR 2-1 FC", 60, 140, 1, (1, 1, 1), center=True)
    common.draw_text(tv_px, "78:12", 100, 132, 1, common.srgb((1, 0.9, 0.2)), center=True)
    tv_mat = common.glow_image_material("TV picture", tv_px, strength=1.1)
    common.box("TV body", (TX - 0.62, 3.05, CZ), (TX + 0.62, 3.75, CZ + 0.05), black, 0.01)
    common.panel("TV screen", (TX, 3.4, CZ + 0.0505), (1, 0, 0), (0, 1, 0), 1.18, 0.66, tv_mat)
    save("wall_tv", (TX, 3.05, CZ))
    put("wall_tv", "wall_tv", "screen_prop", (TX, 3.05, CZ))


# ---- Chalkboard A-frame by the door ----
def a_frame():
    begin()
    cb_px = np.zeros((256, 160, 3), np.float32)
    cb_px[:] = common.srgb((0.08, 0.1, 0.09))
    cb_px *= (0.85 + 0.3 * common.noise2d(256, 160, 6, 2))[..., None]
    chalk = common.srgb((0.92, 0.92, 0.88))
    for i, (line, sc) in enumerate((("TONIGHT", 2), ("", 1), ("LIVE PIANO", 1), ("9 PM", 2), ("", 1), ("HAPPY HOUR", 1),
                                    ("5 - 7", 2), ("", 1), ("DARTS", 1), ("TOURNEY", 1), ("THURSDAY", 1))):
        if line:
            common.draw_text(cb_px, line, 80, 240 - i * 20, sc, chalk if i % 3 else common.srgb((1.0, 0.75, 0.4)),
                             center=True)
    chalkboard = common.image_material("A-frame chalk", cb_px, roughness=0.9)
    af = Matrix.Translation((1.95, 0, L.Z1 - 1.25)) @ Matrix.Rotation(0.5, 4, "Y")
    AF_H = 1.0 / math.cos(0.2)  # each board's length (the sign stands 1 m tall)
    for side in (-1, 1):
        # Hinged at the top (like a real A-frame), the feet apart at the bottom.
        m = (af @ Matrix.Translation((0, 1.0, 0)) @ Matrix.Rotation(-side * 0.2, 4, "X") @
             Matrix.Translation((0, -AF_H, 0)) @ Matrix.Diagonal((1, AF_H, 1, 1)))
        place(common.box("A-frame", (-0.32, 0.0, -0.012), (0.32, 1.0, 0.012), P.mat("wood"), 0.006), m)
        place(common.quad("Chalk", [(0.27, 0.12, side * 0.013), (-0.27, 0.12, side * 0.013), (-0.27, 0.95, side * 0.013),
                                    (0.27, 0.95, side * 0.013)] if side < 0 else
                          [(-0.27, 0.12, 0.013), (0.27, 0.12, 0.013), (0.27, 0.95, 0.013), (-0.27, 0.95, 0.013)],
                          chalkboard), m)
    common.tube("A-frame hinge", [af @ Vector((-0.3, 0.985, 0)), af @ Vector((0.3, 0.985, 0))], 0.012, P.mat("brass"), sides=8)
    for x in (-0.25, 0.25):
        common.tube("A-frame chain", [af @ Vector((x, 0.35, -0.13)), af @ Vector((x, 0.33, 0.0)), af @ Vector((x, 0.35, 0.13))],
                    0.003, P.mat("steel"), sides=4)
    save("chalk_a_frame", (1.95, 0, L.Z1 - 1.25), math.degrees(0.5))
    put("chalk_a_frame", "chalk_a_frame", "wall_prop", (1.95, 0, L.Z1 - 1.25), math.degrees(0.5))


# ---- Plants ----
def plants():
    for k, (x, y, z, size, pot_r, pot_h, leaves, seed) in enumerate((
            (-1.85, 0.0, L.Z1 - 0.3, 1.6, 0.22, 0.42, 34, 21),     # fig by the bay
            (5.35, 0.0, L.Z1 - 0.35, 1.3, 0.2, 0.38, 26, 22),      # by the door
            (-5.15, L.STEP_H, L.Z1 - 0.35, 1.1, 0.18, 0.34, 22, 23),
            (8.55, 0.0, 6.6, 1.2, 0.2, 0.4, 24, 24))):
        begin()
        P.plant(x, y, z, size=size, pot_r=pot_r, pot_h=pot_h, leaves=leaves, seed=seed)
        name = f"floor_plant_{k + 1}"
        save(name, (x, y, z))
        put(name, name, "plant", (x, y, z))
    for k, (x, z) in enumerate(((-1.2, 7.55), (0.8, 7.55))):  # hanging in the bay window
        begin()
        # A rope hanger you can see (three ropes from the rim to a ring on a
        # ceiling hook), so the pot clearly hangs and doesn't look like a lamp.
        rope = common.plain_material("Hanger rope", (0.62, 0.5, 0.34), 0.0, 0.9)
        top = (x, L.BAY_H - 0.12, z)
        common.torus("Hanger ring", top, 0.025, 0.006, P.mat("brass"), axis="z", segments=14, sides=6)
        common.tube("Ceiling hook", [(x, L.BAY_H, z), (x, top[1] + 0.025, z)], 0.008, P.mat("brass"), sides=6)
        for j in range(3):
            a = j * 2 * math.pi / 3
            rim = (x + 0.125 * math.cos(a), 2.35, z + 0.125 * math.sin(a))
            common.tube("Hanger rope", [rim, (x, top[1] - 0.02, z)], 0.007, rope, sides=5)
        common.lathe("Hanging pot", [(0, 0), (0.1, 0.02), (0.13, 0.16), (0.0, 0.16)], P.mat("terracotta"), 16).data.transform(
            Matrix.Translation((x, 2.2, z)))
        P.plant(x, 2.36, z, size=0.8, leaves=14, seed=int(x * 10) + 40, hanging=True)
        name = f"hanging_plant_{k + 1}"
        save(name, (x, L.BAY_H, z))
        put(name, name, "hanging_plant", (x, L.BAY_H, z))


# ---- Corner speakers, sconces, safety things ----
def speakers_and_sconces():
    begin()  # one speaker model, tilted down, placed in four corners
    m = Matrix.Rotation(-0.25, 4, "X")
    place(common.box("Speaker", (-0.15, -0.22, -0.13), (0.15, 0.22, 0.13), black, 0.015), m)
    place(common.box("Speaker grill", (-0.135, -0.2, 0.13), (0.135, 0.2, 0.135),
                     common.plain_material("Grill cloth", (0.04, 0.04, 0.045), 0.0, 0.9), 0.004), m)
    save("corner_speaker")
    for k, (x, z, yaw) in enumerate(((-8.7, -3.3, math.pi / 4 + math.pi / 2), (8.7, 6.7, -math.pi * 3 / 4),
                                     (-8.7, 6.7, -math.pi / 4 + math.pi), (8.7, -4.0, -math.pi / 4))):
        put(f"corner_speaker-{k + 1}", "corner_speaker", "wall_prop", (x, 3.35, z), math.degrees(yaw))
    begin()  # one sconce model on a wall facing +z
    sconce_glass = common.plain_material("Sconce glass", (1.0, 0.85, 0.6), 0.0, 0.3, emission=(1.0, 0.68, 0.35), strength=3.5)
    place(common.box("Sconce plate", (-0.06, 1.85, 0.0), (0.06, 2.05, 0.015), brass, 0.004), Matrix())
    place(common.tube("Sconce arm", [(0, 1.95, 0.01), (0, 1.97, 0.12), (0, 2.05, 0.16)], 0.007, brass, sides=6), Matrix())
    place(common.lathe("Sconce shade", [(0, 0), (0.035, 0.0), (0.06, 0.12), (0.055, 0.12), (0.0, 0.015)], sconce_glass, 14),
          Matrix.Translation((0, 2.03, 0.16)))
    save("wall_sconce")
    for k, (x, z, facing) in enumerate(((-1.0, L.Z1, "-z"), (-5.45, L.Z1, "-z"), (L.X0, -4.6, "+x"), (L.X0, 3.15, "+x"))):
        put(f"wall_sconce-{k + 1}", "wall_sconce", "glow_prop", (x, 0, z), FACING[facing])
    begin()  # fire extinguisher on its bracket
    red = common.plain_material("Extinguisher red", (0.7, 0.03, 0.02), 0.2, 0.35)
    place(common.cylinder("Extinguisher", Vector((0, 0.95, 0.1)), 0.075, 0.5, "y", red, segments=16), Matrix())
    place(common.sphere("Ext top", Vector((0, 1.2, 0.1)), 0.075, red, 16, 8), Matrix())
    place(common.box("Ext valve", (-0.02, 1.25, 0.07), (0.02, 1.32, 0.13), black, 0.004), Matrix())
    place(common.tube("Ext hose", [(0.02, 1.3, 0.12), (0.09, 1.2, 0.14), (0.08, 0.9, 0.15)], 0.008, black, sides=6), Matrix())
    place(common.box("Ext bracket", (-0.06, 0.85, 0.0), (0.06, 1.05, 0.025), black, 0.004), Matrix())
    save("fire_extinguisher")
    for k, (x, z, facing) in enumerate(((2.75, L.Z0, "+z"), (-5.6, L.Z1, "-z"))):
        put(f"fire_extinguisher-{k + 1}", "fire_extinguisher", "wall_prop", (x, 0, z), FACING[facing])
    begin()  # smoke detector under the ceiling
    common.cylinder("Smoke detector", Vector((0, -0.0175, 0)), 0.07, 0.035, "y", P.mat("white"), segments=18)
    common.sphere("Detector LED", Vector((0.03, -0.04, 0)), 0.004,
                  common.plain_material("Detector LED", (0.2, 1, 0.2), 0.0, 0.3, emission=(0.2, 1.0, 0.2), strength=2.2),
                  6, 4)
    save("smoke_detector")
    for k, (x, z) in enumerate(((-4.0, 0.0), (3.0, 3.0), (-6.0, 5.0), (4.0, -2.0), (-1.0, 5.5))):
        put(f"smoke_detector-{k + 1}", "smoke_detector", "wall_prop", (x, L.H, z))
    begin()  # a light switch on its wall plate
    place(common.box("Wall plate", (-0.04, -0.06, 0.0), (0.04, 0.06, 0.008), P.mat("white"), 0.003), Matrix())
    place(common.box("Switch", (-0.008, -0.02, 0.008), (0.008, 0.02, 0.016), P.mat("white"), 0.002), Matrix())
    save("wall_switch")
    for k, (x, z, facing, y) in enumerate(((2.2, L.Z1, "-z", 1.25), (4.35, L.Z1, "-z", 0.35), (L.X1, 0.7, "-x", 1.25),
                                           (-5.6, L.Z1, "-z", 0.35), (1.1, L.Z0, "+z", 0.35))):
        put(f"wall_switch-{k + 1}", "wall_switch", "wall_prop", (x, y, z), FACING[facing])


# ---- By the door: coat hooks with coats, umbrella stand, a trash can ----
def by_the_door():
    begin()
    hm = wall_frame(0, L.Z1, "-z")
    HX0, HX1, HY = -4.35 - 0.1, -4.35 + 0.95, 1.75   # local x (world x = -local)
    place(common.box("Coat rail", (HX0, HY - 0.06, 0.0), (HX1, HY + 0.06, 0.025), P.mat("dark_wood"), 0.008), hm)
    coat_cols = [common.pbr_material(f"Coat {i}", tex_dir, "fabric_pattern_07", tint=c, size=256, saturation=0.3)
                 for i, c in enumerate([(0.25, 0.2, 0.15), (0.12, 0.15, 0.25), (0.4, 0.1, 0.08)])]
    for k in range(5):
        x = HX0 + 0.1 + k * (HX1 - HX0 - 0.2) / 4
        place(common.tube("Hook", [(x, HY, 0.02), (x, HY - 0.02, 0.08), (x, HY + 0.03, 0.1)], 0.006, brass, sides=6), hm)
        if k in (0, 2, 3):
            c = coat_cols[[0, 1, 2, 2][k % 4] if k < 4 else 0]
            coat = common.lathe("Coat", [(0.0, 0.0), (0.2, 0.02), (0.22, 0.4), (0.18, 0.75), (0.08, 0.92), (0.0, 0.95)], c, 14)
            place(coat, hm @ Matrix.Translation((x, HY - 0.95, 0.13)) @ Matrix.Diagonal((1.0, 1.0, 0.45, 1.0)))
    place(common.lathe("Hat", [(0, 0), (0.17, 0), (0.17, 0.01), (0.1, 0.012), (0.095, 0.11), (0.0, 0.12)], black, 18),
          hm @ Matrix.Translation((HX0 + 0.1 + 1 * (HX1 - HX0 - 0.2) / 4, HY + 0.07, 0.12)) @ Matrix.Rotation(0.3, 4, "X"))
    origin = (-(HX0 + HX1) / 2, HY, L.Z1)
    save("coat_rack", origin, 180)
    put("coat_rack", "coat_rack", "wall_prop", origin, 180)
    begin()
    us = Matrix.Translation((4.35 + 0.25, 0, L.Z1 - 0.3))
    place(common.lathe("Umbrella stand", [(0, 0), (0.12, 0), (0.13, 0.55), (0.12, 0.55), (0.11, 0.02), (0, 0.02)],
                       P.mat("brass"), 18), us)
    for k, col in enumerate(((0.05, 0.05, 0.05), (0.5, 0.05, 0.05))):
        um = common.plain_material(f"Umbrella {k}", col, 0.0, 0.5)
        place(common.lathe("Umbrella", [(0, 0.15), (0.03, 0.25), (0.05, 0.75), (0.015, 0.85), (0, 0.85)], um, 10),
              us @ Matrix.Translation((0.03 * (k * 2 - 1), 0, 0)) @ Matrix.Rotation(0.1 * (k * 2 - 1), 4, "Z"))
        place(common.tube("Umbrella handle", [(0.0, 0.85, 0), (0, 0.98, 0), (0.04, 1.02, 0), (0.06, 0.98, 0)], 0.008, black,
                          sides=6), us @ Matrix.Translation((0.03 * (k * 2 - 1), 0, 0)) @ Matrix.Rotation(0.1 * (k * 2 - 1), 4, "Z"))
    save("umbrella_stand", (4.6, 0, L.Z1 - 0.3))
    put("umbrella_stand", "umbrella_stand", "wall_prop", (4.6, 0, L.Z1 - 0.3))
    begin()
    common.lathe("Trash can", [(0, 0), (0.17, 0), (0.19, 0.6), (0.2, 0.62), (0, 0.62)], P.mat("steel"), 20)
    save("trash_can")
    put("trash_can", "trash_can", "wall_prop", (1.95, 0, L.Z1 - 0.3))


# ---- Storage corner by doorway A: beer crates and a keg ----
def storage():
    begin()  # one crate of beer, placed four times
    crate = common.plain_material("Crate plastic", (0.6, 0.08, 0.05), 0.0, 0.5)
    common.box("Crate", (-0.2, 0.0, -0.15), (0.2, 0.28, 0.15), crate, 0.01)
    for i in range(4):
        for j in range(3):
            P.bottle(-0.15 + i * 0.1, 0.04, -0.1 + j * 0.1, "beer", color=rng.choice([0, 9]), segments=6)
    save("beer_crate")
    # The stacked one stands on the crates under it (it floated 2 cm above).
    for k, (x, z, y, a) in enumerate(((4.95, L.Z0 + 0.3, 0.0, 0.0), (5.4, L.Z0 + 0.32, 0.0, 0.05),
                                      (5.15, L.Z0 + 0.3, 0.28, -0.1), (5.65, L.Z0 + 0.75, 0.0, 0.3))):
        put(f"beer_crate-{k + 1}", "beer_crate", "wall_prop", (x, y, z), math.degrees(a))
    begin()
    keg = Matrix.Translation((0, 0, 0))
    place(common.lathe("Keg", [(0, 0), (0.18, 0), (0.2, 0.05), (0.2, 0.2), (0.205, 0.22), (0.2, 0.24), (0.2, 0.36),
                               (0.205, 0.38), (0.2, 0.4), (0.2, 0.55), (0.18, 0.6), (0.0, 0.6)], P.mat("steel"), 24), keg)
    place(common.cylinder("Keg coupler", Vector((0, 0.63, 0)), 0.04, 0.06, "y", black, segments=12), keg)
    save("keg")
    put("keg", "keg", "wall_prop", (5.75, 0, L.Z0 + 0.3))


# ---- Bay window: a little table with a candle and two drinks ----
def bay_table():
    begin()
    common.cylinder("Bay table", Vector((-0.2, 0.55, 7.25)), 0.28, 0.03, "y", P.mat("dark_wood"), segments=24)
    common.cylinder("Bay table post", Vector((-0.2, 0.27, 7.25)), 0.03, 0.54, "y", black, segments=10)
    common.cylinder("Bay table foot", Vector((-0.2, 0.01, 7.25)), 0.2, 0.02, "y", black, segments=20)
    P.candle(-0.2, 0.565, 7.25, h=0.06, r=0.03)
    P.record_glasses = []  # the drinks are their own things (J72.2)
    P.glass(-0.05, 0.565, 7.15, "coupe", "pink")
    P.glass(-0.35, 0.565, 7.35, "wine", "white_wine")
    props_lib.save_records(P)
    P.record_glasses = None
    save("bay_table", (-0.2, 0, 7.25))
    put("bay_table", "bay_table", "wall_prop", (-0.2, 0, 7.25))


string_lights()
neon_signs()
gallery()
collector_wall()
clock_and_tv()
a_frame()
plants()
speakers_and_sconces()
by_the_door()
storage()
bay_table()
if places_path:
    with open(places_path, "w") as f:
        json.dump(PLACES, f, indent=1)
print("PLACES", len(PLACES))
