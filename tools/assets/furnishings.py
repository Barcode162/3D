# Our own versions of the furnishings that were photo-scans from Poly Haven
# (J35; request 2026-11-01 #5: the scans broke the model spec,
# looped/draw/model_spec.h): the round bar stool, the Chesterfield sofa, the
# green armchair, the mantel clock, the framed painting and the vase. Each
# keeps the scan's size, colours and look in broad strokes, built from
# solid, joined shapes with few seams, so it simplifies far away.
#
#   blender -b --factory-startup --python tools/assets/furnishings.py -- <texture dir> <out dir> [names...]
#
# Model space: the middle of its footprint on the floor, y up, front +z.
import math
import os
import sys

import numpy as np
from mathutils import Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402

args = sys.argv[sys.argv.index("--") + 1:]
tex_dir, out_dir = args[:2]
which = args[2:] or ["bar_stool", "chesterfield_sofa", "armchair", "mantel_clock", "painting", "vase"]


def dark_wood(name, tint=(0.55, 0.36, 0.24), size=512):
    return common.pbr_material(name, tex_dir, "dark_wood", tint=tint, size=size)


def bar_stool():
    """A round wooden bar stool (0.82 m): a thick seat with a moulded edge,
    four square legs, a foot ring and an apron ring under the seat."""
    common.reset_scene()
    wood = dark_wood("Stool wood")
    brass = common.plain_material("Stool brass", (0.75, 0.55, 0.3), 1.0, 0.35)
    top = 0.823
    common.cylinder("Seat", Vector((0, top - 0.025, 0)), 0.25, 0.05, "y", wood, segments=24, bevel=0.012)
    common.cylinder("Seat band", Vector((0, top - 0.065, 0)), 0.235, 0.03, "y", wood, segments=24, bevel=0.006)
    leg = 0.155
    for k in range(4):
        a = math.pi / 4 + k * math.pi / 2
        x, z = math.cos(a) * leg, math.sin(a) * leg
        common.box("Leg", (x - 0.022, 0.0, z - 0.022), (x + 0.022, top - 0.08, z + 0.022), wood, bevel=0.004)
    common.torus("Foot ring", Vector((0, 0.24, 0)), leg, 0.012, brass, "y", 24, 6)
    common.torus("Apron ring", Vector((0, top - 0.14, 0)), leg, 0.014, wood, "y", 24, 6)
    common.finish_and_export(os.path.join(out_dir, "bar_stool.glb"), uv_scale={})


def chesterfield_sofa():
    """A black leather Chesterfield (1.99 x 0.78 x 0.9 m): rolled arms as
    high as the back, a deep seat cushion, buttoned back, turned feet."""
    common.reset_scene()
    leather = common.pbr_material("Sofa leather", tex_dir, "fabric_leather_02", tint=(0.11, 0.1, 0.1), size=512)
    wood = dark_wood("Sofa feet", tint=(0.45, 0.2, 0.12), size=256)
    button = common.plain_material("Sofa button", (0.02, 0.02, 0.02), 0.0, 0.45)
    W, D, H = 1.986, 0.9, 0.78
    feet = 0.1
    common.box("Base", (-W / 2 + 0.12, feet, -D / 2 + 0.05), (W / 2 - 0.12, 0.38, D / 2 - 0.04), leather, bevel=0.04, segments=3)
    common.box("Seat", (-W / 2 + 0.16, 0.38, -D / 2 + 0.2), (W / 2 - 0.16, 0.47, D / 2 - 0.02), leather, bevel=0.04, segments=3)
    common.box("Back", (-W / 2 + 0.12, 0.38, -D / 2), (W / 2 - 0.12, H - 0.07, -D / 2 + 0.2), leather, bevel=0.05, segments=3)
    for side in (-1, 1):  # rolled arms
        x = side * (W / 2 - 0.1)
        common.box("Arm", (x - 0.1, feet, -D / 2), (x + 0.1, H - 0.12, D / 2), leather, bevel=0.04, segments=3)
        common.cylinder("Arm roll", Vector((x + side * 0.02, H - 0.12, 0)), 0.11, D, "z", leather, segments=16, bevel=0.02)
    common.cylinder("Back roll", Vector((0, H - 0.08, -D / 2 + 0.1)), 0.08, W - 0.22, "x", leather, segments=16, bevel=0.02)
    for row, y in enumerate((0.5, 0.62)):  # the buttons of the tufting
        for k in range(9 - row):
            x = (k - (8 - row) / 2) * 0.2
            common.sphere("Button", Vector((x, y, -D / 2 + 0.205)), 0.012, button, 6, 4)
    for x in (-W / 2 + 0.15, W / 2 - 0.15):
        for z in (-D / 2 + 0.08, D / 2 - 0.08):
            common.lathe("Foot", [(0.0, 0.0), (0.03, 0.0), (0.04, 0.05), (0.035, feet), (0.0, feet)], wood, 10)
            bpy_obj = [o for o in __import__("bpy").context.scene.objects if o.name.startswith("Foot")][-1]
            bpy_obj.location = (x, 0, z)
    common.finish_and_export(os.path.join(out_dir, "chesterfield_sofa.glb"), uv_scale={})


def armchair():
    """A green upholstered armchair in a dark carved frame (1.06 m): a
    padded seat and back, curved arms, slightly bowed legs."""
    common.reset_scene()
    fabric = common.plain_material("Chair velvet", (0.045, 0.085, 0.07), 0.0, 0.9)
    wood = dark_wood("Chair wood", tint=(0.35, 0.22, 0.16))
    s = 0.33
    seat_y = 0.44
    for (x, z) in ((-s + 0.04, s - 0.04), (s - 0.04, s - 0.04)):  # front legs, bowed out
        common.tube("Front leg", [(x, 0.0, z + 0.02), (x * 1.05, 0.15, z + 0.03), (x, 0.3, z), (x * 0.95, seat_y - 0.02, z - 0.02)],
                    0.022, wood, sides=8)
    for x in (-s + 0.05, s - 0.05):  # back legs run up into the back frame
        common.tube("Back post", [(x, 0.0, -s + 0.02), (x, seat_y, -s + 0.05), (x * 0.92, 1.0, -s - 0.02)], 0.024, wood, sides=8)
        common.tube("Arm", [(x * 1.02, 0.66, -s + 0.06), (x * 1.08, 0.68, 0.0), (x * 1.05, 0.64, s - 0.06), (x, seat_y + 0.02, s - 0.04)],
                    0.022, wood, sides=8)
        common.box("Arm pad", (x - 0.03 if x > 0 else x - 0.03, 0.66, -0.1), (x + 0.03, 0.7, 0.12), fabric, bevel=0.012)
    common.tube("Crest", [(-s + 0.04, 0.98, -s - 0.01), (-0.12, 1.03, -s - 0.03), (0.0, 1.059, -s - 0.035), (0.12, 1.03, -s - 0.03),
                          (s - 0.04, 0.98, -s - 0.01)], 0.022, wood, sides=8)
    common.box("Seat frame", (-s + 0.02, seat_y - 0.06, -s + 0.02), (s - 0.02, seat_y, s - 0.02), wood, bevel=0.01)
    common.box("Seat cushion", (-s + 0.04, seat_y, -s + 0.06), (s - 0.04, seat_y + 0.08, s - 0.03), fabric, bevel=0.03, segments=3)
    common.box("Back cushion", (-s + 0.07, seat_y + 0.06, -s + 0.0), (s - 0.07, 0.97, -s + 0.07), fabric, bevel=0.03, segments=3)
    common.finish_and_export(os.path.join(out_dir, "armchair.glb"), uv_scale={})


def mantel_clock():
    """A tambour mantel clock (0.34 x 0.16 m): an arched wooden case on a
    stepped plinth, a cream dial in a brass bezel, black hands."""
    common.reset_scene()
    wood = dark_wood("Clock wood", tint=(0.5, 0.28, 0.18), size=256)
    brass = common.plain_material("Clock brass", (0.85, 0.65, 0.35), 1.0, 0.3)
    dial = common.plain_material("Clock dial", (0.88, 0.84, 0.74), 0.0, 0.5)
    ink = common.plain_material("Clock hands", (0.02, 0.02, 0.02), 0.0, 0.4)
    W, D = 0.342, 0.126
    common.box("Plinth", (-W / 2, 0.0, -D / 2), (W / 2, 0.02, D / 2), wood, bevel=0.004)
    common.box("Plinth step", (-W / 2 + 0.02, 0.02, -D / 2 + 0.012), (W / 2 - 0.02, 0.032, D / 2 - 0.012), wood, bevel=0.003)
    # The tambour: a low arch rising to a dome in the middle, extruded front to back.
    arch = [(-0.14, 0.032), (0.14, 0.032), (0.14, 0.07)]
    arch += [(0.14 - 0.08 * (1 - math.cos(a)) / 1, 0.07 + 0.02 * math.sin(a)) for a in np.linspace(0.3, 1.2, 4)]
    arch += [(0.075 * math.cos(a), 0.085 + 0.075 * math.sin(a)) for a in np.linspace(0.15, math.pi - 0.15, 12)]
    arch += [(-0.14 + 0.08 * (1 - math.cos(a)), 0.07 + 0.02 * math.sin(a)) for a in np.linspace(1.2, 0.3, 4)]
    arch += [(-0.14, 0.07)]
    body = common.prism("Case", [(x, y) for x, y in arch], -D / 2 + 0.02, D / 2 - 0.02, wood, bevel=0.003)
    # (prism extrudes a floor outline along y; turn it to stand: outline in x-y, depth along z)
    from mathutils import Matrix
    body.data.transform(Matrix.Rotation(-math.pi / 2, 4, "X"))
    common.torus("Bezel", Vector((0, 0.095, D / 2 - 0.018)), 0.052, 0.006, brass, "z", 24, 6)
    common.cylinder("Dial", Vector((0, 0.095, D / 2 - 0.02)), 0.05, 0.004, "z", dial, segments=24, bevel=0.0)
    common.box("Hour hand", (-0.002, 0.095, D / 2 - 0.016), (0.002, 0.125, D / 2 - 0.014), ink, bevel=0.0)
    common.box("Minute hand", (0.0, 0.093, D / 2 - 0.015), (0.04, 0.097, D / 2 - 0.013), ink, bevel=0.0)
    common.finish_and_export(os.path.join(out_dir, "mantel_clock.glb"), uv_scale={})


def _landscape(w=256, h=192, seed=3):
    """A small oil landscape (our own, painted by numbers): a warm sky, far
    blue hills, a green valley, a cottage and dark trees, a brown foreground."""
    rng = np.random.default_rng(seed)
    y = np.linspace(0, 1, h)[:, None]  # 0 at the bottom (v = 0)
    x = np.linspace(0, 1, w)[None, :]
    px = np.zeros((h, w, 3), np.float32)
    sky = np.array([0.95, 0.82, 0.62]) * (1 - y) + np.array([0.55, 0.65, 0.78]) * y
    px[:] = sky[..., None, :].repeat(w, 1) if sky.ndim == 2 else sky
    px[:] = (np.array([0.95, 0.82, 0.62])[None, None] * (1 - y[..., None]) + np.array([0.52, 0.62, 0.76])[None, None] * y[..., None])
    hills = 0.55 + 0.06 * np.sin(x * 7 + 1) + 0.03 * np.sin(x * 19)
    px[y.repeat(w, 1) < hills] = [0.42, 0.5, 0.58]
    valley = 0.4 + 0.05 * np.sin(x * 5 + 2)
    px[y.repeat(w, 1) < valley] = [0.36, 0.45, 0.22]
    ground = 0.2 + 0.08 * np.sin(x * 3)
    px[y.repeat(w, 1) < ground] = [0.38, 0.28, 0.16]
    for cx, cy, r in ((0.2, 0.55, 0.12), (0.28, 0.6, 0.1), (0.75, 0.45, 0.07)):  # trees
        d = ((x - cx) * 1.3) ** 2 + (y - cy) ** 2
        px[d < r * r] = [0.15, 0.22, 0.1]
    house = (x > 0.36) & (x < 0.5) & (y > 0.38) & (y < 0.5)
    px[house] = [0.72, 0.6, 0.48]
    roof = (y >= 0.5) & (y < 0.58) & (np.abs(x - 0.43) < (0.58 - y) * 1.2)
    px[roof] = [0.45, 0.22, 0.14]
    # Soften the edges like wet paint (a few box blurs), then dabs of brush texture.
    for _ in range(6):
        px = (px + np.roll(px, 1, 0) + np.roll(px, -1, 0) + np.roll(px, 1, 1) + np.roll(px, -1, 1)) / 5
    dabs = rng.normal(0, 1, (h // 3 + 1, w // 3 + 1, 1)).repeat(3, 0).repeat(3, 1)[:h, :w]
    px = px * (1 + 0.025 * dabs) + rng.normal(0, 0.005, px.shape)
    px = px * 0.9 + 0.1 * np.array([0.6, 0.45, 0.25])  # an old varnish
    return np.clip(px, 0, 1).astype(np.float32) ** 2.2  # sRGB-ish colours to linear


def painting():
    """A landscape in a carved gilt frame (0.97 x 0.75 m), hung by its back."""
    common.reset_scene()
    gold = common.plain_material("Frame gilt", (0.83, 0.62, 0.3), 1.0, 0.35)
    canvas = common.image_material("Painting canvas", _landscape(), roughness=0.6)
    W, H, d = 0.974, 0.75, 0.032
    f = 0.07
    common.box("Frame top", (-W / 2, H - f, -d / 2), (W / 2, H, d / 2), gold, bevel=0.008)
    common.box("Frame bottom", (-W / 2, 0.0, -d / 2), (W / 2, f, d / 2), gold, bevel=0.008)
    common.box("Frame left", (-W / 2, f, -d / 2), (-W / 2 + f, H - f, d / 2), gold, bevel=0.008)
    common.box("Frame right", (W / 2 - f, f, -d / 2), (W / 2, H - f, d / 2), gold, bevel=0.008)
    common.box("Back", (-W / 2 + f, f, -d / 2), (W / 2 - f, H - f, -d / 2 + 0.006), gold, bevel=0.0)
    common.panel("Canvas", Vector((0, H / 2, 0.0)), Vector((1, 0, 0)), Vector((0, 1, 0)), W - 2 * f + 0.004, H - 2 * f + 0.004, canvas)
    common.finish_and_export(os.path.join(out_dir, "painting.glb"), uv_scale={"Painting canvas": 0})


def vase():
    """A white ceramic vase (0.27 m): a full belly, a narrow neck, a rolled lip, walls with a thickness."""
    common.reset_scene()
    glaze = common.plain_material("Vase glaze", (0.86, 0.85, 0.8), 0.0, 0.35)
    outer = [(0.0, 0.0), (0.055, 0.0), (0.075, 0.03), (0.092, 0.1), (0.088, 0.16), (0.07, 0.21), (0.062, 0.24), (0.07, 0.262), (0.075, 0.27)]
    inner = [(0.066, 0.27), (0.06, 0.262), (0.053, 0.24), (0.06, 0.21), (0.078, 0.16), (0.082, 0.1), (0.066, 0.035), (0.0, 0.012)]
    common.lathe("Vase", outer + inner, glaze, 16)
    common.finish_and_export(os.path.join(out_dir, "vase.glb"), uv_scale={})


os.makedirs(out_dir, exist_ok=True)
for name in which:
    globals()[name]()
