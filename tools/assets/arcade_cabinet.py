# An upright arcade cabinet (upgrade plan item 4: the platformer machine;
# the same cabinet can carry other games later with another marquee and
# screen): shaped side panels with art and glowing edge trim, a lit marquee,
# a tilted screen in a black bezel, a control panel with a ball-top
# joystick and two buttons, a coin door with lit coin slots, a kick plate.
#
#   blender -b --factory-startup --python tools/assets/arcade_cabinet.py -- <texture dir> <out.glb> [title]
#
# Model space: x right, +z toward the player (the front), floor y = 0, the
# middle of its footprint at the origin. 0.74 wide, 0.86 deep, 1.86 tall.
import math
import os
import sys

import bmesh
import numpy as np
from mathutils import Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402

args = sys.argv[sys.argv.index("--") + 1:]
tex_dir, out_path = args[:2]
TITLE = args[2] if len(args) > 2 else "PLATFORMER"
common.reset_scene()

W = 0.74            # outside width
SIDE = 0.025        # side panel thickness
IN0, IN1 = -W / 2 + SIDE, W / 2 - SIDE
# Side profile (z, y), front = +z, counter-clockwise.
PROFILE = [(-0.43, 0.0), (0.40, 0.0), (0.40, 0.80), (0.47, 0.92), (0.47, 0.98), (0.18, 1.07), (0.21, 1.12),
           (0.04, 1.60), (0.25, 1.64), (0.25, 1.86), (-0.43, 1.86)]

PINK, CYAN, YELLOW = (1.0, 0.12, 0.62), (0.1, 0.85, 1.0), (1.0, 0.85, 0.1)


def side_art(w=256, h=512):
    """Purple-to-blue fade with neon stripes and a little jumping figure."""
    yy, xx = np.mgrid[0:h, 0:w] / np.array([h, w], np.float32)[:, None, None]
    img = np.stack([0.18 + 0.25 * yy, 0.02 + 0.03 * yy, 0.35 - 0.1 * yy], -1).astype(np.float32)
    for k, c in enumerate((PINK, CYAN, YELLOW)):
        band = np.abs((xx * 0.8 + yy) - (0.55 + 0.09 * k)) < 0.018
        img[band] = common.srgb(c)
    # A blocky jumper (the platformer's hero) mid-jump.
    cx, cy, s = int(w * 0.5), int(h * 0.7), 10
    for dx, dy in ((0, 0), (0, 1), (0, 2), (-1, 1), (1, 1), (-1, -1), (1, -1), (0, 3)):
        img[cy + dy * s:cy + dy * s + s, cx + dx * s - s // 2:cx + dx * s + s // 2] = common.srgb((0.95, 0.95, 0.95))
    return img


def screen_image(w=320, h=256):
    """The attract screen: a grey room with a black grid (like the game),
    a pillar and a ramp, a spinning health pack, and the title."""
    img = np.zeros((h, w, 3), np.float32)
    yy, xx = np.mgrid[0:h, 0:w]
    hor = int(h * 0.55)
    img[:] = common.srgb((0.72, 0.74, 0.78))  # walls
    floor = yy < hor
    img[floor] = common.srgb((0.82, 0.82, 0.82))
    # Perspective grid on the floor.
    for k in range(-10, 11):
        x_far, x_near = w / 2 + k * 14, w / 2 + k * 70
        for y in range(0, hor):
            t = y / hor
            x = int(x_near + (x_far - x_near) * t)
            if 0 <= x < w:
                img[y, x] = (0.05, 0.05, 0.05)
    for k in range(1, 9):
        y = int(hor * (1 - 1 / (1 + 0.45 * k)))
        img[y, :] = (0.05, 0.05, 0.05)
    # The pillar and the ramp.
    img[hor - 20:hor + 30, 190:250] = common.srgb((0.62, 0.62, 0.64))
    for x in range(120, 190):
        top = hor - 20 + int((190 - x) * 0.55)
        img[top:hor - 20 + 40, x] = common.srgb((0.66, 0.66, 0.68)) if top < hor + 20 else img[top:hor + 20, x]
    # Health pack: white box with a red cross.
    img[40:62, 60:82] = (0.95, 0.95, 0.95)
    img[47:55, 63:79] = img[43:59, 67:75] = common.srgb((0.9, 0.1, 0.1))
    # Title and "tap to play".
    common.draw_text(img, TITLE, w // 2 + 2, h - 50 + 2, 4, (0.02, 0.02, 0.05), center=True)
    common.draw_text(img, TITLE, w // 2, h - 50, 4, common.srgb(PINK), center=True)
    common.draw_text(img, "TAP TO PLAY", w // 2, 18, 2, common.srgb((0.05, 0.05, 0.1)), center=True)
    return img


def marquee_image(w=512, h=128):
    px = np.zeros((h, w, 3), np.float32)
    yy = np.mgrid[0:h, 0:w][0] / h
    px[:] = np.stack([0.05 + 0.15 * yy, 0.0 * yy, 0.12 + 0.2 * yy], -1)
    scale = 7 if len(TITLE) > 8 else 11
    for off in (3, 2, 1):
        for dx in (-off, off):
            for dy in (-off, off):
                common.draw_text(px, TITLE, w // 2 + dx * 2, (h + 7 * scale) // 2 + dy * 2, scale,
                                 tuple(c * 0.25 for c in common.srgb(CYAN)), center=True)
    common.draw_text(px, TITLE, w // 2, (h + 7 * scale) // 2, scale, common.srgb((1.0, 1.0, 1.0)), center=True)
    px[:6] = px[-6:] = common.srgb(PINK)
    return px


body = common.plain_material("Cabinet black", (0.02, 0.02, 0.025), 0.0, 0.45)
art = common.image_material("Cabinet side art", side_art(), roughness=0.35)
trim = common.plain_material("Cabinet T-molding", CYAN, 0.0, 0.3, emission=CYAN, strength=4.0)
screen = common.glow_image_material("Cabinet screen", screen_image(), strength=1.6, base_scale=0.2, roughness=0.08)
marquee = common.glow_image_material("Cabinet marquee", marquee_image(), strength=2.5, base_scale=0.3)
panel_art = common.plain_material("Control panel", (0.08, 0.02, 0.12), 0.1, 0.3)
chrome = common.plain_material("Cabinet chrome", (0.8, 0.8, 0.82), 1.0, 0.2)
red_ball = common.plain_material("Joystick ball", (0.85, 0.05, 0.05), 0.0, 0.25)
btn = {k: common.plain_material(f"Button {k}", c, 0.0, 0.25, emission=c, strength=2.0)
       for k, c in (("pink", PINK), ("yellow", YELLOW))}
coin = common.plain_material("Coin slot light", (1.0, 0.15, 0.05), 0.0, 0.3, emission=(1.0, 0.15, 0.05), strength=4.0)
metal = common.plain_material("Coin door", (0.25, 0.25, 0.27), 0.9, 0.35)


def side_panel(x0, x1, name, mat):
    """The side profile extruded across x0..x1."""
    bm = bmesh.new()
    a = [bm.verts.new((x0, y, z)) for z, y in PROFILE]
    b = [bm.verts.new((x1, y, z)) for z, y in PROFILE]
    bm.faces.new(list(reversed(a)))
    bm.faces.new(b)
    n = len(PROFILE)
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new((a[i], a[j], b[j], b[i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    uv = bm.loops.layers.uv.verify()
    for f in bm.faces:
        for loop in f.loops:
            co = loop.vert.co
            loop[uv].uv = ((co.z + 0.43) / 0.9, co.y / 1.86)
    obj = common.mesh_object(name, bm, mat)
    obj["keep_uv"] = True
    return obj


side_panel(-W / 2, -W / 2 + SIDE, "Side", art)
side_panel(W / 2 - SIDE, W / 2, "Side", art)
# Glowing T-molding along the side panels' front edges.
for x in (-W / 2 - 0.003, W / 2 + 0.003):
    pts = [Vector((x, y, z)) for z, y in PROFILE[1:10]]
    common.tube("T-molding", pts, 0.008, trim, sides=6, caps=True)


def slab(name, z0, y0, z1, y1, depth, mat):
    """A panel spanning between the sides from (z0, y0) to (z1, y1), `depth` thick (behind)."""
    d = Vector((0, z1 - z0, -(y1 - y0))).normalized() * 0  # (kept simple: boxes for flat pieces)
    del d
    bm = bmesh.new()
    t = Vector((0, y1 - y0, z1 - z0)).normalized()
    n = Vector((0, -t.z, t.y))  # outward (toward the front/up)
    pts = []
    for (z, y) in ((z0, y0), (z1, y1)):
        p = Vector((0, y, z))
        pts.append(p)
        pts.append(p - n * depth)
    corners = []
    for x in (IN0, IN1):
        corners.append([bm.verts.new((x, p.y, p.z)) for p in pts])
    a, b = corners
    for quad in ((0, 2, 3, 1),):
        pass
    faces = [(a[0], a[2], b[2], b[0]), (a[1], b[1], b[3], a[3]), (a[0], b[0], b[1], a[1]), (a[2], a[3], b[3], b[2]),
             (a[0], a[1], a[3], a[2]), (b[0], b[2], b[3], b[1])]
    for f in faces:
        bm.faces.new(f)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return common.mesh_object(name, bm, mat)


# Back, top, kick plate, the front below the panel.
common.box("Back", (IN0, 0.02, -0.43), (IN1, 1.86, -0.40), body, 0.0)
common.box("Top", (IN0, 1.84, -0.43), (IN1, 1.86, 0.25), body, 0.0)
common.box("Kick plate", (IN0, 0.0, 0.36), (IN1, 0.12, 0.40), common.plain_material("Kick plate", (0.3, 0.3, 0.32), 0.9, 0.4),
           0.0)
common.box("Front", (IN0, 0.12, 0.37), (IN1, 0.80, 0.40), body, 0.0)
# Coin door with two lit slots and return buttons.
common.box("Coin door", (-0.15, 0.30, 0.40), (0.15, 0.66, 0.415), metal, 0.004)
for x in (-0.07, 0.07):
    common.box("Coin slot", (x - 0.012, 0.52, 0.415), (x + 0.012, 0.58, 0.422), coin, 0.0)
    common.box("Coin return", (x - 0.03, 0.38, 0.415), (x + 0.03, 0.42, 0.42), body, 0.002)
# Control panel: the slanted deck and its front lip.
slab("Panel front", 0.40, 0.80, 0.47, 0.92, 0.03, body)
slab("Panel lip", 0.47, 0.92, 0.47, 0.98, 0.03, chrome)
slab("Control panel", 0.47, 0.98, 0.18, 1.07, 0.03, panel_art)
deck_t = Vector((0, 1.07 - 0.98, 0.18 - 0.47)).normalized()
deck_n = Vector((0, 0.47 - 0.18, 1.07 - 0.98)).normalized()  # up-ish, toward the player


def on_deck(x, s):
    """A point on the deck's surface, `s` metres back from the front lip."""
    return Vector((x, 0.98, 0.47)) + deck_t * s + deck_n * 0.001


# Joystick: base plate, shaft, red ball top; two buttons.
jb = on_deck(-0.16, 0.12)
common.cylinder("Joystick base", jb, 0.035, 0.006, "y", body, segments=16)
common.cylinder("Joystick shaft", jb + Vector((0, 0.045, 0)), 0.006, 0.09, "y", chrome, segments=8)
common.sphere("Joystick ball", jb + Vector((0, 0.1, 0)), 0.02, red_ball, 16, 10)
for i, (x, k) in enumerate(((0.08, "pink"), (0.17, "yellow"))):
    p = on_deck(x, 0.13 + 0.01 * i)
    common.cylinder("Button", p + Vector((0, 0.006, 0)), 0.022, 0.012, "y", btn[k], segments=18)
    common.cylinder("Button ring", p, 0.027, 0.004, "y", chrome, segments=18)
# Screen: bezel slab and the glowing picture just in front of it.
slab("Bezel", 0.21, 1.12, 0.04, 1.60, 0.03, body)
s_t = Vector((0, 1.60 - 1.12, 0.04 - 0.21)).normalized()
s_n = Vector((0, 0.21 - 0.04, 1.60 - 1.12)).normalized()
s_c = (Vector((0, 1.12, 0.21)) + Vector((0, 1.60, 0.04))) / 2 + s_n * 0.002
common.panel("Screen", s_c, (1, 0, 0), s_t, IN1 - IN0 - 0.07, 0.42, screen)
# Marquee: the lit title above the screen.
common.panel("Marquee", Vector((0, 1.75, 0.252)), (1, 0, 0), (0, 1, 0), IN1 - IN0, 0.2, marquee)
common.box("Marquee shelf", (IN0, 1.62, 0.04), (IN1, 1.64, 0.25), body, 0.0)
# Speaker grilles under the marquee.
for x in (-0.2, 0.2):
    common.cylinder("Speaker", Vector((x, 1.63, 0.15)), 0.04, 0.004, "y", metal, segments=16)

common.finish_and_export(out_path, uv_scale={})
