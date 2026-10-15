# Builds the seating-area furniture: the round pub tables, the small square
# booth tables, and the long channel-tufted leather booth.
#
#   blender -b --factory-startup --python tools/assets/furniture.py -- <texture dir> <out dir>
#
# Tables stand on their origin (center of the base on the floor). The booth's
# back is at x = 0 (against the wall), its seat faces +x, and it runs along z
# (centered).
import math
import os
import sys

from mathutils import Matrix, Vector

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402

tex_dir, out_dir = sys.argv[sys.argv.index("--") + 1:][:2]


def table_materials():
    top = common.pbr_material("Table top", tex_dir, "wood_table_001", tint=(1.0, 0.9, 0.82), roughness_scale=0.45,
                              roughness_bias=0.04, grain="v", size=512)
    iron = common.plain_material("Cast iron", (0.035, 0.033, 0.03), 0.7, 0.5)
    brass = common.plain_material("Brass", (0.91, 0.70, 0.38), 1.0, 0.3)
    return top, iron, brass


def pedestal(iron, brass, top_y, foot_len):
    """A cast-iron column with turned rings, on a four-footed cross base."""
    common.lathe("Column", [(0.0, 0.05), (0.07, 0.05), (0.075, 0.07), (0.05, 0.09), (0.035, 0.12),
                            (0.032, 0.25), (0.045, 0.28), (0.032, 0.31), (0.03, top_y - 0.12),
                            (0.042, top_y - 0.1), (0.03, top_y - 0.08), (0.03, top_y - 0.06),
                            (0.12, top_y - 0.05), (0.12, top_y - 0.035), (0.0, top_y - 0.035)], iron, 24)
    for k in range(4):
        foot = common.box("Foot", (0.0, 0.0, -0.028), (foot_len, 0.05, 0.028), iron, 0.012, 2)
        # Taper: lower the outer end a bit for a cast look.
        for v in foot.data.vertices:
            if v.co.x > 0.05 and v.co.y > 0.02:
                v.co.y -= 0.02 * (v.co.x / foot_len)
        common.rotated_y(foot, math.pi / 4 + k * math.pi / 2)
        a = math.pi / 4 + k * math.pi / 2
        pad = Vector((math.cos(a) * (foot_len - 0.03), 0.004, -math.sin(a) * (foot_len - 0.03)))
        common.cylinder("Leveler", pad, 0.022, 0.008, "y", brass, segments=12)


def round_table():
    common.reset_scene()
    top, iron, brass = table_materials()
    r, y1 = 0.45, 0.785
    # Thick top with a softly rounded (bullnose) edge, eased underside.
    t = common.lathe("Top", [(0.0, y1 - 0.045), (r - 0.03, y1 - 0.045), (r - 0.008, y1 - 0.038),
                             (r, y1 - 0.025), (r - 0.002, y1 - 0.012), (r - 0.012, y1 - 0.002), (r - 0.02, y1),
                             (0.0, y1)], top, 56, smooth=False)
    t["smooth"] = True
    pedestal(iron, brass, y1 - 0.045 + 0.035, 0.34)
    common.finish_and_export(os.path.join(out_dir, "round_table.glb"), uv_scale={"Table top": 1.2})


def booth_table():
    common.reset_scene()
    top, iron, brass = table_materials()
    y1 = 0.765
    common.box("Top", (-0.375, y1 - 0.04, -0.375), (0.375, y1, 0.375), top, 0.014, 4)
    # Apron under the top edge.
    common.box("Apron", (-0.33, y1 - 0.1, -0.33), (0.33, y1 - 0.04, 0.33), iron, 0.004)
    pedestal(iron, brass, y1 - 0.1 + 0.035, 0.30)
    common.finish_and_export(os.path.join(out_dir, "booth_table.glb"), uv_scale={"Table top": 1.2})


def booth():
    common.reset_scene()
    L = 3.6
    HL = L / 2
    leather = common.pbr_material("Booth leather", tex_dir, "leather_red_02", tint=(0.9, 0.72, 0.72),
                                  roughness_scale=0.75, size=512)
    seam = common.plain_material("Leather seam", (0.10, 0.02, 0.02), 0.0, 0.5)
    wood = common.pbr_material("Booth wood", tex_dir, "dark_wood", tint=(0.55, 0.45, 0.4), roughness_scale=0.55,
                               size=512)
    kick = common.plain_material("Black kick plate", (0.02, 0.02, 0.02), 0.0, 0.6)

    def wbox(name, lo, hi, mat, bevel=0.006, segments=2, grain_axis="z"):
        obj = common.box(name, lo, hi, mat, bevel, segments)
        obj["grain_axis"] = grain_axis
        return obj

    # Wooden base with a recessed toe kick.
    wbox("Base", (0.0, 0.08, -HL + 0.05), (0.6, 0.3, HL - 0.05), wood, 0.008)
    wbox("Toe kick", (0.0, 0.0, -HL + 0.05), (0.55, 0.08, HL - 0.05), kick, 0.002)
    wbox("Base trim", (0.0, 0.28, -HL + 0.05), (0.62, 0.31, HL - 0.05), wood, 0.01, 3)
    # Seat: three plump cushions with a rolled front edge.
    n = 3
    gap = 0.012
    seg = (L - 0.1 - gap * (n - 1)) / n
    for i in range(n):
        z0 = -HL + 0.05 + i * (seg + gap)
        c = common.box("Seat cushion", (0.12, 0.31, z0), (0.66, 0.46, z0 + seg), leather, 0.05, 5)
        c["uv_scale"] = 2.5
        common.tube("Seat welt", [Vector((0.66 - 0.006, 0.445, z0 + 0.04)), Vector((0.66 - 0.006, 0.445, z0 + seg - 0.04))],
                    0.006, seam, sides=8)
    # Back: vertical channel-tufted rolls, leaning back 8 degrees.
    lean = Matrix.Translation((0.17, 0.46, 0)) @ Matrix.Rotation(math.radians(8), 4, "Z") @ \
        Matrix.Translation((-0.17, -0.46, 0))
    channels = 22
    cw = (L - 0.1) / channels
    for i in range(channels):
        z0 = -HL + 0.05 + i * cw
        roll = common.box("Back channel", (0.04, 0.47, z0 + 0.003), (0.17, 1.12, z0 + cw - 0.003), leather, 0.045, 4)
        roll["uv_scale"] = 2.5
        common.transform(roll, lean)
    # The frame behind the back and the wooden cap rail on top.
    wbox("Back frame", (0.0, 0.31, -HL + 0.05), (0.06, 1.18, HL - 0.05), wood, 0.004)
    wbox("Cap rail", (-0.01, 1.16, -HL + 0.03), (0.2, 1.21, HL - 0.03), wood, 0.016, 3)
    # End panels with a rounded top front corner.
    for z in (-HL, HL - 0.05):
        wbox("End panel", (0.0, 0.0, z), (0.68, 0.62, z + 0.05), wood, 0.012, 3, "x")
        wbox("End panel back", (0.0, 0.62, z), (0.22, 1.24, z + 0.05), wood, 0.012, 3, "y")
        wbox("End arm", (0.0, 0.6, z - 0.01), (0.7, 0.65, z + 0.06), wood, 0.02, 4, "x")
    common.finish_and_export(os.path.join(out_dir, "booth.glb"), uv_scale={"Booth wood": 0.9})


round_table()
booth_table()
booth()
