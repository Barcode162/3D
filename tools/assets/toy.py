# Shared helpers for the toy-like 3D UI pictures (badges.py, icons.py): glossy
# materials, rounded shapes, and a studio render to a transparent PNG.
import math
import os

import bmesh
import bpy
from mathutils import Vector


# ---- Materials ----
def mat(name, color, metallic=0.0, rough=0.35, coat=0.0, emit=None, strength=0.0, transmission=0.0):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*color, 1)
    b.inputs["Metallic"].default_value = metallic
    b.inputs["Roughness"].default_value = rough
    b.inputs["Coat Weight"].default_value = coat
    b.inputs["Coat Roughness"].default_value = 0.08
    if transmission:
        b.inputs["Transmission Weight"].default_value = transmission
    if emit:
        b.inputs["Emission Color"].default_value = (*emit, 1)
        b.inputs["Emission Strength"].default_value = strength
    return m


def gold():
    return mat("Gold", (1.0, 0.68, 0.18), 1.0, 0.22)


def silver():
    return mat("Silver", (0.85, 0.87, 0.9), 1.0, 0.2)


def enamel(name, color):
    return mat(name, color, 0.0, 0.3, coat=1.0)


# ---- Shapes ----
def obj_from(bm, name, m, smooth=True):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    o = bpy.data.objects.new(name, me)
    bpy.context.collection.objects.link(o)
    o.data.materials.append(m)
    if smooth:
        for p in o.data.polygons:
            p.use_smooth = True
    return o


def extrude(name, outline, z0, z1, m, bevel=0.05, segs=4):
    """A flat outline [(x, y)...] made into a slab from z0 to z1 with rounded edges."""
    bm = bmesh.new()
    vs = [bm.verts.new((x, y, z0)) for x, y in outline]
    f = bm.faces.new(vs)
    bmesh.ops.recalc_face_normals(bm, faces=[f])
    if f.normal.z > 0:
        f.normal_flip()
    r = bmesh.ops.extrude_face_region(bm, geom=[f])
    top = [e for e in r["geom"] if isinstance(e, bmesh.types.BMVert)]
    bmesh.ops.translate(bm, vec=(0, 0, z1 - z0), verts=top)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    o = obj_from(bm, name, m, smooth=False)
    if bevel > 0:
        b = o.modifiers.new("bevel", "BEVEL")
        b.width = bevel
        b.segments = segs
        b.limit_method = "ANGLE"
        b.harden_normals = False
    o.modifiers.new("ws", "WEIGHTED_NORMAL")
    for p in o.data.polygons:
        p.use_smooth = True
    return o


def tube(name, points, radius, m, closed=False):
    cu = bpy.data.curves.new(name, "CURVE")
    cu.dimensions = "3D"
    cu.bevel_depth = radius
    cu.bevel_resolution = 6
    cu.use_fill_caps = True
    sp = cu.splines.new("POLY")
    sp.points.add(len(points) - 1)
    for p, c in zip(sp.points, points):
        p.co = (*c, 1)
    sp.use_cyclic_u = closed
    o = bpy.data.objects.new(name, cu)
    bpy.context.collection.objects.link(o)
    o.data.materials.append(m)
    return o


def sphere(name, c, r, m):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=32, v_segments=16, radius=r)
    bmesh.ops.translate(bm, vec=c, verts=bm.verts)
    return obj_from(bm, name, m)


def lathe(name, profile, m, center=(0, 0, 0), segs=48):
    bm = bmesh.new()
    rings = []
    for k in range(segs):
        a = 2 * math.pi * k / segs
        rings.append([bm.verts.new((center[0] + r * math.cos(a), center[1] + y, center[2] + r * math.sin(a)))
                      for r, y in profile])
    for k in range(segs):
        ra, rb = rings[k], rings[(k + 1) % segs]
        for i in range(len(profile) - 1):
            bm.faces.new([ra[i], ra[i + 1], rb[i + 1], rb[i]])
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return obj_from(bm, name, m)


def cylinder_z(name, c, r, depth, m, segs=48):
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=segs, radius1=r, radius2=r, depth=depth)
    bmesh.ops.translate(bm, vec=c, verts=bm.verts)
    o = obj_from(bm, name, m, smooth=False)
    b = o.modifiers.new("bevel", "BEVEL")
    b.width = min(depth * 0.3, r * 0.3)
    b.segments = 3
    b.limit_method = "ANGLE"
    for p in o.data.polygons:
        p.use_smooth = True
    return o


def arc_pts(cx, cy, r, a0, a1, n, z=0.0):
    return [(cx + r * math.cos(a0 + (a1 - a0) * i / n), cy + r * math.sin(a0 + (a1 - a0) * i / n), z) for i in range(n + 1)]


def shield_outline(s=1.0):
    """A heraldic shield: rounded top corners, curved sides to a point."""
    pts = []
    w, top = 0.82 * s, 0.86 * s
    pts += [(x, y) for x, y, _ in arc_pts(-w + 0.18 * s, top - 0.18 * s, 0.18 * s, math.pi, math.pi / 2, 8)]
    pts += [(x, y) for x, y, _ in arc_pts(w - 0.18 * s, top - 0.18 * s, 0.18 * s, math.pi / 2, 0, 8)]
    # Straight sides for the top part, then curving in to the point.
    y0, y1 = top - 0.18 * s, -1.0 * s
    side = []
    for i in range(1, 21):
        t = i / 20
        k = max(0.0, (t - 0.3) / 0.7)
        side.append((w * (1 - k ** 1.7), y0 + (y1 - y0) * t))
    pts += side
    pts += [(-x, y) for x, y in reversed(side[:-1])]
    return pts


def star_outline(r0, r1, n=5, rot=math.pi / 2):
    return [((r1 if i % 2 == 0 else r0) * math.cos(rot + i * math.pi / n), (r1 if i % 2 == 0 else r0) * math.sin(rot + i * math.pi / n))
            for i in range(2 * n)]


def turn_all(rx=-10, ry=16):
    """Parents everything to one empty, turned a little for depth."""
    root = bpy.data.objects.new("Root", None)
    bpy.context.collection.objects.link(root)
    for o in list(bpy.context.collection.objects):
        if o is not root:
            o.parent = root
    root.rotation_euler = (math.radians(rx), math.radians(ry), 0)
    return root


def render(path, size, ortho=2.6, cam_y=-0.06):
    sc = bpy.context.scene
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = ortho
    cam.location = (0, cam_y, 6)
    sc.collection.objects.link(cam)
    sc.camera = cam
    for name, loc, energy, lsize, color in (("Key", (-3, 4, 5), 900, 3, (1, 0.97, 0.92)), ("Fill", (4, -1, 4), 250, 4, (0.8, 0.88, 1)),
                                           ("Rim", (2, 4, -3), 600, 2, (1, 1, 1))):
        L = bpy.data.lights.new(name, "AREA")
        L.energy = energy
        L.size = lsize
        L.color = color
        lo = bpy.data.objects.new(name, L)
        lo.location = loc
        lo.rotation_euler = (Vector((0, 0, 0)) - Vector(loc)).to_track_quat("-Z", "Y").to_euler()
        sc.collection.objects.link(lo)
    world = bpy.data.worlds.new("W")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs[0].default_value = (0.5, 0.5, 0.55, 1)
    world.node_tree.nodes["Background"].inputs[1].default_value = 0.6
    sc.world = world
    sc.render.engine = "CYCLES"
    sc.cycles.samples = 64
    sc.cycles.use_denoising = True
    sc.cycles.device = "CPU"
    sc.render.film_transparent = True
    sc.render.resolution_x = sc.render.resolution_y = size
    sc.render.image_settings.file_format = "PNG"
    sc.render.image_settings.color_mode = "RGBA"
    sc.view_settings.view_transform = "Standard"
    sc.view_settings.look = "Medium High Contrast"
    sc.render.filepath = path
    bpy.ops.render.render(write_still=True)
    return path
