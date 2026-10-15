# Renders a quick preview picture of each .glb (Cycles on the CPU), to check
# models before loading them in the engine.
#
#   blender -b --factory-startup --python tools/assets/preview.py -- <out dir> <a.glb> [b.glb ...]
#
# Writes <out dir>/<name>.png: a 3/4 view from the front (+z) right, framed
# to the model's bounds.
import math
import os
import sys

import bpy
from mathutils import Vector

args = sys.argv[sys.argv.index("--") + 1:]
out_dir, files = args[0], args[1:]
os.makedirs(out_dir, exist_ok=True)

for path in files:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=path)
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    lo = Vector((1e9, 1e9, 1e9))
    hi = -lo
    for o in meshes:
        for c in o.bound_box:
            w = o.matrix_world @ Vector(c)
            lo = Vector(map(min, lo, w))
            hi = Vector(map(max, hi, w))
    center = (lo + hi) / 2
    size = (hi - lo).length
    # Blender's import turns y-up into z-up: the model's front (+z) is -y here.
    cam_data = bpy.data.cameras.new("cam")
    cam_data.lens = 50
    cam = bpy.data.objects.new("cam", cam_data)
    bpy.context.scene.collection.objects.link(cam)
    d = Vector((0.55, -1.0, 0.45)).normalized()
    cam.location = center + d * size * 1.25
    cam.rotation_euler = (center - cam.location).to_track_quat("-Z", "Y").to_euler()
    bpy.context.scene.camera = cam
    for (loc, energy) in (((3, -4, 5), 600), ((-4, -2, 3), 250), ((0, 5, 4), 300)):
        ld = bpy.data.lights.new("l", "AREA")
        ld.size = 3
        ld.energy = energy * max(size, 0.5) ** 2
        lo_ = bpy.data.objects.new("l", ld)
        lo_.location = center + Vector(loc) * max(size, 0.5) * 0.6
        lo_.rotation_euler = (center - lo_.location).to_track_quat("-Z", "Y").to_euler()
        bpy.context.scene.collection.objects.link(lo_)
    world = bpy.data.worlds.new("w")
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs[0].default_value = (0.25, 0.25, 0.27, 1)
    world.node_tree.nodes["Background"].inputs[1].default_value = 0.6
    bpy.context.scene.world = world
    sc = bpy.context.scene
    sc.render.engine = "CYCLES"
    sc.cycles.device = "CPU"
    sc.cycles.samples = 24
    sc.cycles.use_denoising = False
    sc.render.resolution_x = sc.render.resolution_y = 480
    sc.render.filepath = os.path.join(out_dir, os.path.splitext(os.path.basename(path))[0] + ".png")
    bpy.ops.render.render(write_still=True)
    print("PREVIEW", sc.render.filepath)
