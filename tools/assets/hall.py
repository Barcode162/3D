# Builds the new bar's shell (docs/BAR_REDESIGN.md): parquet floor, walls
# (red brick at the bar, darker brick in the game corner, deep green plaster
# over oak wainscot in the lounge), the diagonal corner wall, a wood-slat
# soffit over the bar, an open ceiling with steel beams, ducts and
# sprinkler pipes, the bay window with its seat, the street door, the two
# arched doorways with the unfinished rooms behind them, the stage, the
# chimney breast and the rubber mats behind the bar.
#
#   blender -b --factory-startup --python tools/assets/hall.py -- <texture dir> <out.glb>
import math
import os
import sys

import numpy as np
from mathutils import Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402
from layout import *  # noqa: E402,F401,F403

tex_dir, out_path = sys.argv[sys.argv.index("--") + 1:][:2]
common.reset_scene()

# ---- Materials ----
parquet = common.pbr_material("Parquet", tex_dir, "herringbone_parquet", tint=(0.62, 0.5, 0.42),
                              roughness_scale=0.75)
brick = common.pbr_material("Brick", tex_dir, "red_brick_03", tint=(0.85, 0.76, 0.72), normal_strength=1.4)
brick2 = common.pbr_material("Dark brick", tex_dir, "brick_wall_006", tint=(0.62, 0.55, 0.55),
                             normal_strength=1.3, size=512)
arch_brick = common.plain_material("Arch brick", (0.32, 0.11, 0.07), 0.0, 0.85)
green = common.pbr_material("Green plaster", tex_dir, "painted_plaster_wall", tint=(0.16, 0.30, 0.22),
                            roughness_scale=0.9, saturation=0.4, size=512)
ceiling = common.pbr_material("Ceiling", tex_dir, "plastered_wall_02", tint=(0.13, 0.12, 0.12), size=512)
oak = common.pbr_material("Wainscot oak", tex_dir, "dark_wood", tint=(0.55, 0.42, 0.36), roughness_scale=0.6,
                          size=512)
slats = common.pbr_material("Soffit wood", tex_dir, "wood_table_001", tint=(0.62, 0.45, 0.33),
                            roughness_scale=0.7, size=512)
stage_wood = common.pbr_material("Stage boards", tex_dir, "black_painted_planks", tint=(0.55, 0.5, 0.48),
                                 roughness_scale=0.9, size=512)
door_wood = common.pbr_material("Door wood", tex_dir, "wood_table_001", tint=(0.42, 0.3, 0.24),
                                roughness_scale=0.6, grain="v", size=512)
concrete = common.pbr_material("Concrete", tex_dir, "concrete_floor_worn_001", tint=(0.6, 0.6, 0.6), size=512)
studs = common.pbr_material("Pine studs", tex_dir, "plywood", tint=(0.9, 0.78, 0.62), size=512)
rubber = common.pbr_material("Rubber mat", tex_dir, "rubber_tiles", tint=(0.25, 0.25, 0.25), size=512)
drywall = common.plain_material("Drywall", (0.62, 0.6, 0.56), 0.0, 0.85)
steel = common.plain_material("Black steel", (0.03, 0.03, 0.032), 0.6, 0.45)
duct = common.plain_material("Galvanized duct", (0.55, 0.56, 0.57), 1.0, 0.42)
red_pipe = common.plain_material("Sprinkler pipe", (0.45, 0.04, 0.03), 0.3, 0.4)
brass = common.plain_material("Brass", (0.91, 0.70, 0.38), 1.0, 0.28)
chrome = common.plain_material("Chrome", (0.9, 0.9, 0.92), 1.0, 0.12)
cushion = common.pbr_material("Seat velvet", tex_dir, "velour_velvet", tint=(0.55, 0.12, 0.14), size=512)
pillow = common.pbr_material("Pillow fabric", tex_dir, "fabric_pattern_07", tint=(0.9, 0.8, 0.6), size=512)


def night_street(w, h, seed):
    """A night street seen through a window: dusk sky, buildings with lit
    windows, street lamps, a few car lights, wet road reflections."""
    rng = np.random.default_rng(seed)
    img = np.zeros((h, w, 3), np.float32)
    y = np.linspace(0, 1, h)[:, None]
    sky = np.array(common.srgb((0.10, 0.08, 0.22)))[None, None, :] * (0.35 + 0.65 * y[..., None])
    img[:] = sky + np.array(common.srgb((0.30, 0.12, 0.22)))[None, None, :] * np.exp(-((y - 0.45) / 0.15) ** 2)[..., None]
    street = int(h * 0.22)
    x = 0
    while x < w:  # buildings
        bw = int(rng.integers(w // 10, w // 4))
        top = int(h * rng.uniform(0.55, 0.95))
        col = np.array(common.srgb((0.035, 0.03, 0.045))) * rng.uniform(0.7, 1.3)
        img[street:top, x:x + bw] = col
        for wy in range(street + 12, top - 10, 16):  # lit windows
            for wx in range(x + 6, x + bw - 8, 13):
                if rng.random() < 0.45:
                    warm = common.srgb((1.0, rng.uniform(0.6, 0.8), rng.uniform(0.3, 0.5)))
                    img[wy:wy + 8, wx:wx + 6] = np.array(warm) * rng.uniform(0.25, 0.9)
        x += bw + int(rng.integers(0, 12))
    img[:street] = np.array(common.srgb((0.02, 0.02, 0.025)))
    yy, xx = np.mgrid[0:h, 0:w]
    for lx in rng.integers(0, w, 4):  # street lamps: pole + glow
        ly = int(h * 0.48)
        img[street:ly, lx:lx + 3] = 0.02
        g = np.exp(-(((xx - lx) ** 2 + (yy - ly) ** 2) / (2 * 22.0 ** 2)))[..., None]
        img += g * np.array(common.srgb((1.0, 0.75, 0.45))) * 1.4
        r = np.exp(-(((xx - lx) / 10.0) ** 2) - ((yy - street * 0.5) / (street * 0.6)) ** 2)[..., None]
        img += r * np.array(common.srgb((1.0, 0.7, 0.4))) * 0.5  # reflection on the wet road
    for _ in range(3):  # car lights
        cx, cy = int(rng.integers(0, w)), int(street * 0.75)
        color = (1.0, 0.15, 0.1) if rng.random() < 0.5 else (1.0, 0.95, 0.8)
        g = np.exp(-(((xx - cx) ** 2) / 40.0 + ((yy - cy) ** 2) / 10.0))[..., None]
        img += g * np.array(common.srgb(color)) * 1.5
    return np.clip(img, 0, 1)


window_glow = common.glow_image_material("Night street", night_street(1024, 512, 7), strength=1.0, base_scale=0.4)
door_glow = common.glow_image_material("Street through door", night_street(512, 512, 19), strength=0.8,
                                       base_scale=0.4)
exit_px = np.zeros((32, 96, 3), np.float32)
exit_px[:] = common.srgb((0.0, 0.25, 0.08))
common.draw_text(exit_px, "EXIT", 48, 28, 4, common.srgb((0.75, 1.0, 0.8)), center=True)
exit_sign = common.glow_image_material("Exit sign", exit_px, strength=2.5, base_scale=0.5)


def box(name, lo, hi, mat, bevel=0.004, segments=2, grain_axis="x"):
    obj = common.box(name, lo, hi, mat, bevel, segments)
    obj["grain_axis"] = grain_axis
    return obj


def xy_slab(name, outline, z0, z1, mat, grain_axis="x"):
    """An outline in the wall plane [(x, y), ...] pushed through z0..z1."""
    obj = common.prism(name, [(x, y) for (x, y) in outline], z0, z1, mat)
    # prism builds (x, height, z); swap so the outline's second value is y.
    for v in obj.data.vertices:
        v.co = Vector((v.co.x, v.co.z, v.co.y))
    obj.data.flip_normals()
    obj["grain_axis"] = grain_axis
    return obj


def zy_slab(name, outline, x0, x1, mat):
    """An outline in a side wall's plane [(z, y), ...] pushed through x0..x1."""
    obj = common.prism(name, outline, x0, x1, mat)
    for v in obj.data.vertices:
        v.co = Vector((v.co.y, v.co.z, v.co.x))
    obj["grain_axis"] = "z"
    return obj


def arch_spandrels(u0, u1, plane_fn, mat, name):
    """The wall pieces beside a round arch top (opening u0..u1, wall plane
    coordinates u along the wall, y up), up to the top of the arch."""
    r = (u1 - u0) / 2
    uc = u0 + r
    top = ARCH_SPRING + r
    for side in (-1, 1):
        pts = [(uc + side * r, ARCH_SPRING)]
        for k in range(1, 12):
            a = math.pi / 2 * k / 12
            pts.append((uc + side * r * math.cos(a), ARCH_SPRING + r * math.sin(a)))
        pts += [(uc, top), (uc + side * r, top)]
        plane_fn(name, pts, mat)
    return top


def voussoirs(center, r, along, normal, mat):
    """Header bricks around an arch (center = middle at spring height)."""
    from mathutils import Matrix
    c, a, n = Vector(center), Vector(along), Vector(normal)
    up = Vector((0, 1, 0))
    count = 21
    for k in range(count):
        ang = math.pi * (k + 0.5) / count
        d = a * math.cos(ang) + up * math.sin(ang)  # outward, in the wall plane
        t = n.cross(d)
        # Radial 0.22 m, along the arch ~0.11 m, 0.02 m proud of the wall.
        mid = c + d * (r + 0.11) + n * (-0.14)
        half_t = math.pi * (r + 0.11) / count / 2 - 0.005
        obj = common.box("Voussoir", (-0.11, -half_t, -0.16), (0.11, half_t, 0.16), mat, 0.008)
        rot = Matrix((d, t, n)).transposed().to_4x4()  # local x = d, y = t, z = n
        obj.data.transform(Matrix.Translation(mid) @ rot)
        obj["grain_axis"] = "x"


# ---- Floor and ceiling ----
outline = [(X0, Z0), CHAMFER_A, CHAMFER_B, (X1, Z1), (BAY_X1, Z1), (BAY_X1, Z1 + BAY_DEPTH),
           (BAY_X0, Z1 + BAY_DEPTH), (BAY_X0, Z1), (X0, Z1)]
common.prism("Floor", outline, -0.1, 0.0, parquet)["grain_axis"] = "z"
hall_top = [(X0, SOFFIT_Z1), (SOFFIT_X1, SOFFIT_Z1), (SOFFIT_X1, Z0), CHAMFER_A, CHAMFER_B, (X1, Z1), (X0, Z1)]
common.prism("Ceiling", hall_top, H, H + 0.1, ceiling)
common.prism("Bay ceiling", [(BAY_X0, Z1), (BAY_X1, Z1), (BAY_X1, Z1 + BAY_DEPTH), (BAY_X0, Z1 + BAY_DEPTH)],
             BAY_H, BAY_H + 0.1, ceiling)

# ---- Walls ----
# Back wall (red brick), with doorway A.
box("Back wall", (X0 - T, 0, Z0 - T), (DOOR_A_X0, H, Z0), brick, 0)
box("Back wall", (DOOR_A_X1, 0, Z0 - T), (CHAMFER_A[0], H, Z0), brick, 0)
top_a = arch_spandrels(DOOR_A_X0, DOOR_A_X1, lambda n, p, m: xy_slab(n, p, Z0 - T, Z0, m), brick, "Arch wall")
box("Back wall", (DOOR_A_X0, top_a, Z0 - T), (DOOR_A_X1, H, Z0), brick, 0)
voussoirs(((DOOR_A_X0 + DOOR_A_X1) / 2, ARCH_SPRING, Z0), (DOOR_A_X1 - DOOR_A_X0) / 2, (1, 0, 0), (0, 0, 1),
          arch_brick)

# Diagonal wall (dark brick) across the back-right corner.
dx, dz = CHAMFER_B[0] - CHAMFER_A[0], CHAMFER_B[1] - CHAMFER_A[1]
diag_len = math.hypot(dx, dz)
diag_yaw = -math.atan2(dz, dx)
mid = ((CHAMFER_A[0] + CHAMFER_B[0]) / 2, (CHAMFER_A[1] + CHAMFER_B[1]) / 2)
nx, nz = -dz / diag_len, dx / diag_len  # into the room
dw = common.obox("Diagonal wall", (mid[0] - nx * T / 2, H / 2, mid[1] - nz * T / 2), (diag_len + 0.6, H, T),
                 diag_yaw, brick2, 0)
dw["grain_axis"] = "x"

# Right wall (dark brick).
box("Right wall", (X1, 0, CHAMFER_B[1] - 0.3), (X1 + T, H, Z1 + T), brick2, 0, grain_axis="z")

# Front wall: plaster, with the bay window opening and the street door.
for (a, b) in ((X0 - T, BAY_X0), (BAY_X1, ENTRY_X0), (ENTRY_X1, X1 + T)):
    box("Front wall", (a, 0, Z1), (b, H, Z1 + T), green, 0)
box("Over the bay", (BAY_X0, BAY_H, Z1), (BAY_X1, H, Z1 + T), green, 0)
box("Over the door", (ENTRY_X0, ENTRY_H + 0.55, Z1), (ENTRY_X1, H, Z1 + T), green, 0)
# Bay window walls.
box("Bay side", (BAY_X0 - 0.15, 0, Z1), (BAY_X0, BAY_H, Z1 + BAY_DEPTH), green, 0, grain_axis="z")
box("Bay side", (BAY_X1, 0, Z1), (BAY_X1 + 0.15, BAY_H, Z1 + BAY_DEPTH), green, 0, grain_axis="z")
box("Bay front", (BAY_X0 - 0.15, 0, Z1 + BAY_DEPTH), (BAY_X1 + 0.15, 0.55, Z1 + BAY_DEPTH + 0.15), green, 0)
box("Bay front", (BAY_X0 - 0.15, 2.6, Z1 + BAY_DEPTH), (BAY_X1 + 0.15, BAY_H, Z1 + BAY_DEPTH + 0.15), green, 0)

# Left wall: plaster, with doorway B and the chimney breast.
box("Left wall", (X0 - T, 0, Z0), (X0, H, DOOR_B_Z0), green, 0, grain_axis="z")
box("Left wall", (X0 - T, 0, DOOR_B_Z1), (X0, H, Z1), green, 0, grain_axis="z")
top_b = arch_spandrels(DOOR_B_Z0, DOOR_B_Z1, lambda n, p, m: zy_slab(n, p, X0 - T, X0, m), green, "Arch wall")
box("Left wall", (X0 - T, top_b, DOOR_B_Z0), (X0, H, DOOR_B_Z1), green, 0, grain_axis="z")
voussoirs((X0, ARCH_SPRING, (DOOR_B_Z0 + DOOR_B_Z1) / 2), (DOOR_B_Z1 - DOOR_B_Z0) / 2, (0, 0, 1), (1, 0, 0),
          arch_brick)
# Chimney breast, with the firebox opening (lounge.py builds the fireplace).
FB_Z0, FB_Z1, FB_H = (CHIMNEY_Z0 + CHIMNEY_Z1) / 2 - 0.45, (CHIMNEY_Z0 + CHIMNEY_Z1) / 2 + 0.45, 0.78
box("Chimney breast", (X0, 0, CHIMNEY_Z0), (X0 + CHIMNEY_D, H, FB_Z0), brick, 0.01, grain_axis="z")
box("Chimney breast", (X0, 0, FB_Z1), (X0 + CHIMNEY_D, H, CHIMNEY_Z1), brick, 0.01, grain_axis="z")
box("Chimney breast", (X0, FB_H, FB_Z0), (X0 + CHIMNEY_D, H, FB_Z1), brick, 0.0, grain_axis="z")

# ---- Wainscot on the plaster walls ----
PANEL_H = 1.1


def wainscot(wall, a, b):
    """Raised oak panels with a chair rail; wall = "left" (x = X0), "front"
    (z = Z1) or "bayL"/"bayR" (the bay's side walls); a..b along the wall."""
    def piece(name, u0, u1, y0, y1, d0, d1, bevel, segments=2, vertical=False):
        if wall == "left":
            lo, hi, ax = (X0 + d0, y0, u0), (X0 + d1, y1, u1), "z"
        elif wall == "bayL":
            lo, hi, ax = (BAY_X0 + d0, y0, u0), (BAY_X0 + d1, y1, u1), "z"
        elif wall == "bayR":
            lo, hi, ax = (BAY_X1 - d1, y0, u0), (BAY_X1 - d0, y1, u1), "z"
        else:
            lo, hi, ax = (u0, y0, Z1 - d1), (u1, y1, Z1 - d0), "x"
        return box(name, lo, hi, oak, bevel, segments, "y" if vertical else ax)

    piece("Wainscot back", a, b, 0.0, PANEL_H, 0.0, 0.015, 0.0)
    piece("Baseboard", a, b, 0.0, 0.18, 0.0, 0.03, 0.006)
    piece("Base cap", a, b, 0.175, 0.195, 0.0, 0.036, 0.008, 3)
    piece("Lower rail", a, b, 0.195, 0.28, 0.0, 0.025, 0.004)
    piece("Upper rail", a, b, PANEL_H - 0.1, PANEL_H - 0.04, 0.0, 0.025, 0.004)
    piece("Chair rail", a, b, PANEL_H - 0.04, PANEL_H, 0.0, 0.05, 0.012, 3)
    n = max(1, round((b - a) / 0.75))
    stile = 0.07
    w = (b - a - stile * (n + 1)) / n
    for i in range(n + 1):
        u = a + i * (w + stile)
        piece("Stile", u, u + stile, 0.28, PANEL_H - 0.1, 0.0, 0.025, 0.004, vertical=True)
    for i in range(n):
        u = a + stile + i * (w + stile)
        piece("Field", u + 0.035, u + w - 0.035, 0.32, PANEL_H - 0.14, 0.0, 0.03, 0.014, 3, vertical=True)


wainscot("left", Z0, DOOR_B_Z0)
wainscot("left", DOOR_B_Z1, CHIMNEY_Z0)
wainscot("left", CHIMNEY_Z1, STAGE_Z0 - STEP_W)
wainscot("front", BAY_X1, ENTRY_X0)
wainscot("front", STAGE_X1 + STEP_W, BAY_X0)
wainscot("front", ENTRY_X1, X1)
wainscot("bayL", Z1, Z1 + BAY_DEPTH)
wainscot("bayR", Z1, Z1 + BAY_DEPTH)


def baseboard_x(z, a, b, facing):
    """Baseboard along a wall at z (facing = +1 into +z)."""
    z2 = z + facing * 0.03
    box("Baseboard", (a, 0, min(z, z2)), (b, 0.16, max(z, z2)), oak, 0.006)


def baseboard_z(x, a, b, facing):
    x2 = x + facing * 0.03
    box("Baseboard", (min(x, x2), 0, a), (max(x, x2), 0.16, b), oak, 0.006, grain_axis="z")


baseboard_x(Z0, X0, DOOR_A_X0, 1)
baseboard_x(Z0, DOOR_A_X1, CHAMFER_A[0], 1)
baseboard_z(X1, CHAMFER_B[1], Z1, -1)
bb = common.obox("Baseboard", (mid[0] + nx * 0.015, 0.08, mid[1] + nz * 0.015), (diag_len, 0.16, 0.03), diag_yaw,
                 oak, 0.006)
baseboard_z(X0 + CHIMNEY_D, CHIMNEY_Z0, CHIMNEY_Z1, 1)

# Crown molding where the plaster walls meet the hall ceiling.
for (lo, hi, ax) in [((X0, H - 0.12, SOFFIT_Z1), (X0 + 0.07, H, Z1), "z"),
                     ((X0, H - 0.12, Z1 - 0.07), (X1, H, Z1), "x")]:
    box("Crown", lo, hi, oak, 0.015, 3, ax)

# ---- Bar soffit: a lowered wood-slat ceiling over the counter ----
box("Soffit body", (X0, SOFFIT_Y, Z0), (SOFFIT_X1, H, SOFFIT_Z1), ceiling, 0)
box("Soffit board", (X0, SOFFIT_Y - 0.02, Z0), (SOFFIT_X1, SOFFIT_Y, SOFFIT_Z1), oak, 0)
z = Z0 + 0.06
while z < SOFFIT_Z1 - 0.05:  # slats along the bar
    box("Slat", (X0, SOFFIT_Y - 0.065, z), (SOFFIT_X1 - 0.02, SOFFIT_Y - 0.02, z + 0.05), slats, 0.004)
    z += 0.11
box("Soffit fascia", (X0, SOFFIT_Y - 0.12, SOFFIT_Z1 - 0.02), (SOFFIT_X1 + 0.02, H - 0.25, SOFFIT_Z1 + 0.03), oak,
    0.01)
box("Soffit fascia", (SOFFIT_X1 - 0.02, SOFFIT_Y - 0.12, Z0), (SOFFIT_X1 + 0.03, H - 0.25, SOFFIT_Z1 + 0.03), oak,
    0.01, grain_axis="z")
box("Fascia crown", (X0, H - 0.25, SOFFIT_Z1 - 0.02), (SOFFIT_X1 + 0.06, H - 0.18, SOFFIT_Z1 + 0.07), oak, 0.02, 3)
box("Fascia crown", (SOFFIT_X1 - 0.02, H - 0.25, Z0), (SOFFIT_X1 + 0.07, H - 0.18, SOFFIT_Z1 + 0.07), oak, 0.02, 3,
    grain_axis="z")

# ---- Open ceiling: steel beams, ducts, sprinkler pipes ----


def i_beam_z(x, z0, z1, depth=0.36, flange=0.2):
    box("Beam flange", (x - flange / 2, H - 0.025, z0), (x + flange / 2, H, z1), steel, 0.003, grain_axis="z")
    box("Beam flange", (x - flange / 2, H - depth, z0), (x + flange / 2, H - depth + 0.025, z1), steel, 0.003,
        grain_axis="z")
    box("Beam web", (x - 0.012, H - depth, z0), (x + 0.012, H, z1), steel, 0.002, grain_axis="z")
    for zz in np.arange(z0 + 0.6, z1, 1.2):  # stiffener plates
        box("Stiffener", (x - flange / 2 + 0.01, H - depth + 0.025, zz), (x + flange / 2 - 0.01, H - 0.025, zz + 0.012),
            steel, 0.001)


for x in (-6.0, -2.0):
    i_beam_z(x, SOFFIT_Z1, Z1)
i_beam_z(2.0, SOFFIT_Z1, Z1)
i_beam_z(6.0, Z0 + 0.1, Z1)
# A girder across the hall.
box("Girder", (X0, H - 0.42, 1.9), (X1, H - 0.39, 2.1), steel, 0.003)
box("Girder", (X0, H - 0.03, 1.9), (X1, H, 2.1), steel, 0.003)
box("Girder web", (X0, H - 0.42, 1.985), (X1, H, 2.015), steel, 0.002)

# Main duct along the hall, with joints, hangers and three diffusers.
DUCT_Y, DUCT_Z, DUCT_R = H - 0.62, 0.4, 0.2
common.cylinder("Duct", Vector((0.0, DUCT_Y, DUCT_Z)), DUCT_R, X1 - X0 - 0.4, "x", duct, segments=28)
for x in np.arange(X0 + 0.8, X1 - 0.4, 1.4):
    common.torus("Duct joint", Vector((x, DUCT_Y, DUCT_Z)), DUCT_R + 0.004, 0.012, duct, axis="x", segments=28, sides=6)
for x in np.arange(X0 + 1.5, X1 - 0.5, 2.8):
    for zz in (DUCT_Z - 0.17, DUCT_Z + 0.17):
        box("Hanger", (x - 0.015, DUCT_Y, zz - 0.004), (x + 0.015, H, zz + 0.004), steel, 0.001)
    common.torus("Hanger strap", Vector((x, DUCT_Y, DUCT_Z)), DUCT_R + 0.012, 0.008, steel, axis="x", segments=24,
                 sides=4)
for x in (-4.0, 0.0, 4.0):
    common.cylinder("Drop", Vector((x, DUCT_Y - DUCT_R - 0.1, DUCT_Z)), 0.08, 0.22, "y", duct, segments=16)
    box("Diffuser", (x - 0.22, DUCT_Y - DUCT_R - 0.3, DUCT_Z - 0.22), (x + 0.22, DUCT_Y - DUCT_R - 0.2, DUCT_Z + 0.22),
        duct, 0.01)
    for k in range(5):  # grille slots
        o = -0.16 + k * 0.08
        box("Diffuser slot", (x - 0.17, DUCT_Y - DUCT_R - 0.305, DUCT_Z + o - 0.012),
            (x + 0.17, DUCT_Y - DUCT_R - 0.29, DUCT_Z + o + 0.012), steel, 0.0)
# A smaller duct down the right side to the game corner.
common.cylinder("Duct", Vector((X1 - 0.5, H - 0.5, (CHAMFER_B[1] + Z1) / 2)), 0.14, Z1 - CHAMFER_B[1] - 0.4, "z",
                duct, segments=24)

# Sprinkler pipes with heads.
for zz in (-1.6, 4.6):
    common.cylinder("Sprinkler pipe", Vector((0.0, H - 0.12, zz)), 0.026, X1 - X0 - 0.3, "x", red_pipe, segments=12)
    for x in np.arange(X0 + 1.2, X1 - 0.8, 2.6):
        common.cylinder("Sprinkler drop", Vector((x, H - 0.17, zz)), 0.012, 0.1, "y", brass, segments=8)
        common.cylinder("Sprinkler head", Vector((x, H - 0.23, zz)), 0.03, 0.012, "y", brass, segments=12)
        box("Sprinkler pipe clamp", (x + 0.3, H - 0.15, zz - 0.01), (x + 0.32, H, zz + 0.01), steel, 0.0)

# ---- Bay window and seat ----
WZ = Z1 + BAY_DEPTH
common.panel("Bay glass", ((BAY_X0 + BAY_X1) / 2, 1.575, WZ - 0.01), (-1, 0, 0), (0, 1, 0), BAY_X1 - BAY_X0, 2.05,
             window_glow)
box("Sill", (BAY_X0, 0.5, WZ - 0.12), (BAY_X1, 0.56, WZ), oak, 0.01)
box("Window head", (BAY_X0, 2.56, WZ - 0.08), (BAY_X1, 2.64, WZ), door_wood, 0.006)
box("Transom bar", (BAY_X0, 2.08, WZ - 0.07), (BAY_X1, 2.14, WZ - 0.01), door_wood, 0.006)
for i in range(4):
    x = BAY_X0 + i * (BAY_X1 - BAY_X0) / 3
    box("Mullion", (x - 0.045, 0.56, WZ - 0.08), (x + 0.045, 2.56, WZ - 0.01), door_wood, 0.006, grain_axis="y")
for i in range(3):  # thin glazing bars in the transom lights
    x0 = BAY_X0 + i * (BAY_X1 - BAY_X0) / 3
    for f in (1 / 3, 2 / 3):
        x = x0 + f * (BAY_X1 - BAY_X0) / 3
        box("Glazing bar", (x - 0.012, 2.14, WZ - 0.04), (x + 0.012, 2.56, WZ - 0.01), door_wood, 0.003,
            grain_axis="y")
# Window seat: paneled base, velvet cushion, pillows.
box("Seat base", (BAY_X0, 0, Z1 + 0.42), (BAY_X1, 0.42, WZ - 0.12), oak, 0.01)
box("Seat kick", (BAY_X0 + 0.02, 0, Z1 + 0.40), (BAY_X1 - 0.02, 0.09, Z1 + 0.43), oak, 0.004)
for i in range(5):
    x0 = BAY_X0 + 0.08 + i * (BAY_X1 - BAY_X0 - 0.16) / 5
    box("Seat panel", (x0 + 0.04, 0.13, Z1 + 0.405), (x0 + (BAY_X1 - BAY_X0 - 0.16) / 5 - 0.04, 0.36, Z1 + 0.42),
        oak, 0.008, 3)
box("Seat cushion", (BAY_X0 + 0.02, 0.42, Z1 + 0.40), (BAY_X1 - 0.02, 0.53, WZ - 0.12), cushion, 0.04, 4)
for (x, tilt) in ((-1.7, 0.15), (-1.1, -0.1), (1.2, 0.12)):
    common.obox("Pillow", (x, 0.73, WZ - 0.27), (0.45, 0.42, 0.14), tilt, pillow, 0.06, 4)

# ---- Street door ----
DW = ENTRY_X1 - ENTRY_X0
box("Door casing", (ENTRY_X0 - 0.12, 0, Z1 - 0.04), (ENTRY_X0, ENTRY_H + 0.55, Z1), door_wood, 0.01, 3, "y")
box("Door casing", (ENTRY_X1, 0, Z1 - 0.04), (ENTRY_X1 + 0.12, ENTRY_H + 0.55, Z1), door_wood, 0.01, 3, "y")
box("Door head", (ENTRY_X0 - 0.15, ENTRY_H + 0.55, Z1 - 0.05), (ENTRY_X1 + 0.15, ENTRY_H + 0.7, Z1), door_wood,
    0.012, 3)
box("Transom frame", (ENTRY_X0, ENTRY_H, Z1 - 0.02), (ENTRY_X1, ENTRY_H + 0.06, Z1 + 0.05), door_wood, 0.006)
common.panel("Transom glass", ((ENTRY_X0 + ENTRY_X1) / 2, ENTRY_H + 0.3, Z1 + 0.03), (-1, 0, 0), (0, 1, 0), DW, 0.5,
             door_glow, uvs=((0, 0.75), (1, 0.75), (1, 1), (0, 1)))
for k, (x0, x1) in enumerate(((ENTRY_X0, ENTRY_X0 + DW / 2 - 0.005), (ENTRY_X0 + DW / 2 + 0.005, ENTRY_X1))):
    zf = Z1 + 0.01
    box("Door leaf", (x0, 0.01, zf), (x1, ENTRY_H, zf + 0.05), door_wood, 0.006, 2, "y")
    common.panel("Door glass", ((x0 + x1) / 2, 1.6, zf - 0.002), (-1, 0, 0), (0, 1, 0), x1 - x0 - 0.24, 1.2,
                 door_glow, uvs=((k * 0.5, 0.2), (k * 0.5 + 0.5, 0.2), (k * 0.5 + 0.5, 0.95), (k * 0.5, 0.95)))
    for (ya, yb) in ((0.98, 1.0), (2.2, 2.22)):
        box("Glass stop", (x0 + 0.1, ya, zf - 0.015), (x1 - 0.1, yb, zf), door_wood, 0.003)
    for xx in (x0 + 0.1, x1 - 0.12):
        box("Glass stop", (xx, 1.0, zf - 0.015), (xx + 0.02, 2.2, zf), door_wood, 0.003, grain_axis="y")
    box("Door panel", (x0 + 0.1, 0.2, zf - 0.012), (x1 - 0.1, 0.85, zf), door_wood, 0.012, 3, "y")
    box("Kick plate", (x0 + 0.03, 0.02, zf - 0.004), (x1 - 0.03, 0.18, zf), brass, 0.002)
    common.cylinder("Push bar", Vector(((x0 + x1) / 2, 1.05, zf - 0.07)), 0.018, x1 - x0 - 0.16, "x", chrome,
                    segments=14)
    for xx in (x0 + 0.1, x1 - 0.1):
        common.cylinder("Push bar post", Vector((xx, 1.05, zf - 0.035)), 0.012, 0.07, "z", chrome, segments=10)
common.panel("Exit sign", ((ENTRY_X0 + ENTRY_X1) / 2, ENTRY_H + 0.95, Z1 - 0.08), (-1, 0, 0), (0, 1, 0), 0.36, 0.12,
             exit_sign)
box("Exit sign box", ((ENTRY_X0 + ENTRY_X1) / 2 - 0.2, ENTRY_H + 0.87, Z1 - 0.075),
    ((ENTRY_X0 + ENTRY_X1) / 2 + 0.2, ENTRY_H + 1.03, Z1), steel, 0.004)
box("Door mat", (ENTRY_X0 - 0.1, 0.0, Z1 - 1.0), (ENTRY_X1 + 0.1, 0.012, Z1 - 0.06), rubber, 0.004)

# ---- The two unfinished rooms behind the arches ----
# Doorway A: concrete floor, studs on one side, bare drywall on the other,
# exposed joists, a back wall of plywood.
ax0, ax1 = DOOR_A_X0 - 0.4, DOOR_A_X1 + 0.4
az0 = Z0 - T - CORRIDOR_DEPTH
box("Room 2 floor", (ax0, -0.1, az0), (ax1, 0.0, Z0 - T + 0.01), concrete, 0, grain_axis="z")
box("Room 2 ceiling", (ax0, CORRIDOR_H, az0), (ax1, CORRIDOR_H + 0.1, Z0 - T), drywall, 0)
box("Room 2 back", (ax0, 0, az0 - 0.05), (ax1, CORRIDOR_H, az0), studs, 0)
box("Room 2 drywall", (ax1, 0, az0), (ax1 + 0.03, CORRIDOR_H, Z0 - T), drywall, 0, grain_axis="z")
box("Room 2 side", (ax0 - 0.1, 0, az0), (ax0 - 0.08, CORRIDOR_H, Z0 - T), drywall, 0, grain_axis="z")
for zz in np.arange(az0 + 0.1, Z0 - T, 0.4):
    box("Stud", (ax0 - 0.08, 0, zz), (ax0 + 0.01, CORRIDOR_H, zz + 0.04), studs, 0.002, grain_axis="y")
    box("Joist", (ax0, CORRIDOR_H - 0.2, zz), (ax1, CORRIDOR_H, zz + 0.05), studs, 0.002)
box("Sole plate", (ax0 - 0.08, 0, az0), (ax0 + 0.01, 0.04, Z0 - T), studs, 0.002, grain_axis="z")
box("Top plate", (ax0 - 0.08, CORRIDOR_H - 0.24, az0), (ax0 + 0.01, CORRIDOR_H - 0.2, Z0 - T), studs, 0.002,
    grain_axis="z")
# Doorway B (left wall): the same, turned.
bz0, bz1 = DOOR_B_Z0 - 0.4, DOOR_B_Z1 + 0.4
bx0 = X0 - T - CORRIDOR_DEPTH
box("Room 3 floor", (bx0, -0.1, bz0), (X0 - T + 0.01, 0.0, bz1), concrete, 0)
box("Room 3 ceiling", (bx0, CORRIDOR_H, bz0), (X0 - T, CORRIDOR_H + 0.1, bz1), drywall, 0)
box("Room 3 back", (bx0 - 0.05, 0, bz0), (bx0, CORRIDOR_H, bz1), studs, 0, grain_axis="z")
box("Room 3 drywall", (bx0, 0, bz0 - 0.03), (X0 - T, CORRIDOR_H, bz0), drywall, 0)
box("Room 3 side", (bx0, 0, bz1 + 0.08), (X0 - T, CORRIDOR_H, bz1 + 0.1), drywall, 0)
for xx in np.arange(bx0 + 0.1, X0 - T, 0.4):
    box("Stud", (xx, 0, bz1 - 0.01), (xx + 0.04, CORRIDOR_H, bz1 + 0.08), studs, 0.002, grain_axis="y")
    box("Joist", (xx, CORRIDOR_H - 0.2, bz0), (xx + 0.05, CORRIDOR_H, bz1), studs, 0.002, grain_axis="z")

# ---- Stage ----
stage_outline = [(X0, STAGE_Z0), (STAGE_X1, STAGE_Z0), (STAGE_X1, Z1), (X0, Z1)]
common.prism("Stage", stage_outline, 0.0, STAGE_H - 0.03, steel)
box("Stage top", (X0, STAGE_H - 0.03, STAGE_Z0), (STAGE_X1, STAGE_H, Z1), stage_wood, 0.004, grain_axis="z")
step_outline = [(X0, STAGE_Z0 - STEP_W), (STAGE_X1 + STEP_W, STAGE_Z0 - STEP_W), (STAGE_X1 + STEP_W, Z1),
                (STAGE_X1, Z1), (STAGE_X1, STAGE_Z0), (X0, STAGE_Z0)]
common.prism("Step", step_outline, 0.0, STEP_H - 0.02, steel)
for (lo, hi, ax) in (((X0, STEP_H - 0.02, STAGE_Z0 - STEP_W), (STAGE_X1 + STEP_W, STEP_H, STAGE_Z0), "x"),
                     ((STAGE_X1, STEP_H - 0.02, STAGE_Z0), (STAGE_X1 + STEP_W, STEP_H, Z1), "z")):
    box("Step tread", lo, hi, stage_wood, 0.004, grain_axis=ax)
# Brass nosing on the step and the stage edge.
for (y, off) in ((STEP_H, STEP_W), (STAGE_H, 0.0)):
    box("Nosing", (X0, y - 0.02, STAGE_Z0 - off - 0.01), (STAGE_X1 + off + 0.01, y + 0.003, STAGE_Z0 - off + 0.03),
        brass, 0.003)
    box("Nosing", (STAGE_X1 + off - 0.03, y - 0.02, STAGE_Z0 - off - 0.01), (STAGE_X1 + off + 0.01, y + 0.003, Z1),
        brass, 0.003, grain_axis="z")

# ---- Rubber mats behind the bar ----
for x in np.arange(BACK_BAR_X0 + 0.2, BACK_BAR_X1 - 0.6, 1.25):
    box("Bar mat", (x, 0.0, Z0 + 0.62), (x + 1.2, 0.016, Z0 + 1.42), rubber, 0.006)

common.finish_and_export(out_path, uv_scale={
    "Parquet": 0.45, "Brick": 0.75, "Dark brick": 0.6, "Green plaster": 0.35, "Ceiling": 0.25,
    "Wainscot oak": 1.0, "Soffit wood": 1.0, "Stage boards": 0.5, "Door wood": 1.0, "Concrete": 0.4,
    "Pine studs": 1.0, "Rubber mat": 1.2, "Seat velvet": 1.5, "Pillow fabric": 2.0})
