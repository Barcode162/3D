# A racing simulator rig (docs/BAR_REDESIGN.md, placed twice on the right
# wall): aluminum profile frame, a racing bucket seat with harness slots, a
# direct-drive wheel base and wheel (rim, spokes, buttons, a little display,
# shift paddles), three pedals, an H-shifter and a handbrake, triple
# monitors on a stand showing a race (road to the horizon, grandstands,
# HUD), headphones on a hook, and cables.
#
#   blender -b --factory-startup --python tools/assets/sim_rig.py -- <texture dir> <out.glb>
#
# Model space: the driver faces -z, the rig is centered on x, floor y = 0.
import math
import os
import sys

import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402

tex_dir, out_path = sys.argv[sys.argv.index("--") + 1:][:2]
common.reset_scene()

alu = common.plain_material("Profile aluminum", (0.70, 0.71, 0.73), 1.0, 0.38)
slot = common.plain_material("Profile slot", (0.05, 0.05, 0.05), 0.5, 0.6)
black = common.plain_material("Rig black", (0.02, 0.02, 0.02), 0.2, 0.45)
rubber = common.plain_material("Wheel grip", (0.03, 0.03, 0.03), 0.0, 0.85)
carbon = common.plain_material("Carbon", (0.05, 0.05, 0.06), 0.6, 0.25)
red = common.plain_material("Accent red", (0.7, 0.03, 0.03), 0.0, 0.4)
seat_fab = common.pbr_material("Seat fabric", tex_dir, "fabric_leather_02", tint=(0.1, 0.1, 0.1), size=512)
steel = common.plain_material("Pedal steel", (0.75, 0.75, 0.78), 1.0, 0.3)
bezel = common.plain_material("Monitor bezel", (0.015, 0.015, 0.015), 0.1, 0.4)
btn_cols = [common.plain_material(f"Button {i}", c, 0.0, 0.35, emission=c, strength=1.5) for i, c in enumerate(
    [(0.9, 0.1, 0.1), (0.1, 0.4, 1.0), (0.1, 0.8, 0.2), (1.0, 0.75, 0.05)])]


def race_screen(w, h, part, seed=1):
    """A race seen from the cockpit: dusk sky, the track to the horizon with
    kerbs, barriers and grandstands, and for the middle screen a HUD."""
    rng = np.random.default_rng(seed)
    img = np.zeros((h, w, 3), np.float32)
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    hor = h * 0.55
    sky = (yy > hor)
    t = (yy - hor) / (h - hor)
    img[sky] = (np.stack([0.95 - 0.6 * t, 0.45 - 0.2 * t, 0.35 + 0.3 * t], -1))[sky]
    ground = ~sky
    img[ground] = np.array([0.08, 0.16, 0.06])
    # The track: a trapezoid narrowing to the horizon (shifted per screen).
    cx = w * (0.5 + {"left": 0.9, "mid": 0.0, "right": -0.9}[part])
    depth = np.clip((hor - yy) / hor, 0, 1)  # 1 at the bottom, 0 at the horizon
    half = 30 + depth * w * 0.9
    road = ground & (np.abs(xx - cx) < half)
    img[road] = np.array([0.12, 0.12, 0.13]) * (0.6 + 0.4 * depth[road, None])
    kerb = ground & (np.abs(np.abs(xx - cx) - half) < 6 + depth * 25)
    stripe = ((np.floor(1.0 / (depth + 0.05) * 3)) % 2) == 0
    img[kerb & stripe] = np.array([0.9, 0.1, 0.08])
    img[kerb & ~stripe] = np.array([0.95, 0.95, 0.95])
    center = ground & (np.abs(xx - cx) < 2 + depth * 6) & stripe
    img[center] = 0.9
    # Grandstands and barriers near the horizon.
    for k in range(6):
        x0 = rng.uniform(0, w)
        bw, bh = rng.uniform(40, 120), rng.uniform(15, 40)
        stand = (xx > x0) & (xx < x0 + bw) & (yy > hor) & (yy < hor + bh)
        img[stand] = np.array([0.2, 0.2, 0.25]) + 0.5 * rng.random((1, 3)) * ((xx[stand, None] + yy[stand, None]) % 6 < 2)
    # Cars ahead.
    if part == "mid":
        for (dx, dy, col) in ((-30, hor - 40, (0.1, 0.3, 0.9)), (40, hor - 22, (0.95, 0.75, 0.05))):
            car = (np.abs(xx - (cx + dx)) < 22 - (hor - dy) * 0.0) & (np.abs(yy - dy) < 8)
            img[car] = col
        # HUD: speed, gear, lap, position, a rev bar, the dashboard.
        img[:int(h * 0.18)] = np.array([0.02, 0.02, 0.025])
        hud = common.srgb((1, 1, 1))
        common.draw_text(img, "184", int(w * 0.5), int(h * 0.15), 5, hud, center=True)
        common.draw_text(img, "KM/H", int(w * 0.5), int(h * 0.06), 2, hud, center=True)
        common.draw_text(img, "4", int(w * 0.82), int(h * 0.15), 6, common.srgb((1.0, 0.8, 0.1)), center=True)
        common.draw_text(img, "LAP 2/5", 12, h - 10, 2, hud)
        common.draw_text(img, "P1", w - 40, h - 10, 3, common.srgb((1.0, 0.85, 0.2)))
        common.draw_text(img, "1:24.618", 12, h - 34, 2, hud)
        for k in range(15):
            col = (0.1, 0.9, 0.1) if k < 8 else (1.0, 0.8, 0.0) if k < 12 else (0.95, 0.1, 0.1)
            if k < 11:
                img[int(h * 0.2):int(h * 0.23), int(w * 0.3) + k * 16:int(w * 0.3) + k * 16 + 12] = col
    return np.clip(img, 0, 1)


def box(name, lo, hi, mat, bevel=0.004, seg=2):
    return common.box(name, lo, hi, mat, bevel, seg)


def profile(name, a, b, size=0.08):
    """An 80 mm aluminum extrusion between two points along one axis, with
    dark slots on its faces."""
    a, b = Vector(a), Vector(b)
    lo = Vector((min(a.x, b.x), min(a.y, b.y), min(a.z, b.z))) - Vector((size / 2,) * 3)
    hi = Vector((max(a.x, b.x), max(a.y, b.y), max(a.z, b.z))) + Vector((size / 2,) * 3)
    box(name, lo, hi, alu, 0.006)
    axis = max(range(3), key=lambda i: hi[i] - lo[i])
    for i in range(3):
        if i == axis:
            continue
        for sgn in (-1, 1):
            c = (lo + hi) / 2
            slo, shi = lo.copy(), hi.copy()
            slo[axis] += 0.004
            shi[axis] -= 0.004
            other = [j for j in range(3) if j not in (axis, i)][0]
            slo[other], shi[other] = c[other] - 0.006, c[other] + 0.006
            if sgn < 0:
                slo[i], shi[i] = lo[i] - 0.001, lo[i] + 0.004
            else:
                slo[i], shi[i] = hi[i] - 0.004, hi[i] + 0.001
            box(name + " slot", slo, shi, slot, 0.0)


# ---- Frame ----
for x in (-0.3, 0.3):
    profile("Side rail", (x, 0.04, 0.55), (x, 0.04, -0.95))
for z in (0.5, -0.1, -0.9):
    profile("Cross rail", (-0.3, 0.04, z), (0.3, 0.04, z))
for x in (-0.3, 0.3):
    profile("Wheel upright", (x, 0.04, -0.5), (x, 0.66, -0.5))
profile("Wheel deck rail", (-0.3, 0.7, -0.5), (0.3, 0.7, -0.5))
box("Wheel deck", (-0.18, 0.74, -0.62), (0.18, 0.755, -0.4), black, 0.004)
box("Pedal plate", (-0.28, 0.0, -0.03), (0.28, 0.012, 0.3), black, 0.004).data.transform(
    Matrix.Translation((0, 0.08, -0.92)) @ Matrix.Rotation(0.35, 4, "X"))
for x in (-0.3, 0.3):
    for z in (0.5, -0.9):
        box("Foot", (x - 0.05, 0.0, z - 0.05), (x + 0.05, 0.01, z + 0.05), black, 0.004)

# ---- Seat ----
seat_m = Matrix.Translation((0, 0.12, 0.25))
base = box("Seat base", (-0.24, 0.0, -0.28), (0.24, 0.12, 0.22), seat_fab, 0.04, 3)
base.data.transform(seat_m)
for side in (-1, 1):
    bol = box("Seat bolster", (-0.05, 0.0, -0.3), (0.05, 0.2, 0.2), seat_fab, 0.04, 3)
    bol.data.transform(seat_m @ Matrix.Translation((side * 0.24, 0.02, 0)))
back_m = seat_m @ Matrix.Translation((0, 0.08, 0.22)) @ Matrix.Rotation(0.3, 4, "X")
back = box("Seat back", (-0.25, 0.0, 0.0), (0.25, 0.78, 0.1), seat_fab, 0.05, 3)
back.data.transform(back_m)
for side in (-1, 1):
    wing = box("Back bolster", (-0.05, 0.0, -0.1), (0.05, 0.55, 0.08), seat_fab, 0.04, 3)
    wing.data.transform(back_m @ Matrix.Translation((side * 0.25, 0.05, 0)) @ Matrix.Rotation(side * 0.25, 4, "Y"))
for side in (-0.11, 0.11):  # harness slots
    s = box("Harness slot", (side - 0.035, 0.6, -0.003), (side + 0.035, 0.64, 0.0), black, 0.0)
    s.data.transform(back_m)
stripe = box("Seat stripe", (-0.03, 0.05, -0.004), (0.03, 0.75, 0.0), red, 0.0)
stripe.data.transform(back_m)
box("Seat bracket", (-0.22, 0.06, 0.0), (0.22, 0.12, 0.5), black, 0.004)

# ---- Wheel base and wheel ----
box("Wheel base", (-0.07, 0.755, -0.62), (0.07, 0.9, -0.38), black, 0.015, 3)
WC = Vector((0, 0.86, -0.3))   # wheel center
tilt = Matrix.Translation(WC) @ Matrix.Rotation(-0.25, 4, "X")
hub = common.cylinder("Quick release", Vector((0, 0, -0.06)), 0.035, 0.08, "z", steel, segments=16)
hub.data.transform(tilt)
rim = common.torus("Wheel rim", Vector((0, 0, 0)), 0.15, 0.016, rubber, axis="z", segments=40, sides=10)
rim.data.transform(tilt)
for a in (math.pi / 2, math.pi * 7 / 6, math.pi * 11 / 6):
    sp = box("Spoke", (0.0, -0.018, -0.008), (0.14, 0.018, 0.008), carbon, 0.004)
    sp.data.transform(tilt @ Matrix.Rotation(a, 4, "Z"))
plate = common.cylinder("Hub plate", Vector((0, 0, 0.0)), 0.07, 0.025, "z", carbon, segments=24)
plate.data.transform(tilt)
for k in range(8):
    a = math.pi * (0.15 + 0.7 * (k % 4) / 3) + (math.pi if k >= 4 else 0)
    bb = common.cylinder("Wheel button", Vector((0.055 * math.cos(a), 0.055 * math.sin(a), 0.016)), 0.009, 0.01, "z",
                         btn_cols[k % 4], segments=10)
    bb.data.transform(tilt)
disp_px = np.zeros((24, 64, 3), np.float32)
common.draw_text(disp_px, "4 184", 32, 18, 2, common.srgb((0.2, 1.0, 0.3)), center=True)
disp = common.glow_image_material("Wheel display", disp_px, strength=1.2)
dq = common.quad("Wheel display", [(-0.04, 0.02, 0.0135), (0.04, 0.02, 0.0135), (0.04, 0.05, 0.0135),
                                   (-0.04, 0.05, 0.0135)], disp)
dq.data.transform(tilt)
for side in (-1, 1):
    pd = box("Shift paddle", (-0.015, -0.06, -0.004), (0.015, 0.06, 0.004), carbon, 0.003)
    pd.data.transform(tilt @ Matrix.Translation((side * 0.09, 0.04, -0.03)) @ Matrix.Rotation(side * 0.4, 4, "Z"))

# ---- Pedals ----
pedal_m = Matrix.Translation((0, 0.08, -0.92)) @ Matrix.Rotation(0.35, 4, "X")
for k, x in enumerate((-0.14, 0.0, 0.14)):
    arm = box("Pedal arm", (x - 0.01, 0.012, 0.05), (x + 0.01, 0.2, 0.07), steel, 0.004)
    arm.data.transform(pedal_m)
    face = box("Pedal face", (x - 0.04, 0.12, 0.07), (x + 0.04, 0.24, 0.085), steel, 0.006)
    face.data.transform(pedal_m @ Matrix.Translation((0, 0, 0.02)) @ Matrix.Rotation(-0.25, 4, "X"))
    spring = common.cylinder("Pedal spring", Vector((x, 0.07, 0.12)), 0.018, 0.12, "z", red if k == 1 else black,
                             segments=10)
    spring.data.transform(pedal_m)

# ---- Shifter and handbrake on the right ----
profile("Shifter mount", (0.42, 0.04, 0.0), (0.42, 0.55, 0.0), 0.06)
box("Shifter arm", (0.3, 0.52, -0.04), (0.45, 0.58, 0.04), black, 0.006)
box("Shifter box", (0.3, 0.58, -0.08), (0.4, 0.64, 0.08), black, 0.008)
common.cylinder("Shift lever", Vector((0.35, 0.72, 0.0)), 0.007, 0.18, "y", steel, segments=8)
common.sphere("Shift knob", Vector((0.35, 0.82, 0.0)), 0.028, black, 12, 8)
lever = box("Handbrake", (-0.012, 0.0, -0.012), (0.012, 0.3, 0.012), steel, 0.004)
lever.data.transform(Matrix.Translation((0.42, 0.58, 0.12)) @ Matrix.Rotation(-0.5, 4, "X"))
common.box("Handbrake grip", (0.405, 0.75, 0.0), (0.435, 0.85, 0.03), rubber, 0.01)

# ---- Triple monitors on a stand ----
MW, MH, MY, MZ = 0.72, 0.42, 1.08, -1.08
stand_c = Vector((0, 0, MZ - 0.12))
box("Stand foot", (-0.9, 0.0, MZ - 0.3), (0.9, 0.04, MZ + 0.06), black, 0.01)
box("Stand pole", (-0.03, 0.0, MZ - 0.15), (0.03, MY + 0.1, MZ - 0.09), black, 0.006)
box("Stand bar", (-0.95, MY - 0.03, MZ - 0.15), (0.95, MY + 0.03, MZ - 0.09), black, 0.006)
screens = {"left": race_screen(256, 150, "left", 2), "mid": race_screen(256, 150, "mid", 3),
           "right": race_screen(256, 150, "right", 4)}
for part, ang, off in (("mid", 0.0, 0.0), ("left", 0.85, -1), ("right", -0.85, 1)):
    if off == 0:
        m = Matrix.Translation((0, MY, MZ))
    else:
        hinge = Vector((off * (MW / 2 + 0.01), MY, MZ))
        m = Matrix.Translation(hinge) @ Matrix.Rotation(ang, 4, "Y") @ Matrix.Translation((off * (MW / 2 + 0.01), 0, 0))
    body = box("Monitor", (-MW / 2 - 0.012, -MH / 2 - 0.012, -0.05), (MW / 2 + 0.012, MH / 2 + 0.012, 0.0), bezel, 0.006)
    body.data.transform(m)
    mat = common.glow_image_material(f"Screen {part}", screens[part], strength=1.3, base_scale=0.2)
    q = common.quad("Screen", [(-MW / 2, -MH / 2, 0.001), (MW / 2, -MH / 2, 0.001), (MW / 2, MH / 2, 0.001),
                               (-MW / 2, MH / 2, 0.001)], mat)
    q.data.transform(m)

# ---- Headphones on a hook, cables ----
common.tube("Hook", [(-0.36, 0.7, -0.5), (-0.44, 0.7, -0.5), (-0.46, 0.75, -0.5)], 0.006, steel, sides=6)
band = common.torus("Headphone band", Vector((0, 0, 0)), 0.09, 0.012, black, axis="x", segments=24, sides=6)
band.data.transform(Matrix.Translation((-0.45, 0.62, -0.5)))
for side in (-1, 1):
    common.cylinder("Ear cup", Vector((-0.45, 0.55, -0.5 + side * 0.08)), 0.045, 0.04, "z", black, segments=16)
common.tube("Cable", [(0.05, 0.8, -0.62), (0.08, 0.6, -0.66), (0.1, 0.2, -0.7), (0.2, 0.01, -0.8),
                      (0.5, 0.01, -1.1)], 0.006, black, sides=5)
common.tube("Cable", [(-0.25, 0.04, -0.95), (-0.5, 0.01, -1.05), (-0.7, 0.01, -1.12)], 0.005, black, sides=5)

common.finish_and_export(out_path, uv_scale={"Seat fabric": 4.0})
