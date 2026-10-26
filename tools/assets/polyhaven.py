# Brings one Poly Haven model (CC0, fetched by polyhaven_fetch.sh) into the
# bar's kit (J35): imports its glTF, slims it to a triangle budget (Blender's
# Decimate, which keeps the UVs), shrinks its textures, sets its origin at
# the middle of its footprint on the floor (so it stands where it's placed),
# optionally scales it to a real height, and writes the .glb.
#
#   blender -b --factory-startup --python tools/assets/polyhaven.py -- \
#       <fetched dir> <out.glb> [--tris N] [--tex 1024] [--height metres] [--turn degrees]
#
# Poly Haven models are already in metres; --height only when a real size
# differs. --turn spins it about the vertical so its front faces +z (the
# bar's facing convention).
import math
import os
import sys

import bpy
from mathutils import Matrix, Vector

args = sys.argv[sys.argv.index("--") + 1:]
src, out_path = args[0], args[1]


def opt(name, default):
    return type(default)(args[args.index(name) + 1]) if name in args else default


max_tris = opt("--tris", 0)
tex_size = opt("--tex", 1024)
height = opt("--height", 0.0)
turn = opt("--turn", 0.0)

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=os.path.join(src, "model.gltf"))
meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]


def triangles():
    return sum(len(p.vertices) - 2 for o in meshes for p in o.data.polygons)


# Bake every object's place into its mesh (the model is one piece).
for o in meshes:
    o.select_set(True)
bpy.context.view_layer.objects.active = meshes[0]
bpy.ops.object.parent_clear(type="CLEAR_KEEP_TRANSFORM")
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
for o in list(bpy.context.scene.objects):
    if o.type != "MESH":
        bpy.data.objects.remove(o)

before = triangles()
if max_tris and before > max_tris:
    ratio = max_tris / before
    for o in meshes:
        m = o.modifiers.new("slim", "DECIMATE")
        m.ratio = ratio
        m.use_collapse_triangulate = True
        bpy.context.view_layer.objects.active = o
        bpy.ops.object.modifier_apply(modifier=m.name)

# Size, facing and origin (Blender is z-up here; the export turns it y-up).
lo = Vector((math.inf,) * 3)
hi = Vector((-math.inf,) * 3)
for o in meshes:
    for v in o.data.vertices:
        lo = Vector(map(min, lo, v.co))
        hi = Vector(map(max, hi, v.co))
scale = height / (hi.z - lo.z) if height > 0 else 1.0
center = Vector(((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, lo.z))
m = Matrix.Rotation(math.radians(turn), 4, "Z") @ Matrix.Scale(scale, 4) @ Matrix.Translation(-center)
for o in meshes:
    o.data.transform(m)
    o.data.update()

# Textures no bigger than the budget (the phone gets ASTC from these).
for img in bpy.data.images:
    if img.size[0] > tex_size or img.size[1] > tex_size:
        k = tex_size / max(img.size)
        img.scale(max(1, int(img.size[0] * k)), max(1, int(img.size[1] * k)))
    img.pack()

os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
bpy.ops.export_scene.gltf(filepath=out_path, export_format="GLB", export_image_format="JPEG",
                          export_jpeg_quality=88, export_texcoords=True, export_normals=True,
                          export_tangents=True, export_materials="EXPORT", export_cameras=False,
                          export_lights=False)
size = (hi - lo) * scale
print("EXPORTED", out_path, os.path.getsize(out_path), "bytes,", before, "→", triangles(), "triangles,",
      "size %.2f x %.2f x %.2f m (w, d, h)" % (size.x, size.y, size.z))
