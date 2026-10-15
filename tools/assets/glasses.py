# Builds the bar's glassware as .glb files (lathed profiles, real wall
# thickness, see-through glass material).
#
#   blender -b --factory-startup --python tools/assets/glasses.py -- <out dir>
#
# Each glass stands on its origin (bottom center), y up.
import math
import os
import sys

import bmesh
import bpy

sys.path.insert(0, os.path.dirname(__file__))
import common  # noqa: E402

out_dir = sys.argv[sys.argv.index("--") + 1]


def glass_material():
    mat = bpy.data.materials.new("Glass")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (0.86, 0.93, 0.92, 1.0)
    bsdf.inputs["Metallic"].default_value = 0.0
    bsdf.inputs["Roughness"].default_value = 0.04
    bsdf.inputs["Alpha"].default_value = 0.16
    if hasattr(mat, "surface_render_method"):
        mat.surface_render_method = "BLENDED"
    if hasattr(mat, "blend_method"):
        mat.blend_method = "BLEND"
    return mat


def lathe(name, profile, mat, segments=40):
    """Spins a (radius, height) profile around the y axis into a closed solid.

    The profile runs from the outer bottom center, up the outside, over the
    rim and down the inside to the inner bottom center.
    """
    bm = bmesh.new()
    rings = []
    for k in range(segments):
        a = 2 * math.pi * k / segments
        ring = []
        for (r, y) in profile:
            ring.append(bm.verts.new((r * math.cos(a), y, -r * math.sin(a))))
        rings.append(ring)
    n = len(profile)
    for k in range(segments):
        ra, rb = rings[k], rings[(k + 1) % segments]
        for i in range(n - 1):
            if profile[i][0] == 0 and profile[i + 1][0] == 0:
                continue
            verts = [ra[i], rb[i], rb[i + 1], ra[i + 1]]
            bm.faces.new(verts)
    # Collapse the duplicate center vertices (radius 0) into points.
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=1e-7)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    obj = common._new_object(name, bm, mat)
    obj["smooth"] = True
    return obj


def rounded_rim(r_out, r_in, y, steps=4):
    """Points over a half-round rim from the outer wall to the inner wall."""
    mid = (r_out + r_in) / 2
    rad = (r_out - r_in) / 2
    return [(mid + rad * math.cos(math.pi * s / steps), y + rad * math.sin(math.pi * s / steps))
            for s in range(steps + 1)]


def export(name, build):
    common.reset_scene()
    build(glass_material())
    common.finish_and_export(os.path.join(out_dir, name + ".glb"), uv_scale={})


def pint(mat):
    # A shaker pint: 15 cm tall, tapering from 6 cm to 8.6 cm across.
    h, base, top, wall, floor = 0.150, 0.030, 0.043, 0.0028, 0.012
    profile = [(0.0, 0.0), (base - 0.002, 0.0), (base, 0.002), (base + 0.0005, 0.008)]
    profile += rounded_rim(top, top - wall, h - wall / 2)
    profile += [(base + 0.0005 - wall, floor + 0.004), (base - wall - 0.004, floor), (0.0, floor)]
    lathe("Pint glass", profile, mat)


def rocks(mat):
    # A heavy-based rocks (old fashioned) glass: 9 cm tall, 8 cm across.
    h, r, wall, floor = 0.090, 0.040, 0.0035, 0.018
    profile = [(0.0, 0.0), (r - 0.003, 0.0), (r, 0.003)]
    profile += rounded_rim(r, r - wall, h - wall / 2)
    profile += [(r - wall, floor + 0.004), (r - wall - 0.004, floor), (0.0, floor)]
    lathe("Rocks glass", profile, mat)


export("pint_glass", pint)
export("rocks_glass", rocks)
