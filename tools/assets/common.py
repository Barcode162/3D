# Shared helpers for the Blender asset scripts in this folder (run inside
# Blender: `blender -b --factory-startup --python <script> -- ...`).
#
# Conventions: geometry is built directly in model space with y up (the
# engine's convention), so export keeps coordinates as-is (export_yup=False).
# Wood UVs are projected per face from the box sides, with the wood grain
# running along each piece's long direction (the object's "grain_axis").
import math
import os
import tempfile

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

_tmp = tempfile.mkdtemp(prefix="bar3d_assets_")


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def _load_pixels(path, size=None):
    img = bpy.data.images.load(path)
    if size and img.size[0] != size:
        img.scale(size, size)  # smaller textures for small props
    w, h = img.size
    px = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(px)
    bpy.data.images.remove(img)
    return w, h, px.reshape(h, w, 4)


def _save_image(name, w, h, px, color, alpha=False):
    img = bpy.data.images.new(name, w, h, alpha=alpha)
    img.colorspace_settings.name = "sRGB" if color else "Non-Color"
    img.pixels.foreach_set(px.astype(np.float32).ravel())
    path = os.path.join(_tmp, name.replace(" ", "_") + ".png")
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    img.colorspace_settings.name = "sRGB" if color else "Non-Color"
    return img


def wood_material(name, tex_dir, prefix, tint=(1, 1, 1), roughness_scale=1.0, roughness_bias=0.0, grain="u",
                  size=None, normal_strength=1.0, saturation=1.0):
    """A PBR material from a Poly Haven texture set (wood, brick, leather...).

    The color map is tinted, and the roughness map is scaled and packed into
    the glTF metal/roughness layout (G = roughness, B = metallic = 0).
    `grain` says which texture axis the grain runs along ("u" or "v").
    `size` shrinks the images (e.g. 512 for small props).
    """
    w, h, diff = _load_pixels(os.path.join(tex_dir, f"{prefix}_diff_1k.jpg"), size)
    if saturation != 1.0:
        gray = diff[..., :3].mean(axis=2, keepdims=True)
        diff[..., :3] = gray + (diff[..., :3] - gray) * saturation
    # Blender stores sRGB images as linear floats once loaded; tint in linear.
    diff[..., 0] *= tint[0]
    diff[..., 1] *= tint[1]
    diff[..., 2] *= tint[2]
    base = _save_image(f"{name} color", w, h, np.clip(diff, 0, 1), color=True)

    _, _, rough = _load_pixels(os.path.join(tex_dir, f"{prefix}_rough_1k.jpg"), size)
    mr = np.zeros_like(rough)
    mr[..., 0] = 1.0
    mr[..., 1] = np.clip(rough[..., 0] * roughness_scale + roughness_bias, 0.02, 1.0)
    mr[..., 2] = 0.0
    mr[..., 3] = 1.0
    mr_img = _save_image(f"{name} metal-rough", w, h, mr, color=False)

    _, _, nrm = _load_pixels(os.path.join(tex_dir, f"{prefix}_nor_gl_1k.jpg"), size)
    nrm_img = _save_image(f"{name} normal", w, h, nrm, color=False)

    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    t_base = nt.nodes.new("ShaderNodeTexImage")
    t_base.image = base
    nt.links.new(t_base.outputs["Color"], bsdf.inputs["Base Color"])
    t_mr = nt.nodes.new("ShaderNodeTexImage")
    t_mr.image = mr_img
    sep = nt.nodes.new("ShaderNodeSeparateColor")
    nt.links.new(t_mr.outputs["Color"], sep.inputs["Color"])
    nt.links.new(sep.outputs["Green"], bsdf.inputs["Roughness"])
    nt.links.new(sep.outputs["Blue"], bsdf.inputs["Metallic"])
    t_n = nt.nodes.new("ShaderNodeTexImage")
    t_n.image = nrm_img
    nmap = nt.nodes.new("ShaderNodeNormalMap")
    nmap.inputs["Strength"].default_value = normal_strength
    nt.links.new(t_n.outputs["Color"], nmap.inputs["Color"])
    nt.links.new(nmap.outputs["Normal"], bsdf.inputs["Normal"])
    mat["grain"] = grain
    return mat


pbr_material = wood_material


def plain_material(name, color, metallic, roughness, emission=None, strength=1.0):
    """A flat-colored material. `emission` (rgb) makes it glow by itself;
    strengths above 1 export as KHR_materials_emissive_strength."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    if emission is not None:
        bsdf.inputs["Emission Color"].default_value = (*emission, 1.0)
        bsdf.inputs["Emission Strength"].default_value = strength
    mat["grain"] = "u"
    return mat


def glass_material(name="Glass", color=(0.86, 0.93, 0.92), alpha=0.16, roughness=0.04):
    """See-through glass (glTF alphaMode BLEND): drawn after solid things."""
    mat = plain_material(name, color, 0.0, roughness)
    mat.node_tree.nodes["Principled BSDF"].inputs["Alpha"].default_value = alpha
    if hasattr(mat, "surface_render_method"):
        mat.surface_render_method = "BLENDED"
    if hasattr(mat, "blend_method"):
        mat.blend_method = "BLEND"
    return mat


def image_material(name, pixels, metallic=0.0, roughness=0.5, rough_pixels=None, normal_pixels=None):
    """A material from images made in numpy: `pixels` is (h, w, 3 or 4) linear
    color with row 0 at the bottom (v = 0). `rough_pixels` (h, w) is optional
    roughness, `normal_pixels` (h, w, 3) an optional normal map (0..1)."""
    h, w = pixels.shape[:2]
    px = np.ones((h, w, 4), dtype=np.float32)
    px[..., :3] = np.clip(pixels[..., :3], 0, 1)
    base = _save_image(f"{name} color", w, h, px, color=True)
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    t_base = nt.nodes.new("ShaderNodeTexImage")
    t_base.image = base
    nt.links.new(t_base.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    if rough_pixels is not None:
        rh, rw = rough_pixels.shape[:2]
        mr = np.zeros((rh, rw, 4), dtype=np.float32)
        mr[..., 0] = 1.0
        mr[..., 1] = np.clip(rough_pixels, 0.02, 1.0)
        mr[..., 2] = metallic
        mr[..., 3] = 1.0
        t_mr = nt.nodes.new("ShaderNodeTexImage")
        t_mr.image = _save_image(f"{name} metal-rough", rw, rh, mr, color=False)
        sep = nt.nodes.new("ShaderNodeSeparateColor")
        nt.links.new(t_mr.outputs["Color"], sep.inputs["Color"])
        nt.links.new(sep.outputs["Green"], bsdf.inputs["Roughness"])
        nt.links.new(sep.outputs["Blue"], bsdf.inputs["Metallic"])
    if normal_pixels is not None:
        nh, nw = normal_pixels.shape[:2]
        npx = np.ones((nh, nw, 4), dtype=np.float32)
        npx[..., :3] = normal_pixels
        t_n = nt.nodes.new("ShaderNodeTexImage")
        t_n.image = _save_image(f"{name} normal", nw, nh, npx, color=False)
        nmap = nt.nodes.new("ShaderNodeNormalMap")
        nt.links.new(t_n.outputs["Color"], nmap.inputs["Color"])
        nt.links.new(nmap.outputs["Normal"], bsdf.inputs["Normal"])
    mat["grain"] = "u"
    return mat


def height_to_normal(height, strength=1.0):
    """A tangent-space normal map (0..1, OpenGL style) from a height map."""
    gy, gx = np.gradient(height.astype(np.float32))
    n = np.stack([-gx * strength, -gy * strength, np.ones_like(height, dtype=np.float32)], axis=-1)
    n /= np.linalg.norm(n, axis=-1, keepdims=True)
    return n * 0.5 + 0.5


def mesh_object(name, bm, mat, smooth=False):
    """Turns a bmesh into a scene object (frees the bmesh)."""
    obj = _new_object(name, bm, mat)
    if smooth:
        obj["smooth"] = True
    return obj


def _new_object(name, bm, mat):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    mesh.materials.append(mat)
    return obj


def _bevel(obj, width, segments):
    if width <= 0:
        return
    mod = obj.modifiers.new("Bevel", "BEVEL")
    mod.width = width
    mod.segments = segments
    mod.limit_method = "ANGLE"
    mod.angle_limit = math.radians(40)
    mod.harden_normals = True  # flat faces stay flat, the rounded edge catches light


def box(name, lo, hi, mat, bevel=0.006, segments=2):
    """An axis-aligned box from corner `lo` to corner `hi` with rounded edges."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    size = Vector(hi) - Vector(lo)
    center = (Vector(hi) + Vector(lo)) / 2
    bmesh.ops.transform(bm, matrix=Matrix.Translation(center) @ Matrix.Diagonal((*size, 1)), verts=bm.verts)
    obj = _new_object(name, bm, mat)
    _bevel(obj, min(bevel, min(size) / 2.1), segments)
    return obj


def cylinder(name, center, radius, depth, axis, mat, segments=24, bevel=0.002):
    """A cylinder along world axis "x", "y" or "z"."""
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=segments, radius1=radius, radius2=radius, depth=depth)
    rot = {"x": Matrix.Rotation(math.pi / 2, 4, "Y"), "y": Matrix.Rotation(-math.pi / 2, 4, "X"),
           "z": Matrix.Identity(4)}[axis]
    bmesh.ops.transform(bm, matrix=Matrix.Translation(center) @ rot, verts=bm.verts)
    obj = _new_object(name, bm, mat)
    obj["smooth"] = True
    if radius >= 0.025:  # tiny parts (keys, caps, pins) don't need rounded edges
        _bevel(obj, min(bevel, radius / 3), 2)
    return obj


def sphere(name, center, radius, mat, segments=24, rings=12):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segments, v_segments=rings, radius=radius)
    bmesh.ops.translate(bm, vec=center, verts=bm.verts)
    obj = _new_object(name, bm, mat)
    obj["smooth"] = True
    return obj


def transform(obj, matrix):
    """Moves/turns an object's mesh in model space (before export)."""
    obj.data.transform(matrix)
    return obj


def rotated_y(obj, angle, pivot=(0, 0, 0)):
    p = Vector(pivot)
    return transform(obj, Matrix.Translation(p) @ Matrix.Rotation(angle, 4, "Y") @ Matrix.Translation(-p))


def lathe(name, profile, mat, segments=40, smooth=True):
    """Spins a (radius, height) profile around the y axis into a solid.

    For a closed solid the profile starts and ends on the axis (radius 0).
    """
    bm = bmesh.new()
    rings = []
    for k in range(segments):
        a = 2 * math.pi * k / segments
        rings.append([bm.verts.new((r * math.cos(a), y, -r * math.sin(a))) for (r, y) in profile])
    n = len(profile)
    for k in range(segments):
        ra, rb = rings[k], rings[(k + 1) % segments]
        for i in range(n - 1):
            if profile[i][0] == 0 and profile[i + 1][0] == 0:
                continue
            bm.faces.new([ra[i], rb[i], rb[i + 1], ra[i + 1]])
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-6)
    bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=1e-7)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return mesh_object(name, bm, mat, smooth)


def torus(name, center, major, minor, mat, axis="y", segments=32, sides=12):
    """A ring (like a foot ring) around `axis` through `center`."""
    bm = bmesh.new()
    rings = []
    for i in range(segments):
        a = 2 * math.pi * i / segments
        ring = []
        for j in range(sides):
            b = 2 * math.pi * j / sides
            r = major + minor * math.cos(b)
            ring.append(bm.verts.new((r * math.cos(a), minor * math.sin(b), r * math.sin(a))))
        rings.append(ring)
    for i in range(segments):
        ra, rb = rings[i], rings[(i + 1) % segments]
        for j in range(sides):
            bm.faces.new([ra[j], ra[(j + 1) % sides], rb[(j + 1) % sides], rb[j]])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    rot = {"x": Matrix.Rotation(-math.pi / 2, 4, "Z"), "y": Matrix.Identity(4),
           "z": Matrix.Rotation(math.pi / 2, 4, "X")}[axis]
    bmesh.ops.transform(bm, matrix=Matrix.Translation(center) @ rot, verts=bm.verts)
    return mesh_object(name, bm, mat, smooth=True)


def tube(name, points, radius, mat, sides=10, closed=False, caps=True):
    """A round tube along a polyline (neon tubes, cords, bent rods)."""
    pts = [Vector(p) for p in points]
    n = len(pts)
    bm = bmesh.new()

    def tangent(i):
        if closed:
            return (pts[(i + 1) % n] - pts[i - 1]).normalized()
        a, b = pts[max(i - 1, 0)], pts[min(i + 1, n - 1)]
        return (b - a).normalized()

    t0 = tangent(0)
    ref = Vector((0, 1, 0)) if abs(t0.y) < 0.9 else Vector((1, 0, 0))
    normal = t0.cross(ref).normalized()
    rings = []
    for i in range(n):
        t = tangent(i)
        normal = (normal - t * normal.dot(t)).normalized()  # parallel transport
        binormal = t.cross(normal)
        rings.append([bm.verts.new(pts[i] + radius * (math.cos(2 * math.pi * j / sides) * normal +
                                                       math.sin(2 * math.pi * j / sides) * binormal))
                      for j in range(sides)])
    for i in range(n if closed else n - 1):
        ra, rb = rings[i], rings[(i + 1) % n]
        for j in range(sides):
            bm.faces.new([ra[j], ra[(j + 1) % sides], rb[(j + 1) % sides], rb[j]])
    if caps and not closed:
        bm.faces.new(list(reversed(rings[0])))
        bm.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return mesh_object(name, bm, mat, smooth=True)


def arc(center, radius, a0, a1, steps, plane="xy"):
    """Points on a circular arc (angles in radians) in a coordinate plane."""
    out = []
    for k in range(steps + 1):
        a = a0 + (a1 - a0) * k / steps
        u, v = radius * math.cos(a), radius * math.sin(a)
        c = Vector(center)
        out.append(c + (Vector((u, v, 0)) if plane == "xy" else Vector((u, 0, v)) if plane == "xz"
                        else Vector((0, v, u))))
    return out


def set_uvs(obj, fn):
    """Gives an object its own UVs: fn(vertex position, face normal) -> (u, v).
    The export then keeps them instead of box-projecting."""
    mesh = obj.data
    bm = bmesh.new()
    bm.from_mesh(mesh)
    uv = bm.loops.layers.uv.verify()
    for face in bm.faces:
        for loop in face.loops:
            loop[uv].uv = fn(loop.vert.co, face.normal)
    bm.to_mesh(mesh)
    bm.free()
    obj["keep_uv"] = True


def swatch_uvs(obj, u, v, spread=0.004):
    """Points all of an object's UVs at one spot of an atlas texture (with a
    tiny spread so tangents stay well defined)."""
    set_uvs(obj, lambda co, n: (u + spread * (co.x + co.z), v + spread * co.y))


AXES = {"x": 0, "y": 1, "z": 2}


def _project_uvs(obj, repeats_per_meter):
    """Box-projects UVs so the texture's grain follows the piece's grain axis."""
    mesh = obj.data
    if obj.get("keep_uv"):
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
        bm.to_mesh(mesh)
        bm.free()
        return
    grain_world = AXES[obj.get("grain_axis", "x")]
    tex_grain = mesh.materials[0].get("grain", "u") if mesh.materials else "u"
    bm = bmesh.new()
    bm.from_mesh(mesh)
    uv = bm.loops.layers.uv.verify()
    for face in bm.faces:
        n = face.normal
        d = max(range(3), key=lambda i: abs(n[i]))
        plane = [i for i in range(3) if i != d]
        if grain_world in plane:
            along = grain_world
            across = plane[0] if plane[1] == grain_world else plane[1]
        else:
            along, across = plane
        for loop in face.loops:
            co = loop.vert.co
            a, c = co[along] * repeats_per_meter, co[across] * repeats_per_meter
            loop[uv].uv = (a, c) if tex_grain == "u" else (c, a)
    # Triangles only: tangents (for normal maps) need tris or quads.
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    bm.to_mesh(mesh)
    bm.free()


def _join_by_material():
    """One mesh per material, so the engine draws each model in a few calls."""
    groups = {}
    for obj in bpy.context.scene.objects:
        if obj.type == "MESH":
            groups.setdefault(obj.data.materials[0].name, []).append(obj)
    view_layer = bpy.context.view_layer
    for name, objs in groups.items():
        if len(objs) < 2:
            continue
        bpy.ops.object.select_all(action="DESELECT")
        for o in objs:
            o.select_set(True)
        view_layer.objects.active = objs[0]
        bpy.ops.object.join()
        objs[0].name = name
    bpy.ops.object.select_all(action="DESELECT")


def finish_and_export(out_path, uv_scale, image_format="JPEG"):
    """Applies modifiers, adds UVs and smooth shading, and writes the .glb."""
    view_layer = bpy.context.view_layer
    for obj in list(bpy.context.scene.objects):
        if obj.type != "MESH":
            continue
        view_layer.objects.active = obj
        obj.select_set(True)
        for mod in list(obj.modifiers):
            bpy.ops.object.modifier_apply(modifier=mod.name)
        mat = obj.data.materials[0]
        _project_uvs(obj, obj.get("uv_scale", uv_scale.get(mat.name, 1.0)))
        if obj.get("smooth"):
            bpy.ops.object.shade_smooth_by_angle(angle=math.radians(35))
        else:
            # Harden-normals from the bevel already set custom normals; smooth
            # shading lets the rounded edges blend.
            bpy.ops.object.shade_smooth(keep_sharp_edges=True)
        obj.select_set(False)
    _join_by_material()
    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    bpy.ops.export_scene.gltf(
        filepath=out_path,
        export_format="GLB",
        export_yup=False,  # built y-up already
        export_apply=True,
        export_image_format=image_format,  # "AUTO" keeps PNG (needed for alpha)
        export_jpeg_quality=88,
        export_texcoords=True,
        export_normals=True,
        export_tangents=True,
        export_materials="EXPORT",
        export_cameras=False,
        export_lights=False,
    )
    tris = sum(len(p.vertices) - 2 for o in bpy.context.scene.objects if o.type == "MESH" for p in o.data.polygons)
    print("EXPORTED", out_path, os.path.getsize(out_path), "bytes,", tris, "triangles")


# ---- Pixel text for textures (signs, menus, posters) ----
# Uses the engine's own 5x7 pixel font (engine/font.cpp), so painted text
# matches the on-screen text.
_GLYPHS = None


def _glyphs():
    global _GLYPHS
    if _GLYPHS is None:
        import re
        src = open(os.path.join(os.path.dirname(__file__), "..", "..", "engine", "font.cpp")).read()
        _GLYPHS = {}
        for m in re.finditer(r"\{'(\\.|.)',\s*\{([^}]*)\}\}", src):
            c = m.group(1)[-1] if m.group(1).startswith("\\") else m.group(1)
            _GLYPHS[c] = re.findall(r'"([.#]{5})"', m.group(2))
    return _GLYPHS


def text_width(text, scale):
    return len(text) * 6 * scale - scale


def draw_text(px, text, x, y_top, scale, color, center=False):
    """Paints text into an image array whose row 0 is the bottom. (x, y_top)
    is the top-left of the first letter in pixels; `scale` = pixels per dot."""
    g = _glyphs()
    if center:
        x -= text_width(text, scale) // 2
    for i, ch in enumerate(text):
        rows = g.get(ch, g.get(ch.upper(), []))
        for r, row in enumerate(rows):
            for c, dot in enumerate(row):
                if dot != "#":
                    continue
                x0 = x + (i * 6 + c) * scale
                y1 = y_top - r * scale
                if x0 < 0 or y1 - scale < 0 or x0 + scale > px.shape[1] or y1 > px.shape[0]:
                    continue
                px[y1 - scale:y1, x0:x0 + scale, :3] = color


# ---- More shapes (2026-10-16, the big bar upgrade) ----

def glow_image_material(name, pixels, strength=1.0, base_scale=0.15, roughness=0.3):
    """A material whose image glows (screens, lit windows, printed neon):
    `pixels` (h, w, 3) linear, row 0 at the bottom. The same image, dimmed by
    `base_scale`, is the surface color, so it still looks right when lit."""
    h, w = pixels.shape[:2]
    px = np.ones((h, w, 4), dtype=np.float32)
    px[..., :3] = np.clip(pixels[..., :3], 0, 1)
    glow = _save_image(f"{name} glow", w, h, px, color=True)
    dim = px.copy()
    dim[..., :3] *= base_scale
    base = _save_image(f"{name} color", w, h, dim, color=True)
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    t_base = nt.nodes.new("ShaderNodeTexImage")
    t_base.image = base
    nt.links.new(t_base.outputs["Color"], bsdf.inputs["Base Color"])
    t_glow = nt.nodes.new("ShaderNodeTexImage")
    t_glow.image = glow
    nt.links.new(t_glow.outputs["Color"], bsdf.inputs["Emission Color"])
    bsdf.inputs["Emission Strength"].default_value = strength
    bsdf.inputs["Metallic"].default_value = 0.0
    bsdf.inputs["Roughness"].default_value = roughness
    mat["grain"] = "u"
    return mat


def quad(name, corners, mat, uvs=((0, 0), (1, 0), (1, 1), (0, 1))):
    """A flat 4-cornered face (counter-clockwise seen from its front) with its
    own UVs, e.g. a screen or a printed sign."""
    bm = bmesh.new()
    vs = [bm.verts.new(c) for c in corners]
    f = bm.faces.new(vs)
    uv = bm.loops.layers.uv.verify()
    for loop, t in zip(f.loops, uvs):
        loop[uv].uv = t
    obj = _new_object(name, bm, mat)
    obj["keep_uv"] = True
    return obj


def panel(name, center, right, up, w, h, mat, uvs=((0, 0), (1, 0), (1, 1), (0, 1))):
    """A rectangle `w` x `h` centered at `center`, spanned by unit vectors
    `right` and `up` (it faces right x up)."""
    c, r, u = Vector(center), Vector(right) * (w / 2), Vector(up) * (h / 2)
    return quad(name, [c - r - u, c + r - u, c + r + u, c - r + u], mat, uvs)


def prism(name, outline, y0, y1, mat, bevel=0.0, segments=2):
    """Extrudes a floor outline [(x, z), ...] (any winding) from y0 up to y1."""
    bm = bmesh.new()
    pts = list(outline)
    area = sum(pts[i][0] * pts[(i + 1) % len(pts)][1] - pts[(i + 1) % len(pts)][0] * pts[i][1]
               for i in range(len(pts)))
    if area > 0:
        pts.reverse()
    bottom = [bm.verts.new((x, y0, z)) for (x, z) in pts]
    top = [bm.verts.new((x, y1, z)) for (x, z) in pts]
    n = len(pts)
    bm.faces.new(bottom)
    bm.faces.new(list(reversed(top)))
    for i in range(n):
        j = (i + 1) % n
        bm.faces.new([bottom[i], top[i], top[j], bottom[j]])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    obj = _new_object(name, bm, mat)
    _bevel(obj, bevel, segments)
    return obj


def obox(name, center, size, yaw, mat, bevel=0.004, segments=2):
    """A box of `size` (x, y, z) turned by `yaw` radians around +y (same
    sense as the engine's turnY), centered at `center`."""
    hx, hy, hz = size[0] / 2, size[1] / 2, size[2] / 2
    obj = box(name, (-hx, -hy, -hz), (hx, hy, hz), mat, bevel, segments)
    obj.data.transform(Matrix.Translation(Vector(center)) @ Matrix.Rotation(yaw, 4, "Y"))
    return obj


def path_frames(path):
    """For a floor path [(x, z), ...]: per point (position, tangent, normal),
    where normal = tangent turned 90 degrees to the right (+z when going +x)."""
    out = []
    n = len(path)
    for i in range(n):
        a = Vector((*path[max(i - 1, 0)], 0))
        b = Vector((*path[min(i + 1, n - 1)], 0))
        t = (b - a).normalized()
        out.append((path[i], (t.x, t.y), (-t.y, t.x)))
    return out


def sweep(name, path, profile, mat, caps=True, smooth=False):
    """Sweeps a cross-section along a floor path. `path` is [(x, z), ...];
    `profile` is [(s, y), ...] going around the section, where s is the
    sideways offset along the path's right-hand normal (see path_frames)."""
    bm = bmesh.new()
    rings = []
    for (p, t, nrm) in path_frames(path):
        rings.append([bm.verts.new((p[0] + s * nrm[0], y, p[1] + s * nrm[1])) for (s, y) in profile])
    m = len(profile)
    for i in range(len(rings) - 1):
        ra, rb = rings[i], rings[i + 1]
        for j in range(m):
            k = (j + 1) % m
            bm.faces.new([ra[j], ra[k], rb[k], rb[j]])
    if caps:
        bm.faces.new(list(reversed(rings[0])))
        bm.faces.new(rings[-1])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return mesh_object(name, bm, mat, smooth)


def resample(path, step):
    """Evenly spaced points along a polyline [(x, z), ...]."""
    pts = [Vector(p) for p in path]
    out = [pts[0]]
    carry = 0.0
    for a, b in zip(pts, pts[1:]):
        seg = (b - a).length
        d = step - carry
        while d <= seg:
            out.append(a + (b - a) * (d / seg))
            d += step
        carry = seg - (d - step)
    if (out[-1] - pts[-1]).length > 1e-4:
        out.append(pts[-1])
    return [(p.x, p.y) for p in out]


def noise2d(h, w, scale, seed=0, octaves=4):
    """Smooth value noise in 0..1, (h, w)."""
    rng = np.random.default_rng(seed)
    out = np.zeros((h, w), dtype=np.float32)
    amp, total = 1.0, 0.0
    for o in range(octaves):
        cells = max(2, int(scale * (2 ** o)))
        grid = rng.random((cells + 1, cells + 1)).astype(np.float32)
        ys = np.linspace(0, cells, h, endpoint=False)
        xs = np.linspace(0, cells, w, endpoint=False)
        y0, x0 = ys.astype(int), xs.astype(int)
        fy, fx = ys - y0, xs - x0
        fy, fx = fy * fy * (3 - 2 * fy), fx * fx * (3 - 2 * fx)
        a = grid[y0][:, x0]
        b = grid[y0][:, x0 + 1]
        c = grid[y0 + 1][:, x0]
        d = grid[y0 + 1][:, x0 + 1]
        top = a + (b - a) * fx[None, :]
        bot = c + (d - c) * fx[None, :]
        out += amp * (top + (bot - top) * fy[:, None])
        total += amp
        amp *= 0.5
    return out / total


def srgb(c):
    """sRGB 0..1 color (like a color picker) to the linear values images use."""
    return tuple(((v + 0.055) / 1.055) ** 2.4 if v > 0.04045 else v / 12.92 for v in c)


def glow_cutout_material(name, rgba, strength=4.0):
    """A see-through glowing image (flames, sparks): `rgba` (h, w, 4) linear
    with alpha, row 0 at the bottom. Drawn blended, glowing where opaque."""
    h, w = rgba.shape[:2]
    img = _save_image(f"{name} color", w, h, np.clip(rgba, 0, 1).astype(np.float32), color=True, alpha=True)
    img.alpha_mode = "STRAIGHT"
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nt = mat.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    t = nt.nodes.new("ShaderNodeTexImage")
    t.image = img
    nt.links.new(t.outputs["Color"], bsdf.inputs["Base Color"])
    nt.links.new(t.outputs["Color"], bsdf.inputs["Emission Color"])
    nt.links.new(t.outputs["Alpha"], bsdf.inputs["Alpha"])
    bsdf.inputs["Emission Strength"].default_value = strength
    if hasattr(mat, "surface_render_method"):
        mat.surface_render_method = "BLENDED"
    if hasattr(mat, "blend_method"):
        mat.blend_method = "BLEND"
    mat["grain"] = "u"
    return mat
