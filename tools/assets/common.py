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


def _load_pixels(path):
    img = bpy.data.images.load(path)
    w, h = img.size
    px = np.empty(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(px)
    bpy.data.images.remove(img)
    return w, h, px.reshape(h, w, 4)


def _save_image(name, w, h, px, color):
    img = bpy.data.images.new(name, w, h, alpha=False)
    img.colorspace_settings.name = "sRGB" if color else "Non-Color"
    img.pixels.foreach_set(px.astype(np.float32).ravel())
    path = os.path.join(_tmp, name.replace(" ", "_") + ".png")
    img.filepath_raw = path
    img.file_format = "PNG"
    img.save()
    img.colorspace_settings.name = "sRGB" if color else "Non-Color"
    return img


def wood_material(name, tex_dir, prefix, tint=(1, 1, 1), roughness_scale=1.0, roughness_bias=0.0, grain="u"):
    """A PBR wood material from a Poly Haven texture set.

    The color map is tinted, and the roughness map is scaled and packed into
    the glTF metal/roughness layout (G = roughness, B = metallic = 0).
    `grain` says which texture axis the grain runs along ("u" or "v").
    """
    w, h, diff = _load_pixels(os.path.join(tex_dir, f"{prefix}_diff_1k.jpg"))
    # Blender stores sRGB images as linear floats once loaded; tint in linear.
    diff[..., 0] *= tint[0]
    diff[..., 1] *= tint[1]
    diff[..., 2] *= tint[2]
    base = _save_image(f"{name} color", w, h, np.clip(diff, 0, 1), color=True)

    _, _, rough = _load_pixels(os.path.join(tex_dir, f"{prefix}_rough_1k.jpg"))
    mr = np.zeros_like(rough)
    mr[..., 0] = 1.0
    mr[..., 1] = np.clip(rough[..., 0] * roughness_scale + roughness_bias, 0.02, 1.0)
    mr[..., 2] = 0.0
    mr[..., 3] = 1.0
    mr_img = _save_image(f"{name} metal-rough", w, h, mr, color=False)

    _, _, nrm = _load_pixels(os.path.join(tex_dir, f"{prefix}_nor_gl_1k.jpg"))
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
    nt.links.new(t_n.outputs["Color"], nmap.inputs["Color"])
    nt.links.new(nmap.outputs["Normal"], bsdf.inputs["Normal"])
    mat["grain"] = grain
    return mat


def plain_material(name, color, metallic, roughness):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    mat["grain"] = "u"
    return mat


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
    _bevel(obj, min(bevel, radius / 3), 2)
    return obj


def sphere(name, center, radius, mat, segments=24, rings=12):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segments, v_segments=rings, radius=radius)
    bmesh.ops.translate(bm, vec=center, verts=bm.verts)
    obj = _new_object(name, bm, mat)
    obj["smooth"] = True
    return obj


AXES = {"x": 0, "y": 1, "z": 2}


def _project_uvs(obj, repeats_per_meter):
    """Box-projects UVs so the texture's grain follows the piece's grain axis."""
    mesh = obj.data
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


def finish_and_export(out_path, uv_scale):
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
        _project_uvs(obj, uv_scale.get(mat.name, 1.0))
        if obj.get("smooth"):
            bpy.ops.object.shade_smooth_by_angle(angle=math.radians(35))
        else:
            # Harden-normals from the bevel already set custom normals; smooth
            # shading lets the rounded edges blend.
            bpy.ops.object.shade_smooth(keep_sharp_edges=True)
        obj.select_set(False)
    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    bpy.ops.export_scene.gltf(
        filepath=out_path,
        export_format="GLB",
        export_yup=False,  # built y-up already
        export_apply=True,
        export_image_format="JPEG",
        export_jpeg_quality=88,
        export_texcoords=True,
        export_normals=True,
        export_tangents=True,
        export_materials="EXPORT",
        export_cameras=False,
        export_lights=False,
    )
    print("EXPORTED", out_path, os.path.getsize(out_path), "bytes")
