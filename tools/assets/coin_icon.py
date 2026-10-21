# The menu's credit icon, rendered from the coin model itself
# (assets/models/credit_coin.glb), slightly turned, on a clear background.
#
#   blender -b --factory-startup --python tools/assets/coin_icon.py -- <credit_coin.glb> <out.png> [size]
import math
import sys

import bpy
from mathutils import Vector

args = sys.argv[sys.argv.index("--") + 1:]
glb, out = args[0], args[1]
size = int(args[2]) if len(args) > 2 else 128

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=glb)
scene = bpy.context.scene
scene.render.engine = "CYCLES"
scene.cycles.samples = 96
scene.cycles.use_denoising = True
scene.render.film_transparent = True
scene.render.resolution_x = scene.render.resolution_y = size
scene.render.image_settings.file_format = "PNG"
scene.render.image_settings.color_mode = "RGBA"
scene.view_settings.view_transform = "Standard"
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
cam.data.type = "ORTHO"
cam.data.ortho_scale = 0.135
# The coin faces -y in Blender (glTF's +z): look at it from the front, a
# little from the right and above, so the ridged edge shows.
from mathutils import Vector
cam.location = Vector((0.42, -1.0, 0.22))
cam.rotation_euler = (-cam.location).to_track_quat("-Z", "Y").to_euler()
scene.collection.objects.link(cam)
scene.camera = cam
# Warm key light, cool rim, soft fill: shiny gold needs something to reflect.
for name, loc, energy, color in (("Key", (0.7, -1.0, 0.9), 14, (1, 0.9, 0.75)), ("Rim", (-0.9, 0.3, 0.6), 10, (0.6, 0.75, 1)),
                                 ("Fill", (-0.6, -0.9, -0.2), 5, (1, 0.6, 0.85))):
    l = bpy.data.objects.new(name, bpy.data.lights.new(name, "AREA"))
    l.data.energy = energy
    l.data.size = 0.6
    l.data.color = color
    l.location = Vector(loc)
    direction = -l.location
    l.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    scene.collection.objects.link(l)
world = bpy.data.worlds.new("World")
world.use_nodes = True
world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.25, 0.18, 0.3, 1)
world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.35
scene.world = world
scene.render.filepath = out
bpy.ops.render.render(write_still=True)
print("RENDERED", out)
